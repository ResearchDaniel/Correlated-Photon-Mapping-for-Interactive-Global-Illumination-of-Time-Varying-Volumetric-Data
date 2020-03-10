/*********************************************************************************
 *
 * Copyright (c) 2016, Daniel Jönsson
 * All rights reserved.
 * 
 * This work is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
 * http://creativecommons.org/licenses/by-nc/4.0/
 * 
 * You are free to:
 * 
 * Share — copy and redistribute the material in any medium or format
 * Adapt — remix, transform, and build upon the material
 * The licensor cannot revoke these freedoms as long as you follow the license terms.
 * Under the following terms:
 * 
 * Attribution — You must give appropriate credit, provide a link to the license, and indicate if changes were made. You may do so in any reasonable manner, but not in any way that suggests the licensor endorses you or your use.
 * NonCommercial — You may not use the material for commercial purposes.
 * No additional restrictions — You may not apply legal terms or technological measures that legally restrict others from doing anything the license permits.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *********************************************************************************/

#include "minmaxuniformgrid3dimportanceclprocessor.h"
#include <inviwo/core/util/colorconversion.h>
#include <modules/opencl/buffer/buffercl.h>
#include <modules/opencl/syncclgl.h>
#include <glm/gtc/epsilon.hpp>
#define IVW_DETAILED_PROFILING
namespace inviwo {
    
// The Class Identifier has to be globally unique. Use a reverse DNS naming
// scheme
const ProcessorInfo MinMaxUniformGrid3DImportanceCLProcessor::processorInfo_{
    "org.inviwo.MinMaxUniformGrid3DImportanceCLProcessor",  // Class identifier
    "MinMaxUniformGrid3DImportance",                          // Display name
    "UniformGrid3D",                                            // Category
    CodeState::Experimental,                                  // Code state
    Tags::CL,                                                 // Tags
};
const ProcessorInfo MinMaxUniformGrid3DImportanceCLProcessor::getProcessorInfo() const {
    return processorInfo_;
}

MinMaxUniformGrid3DImportanceCLProcessor::MinMaxUniformGrid3DImportanceCLProcessor()
: Processor()
, ProcessorKernelOwner(this)
, incrementalImportance("incrementalImportance", "Incremental importance", true)
, minMaxUniformGrid3DInport_("minMaxUniformGrid3D")
, volumeDifferenceInfoInport_("volumeDifferenceInfo")
, importanceUniformGrid3DOutport_("importanceUniformGrid3D")
, opacityWeight_("constantWeight", "Opacity weight", 1.f, 0.f, 1.f)
, opacityDiffWeight_("opacityDiffWeight", "Opacity difference weight", 0.f, 0.f, 1.f)
, colorWeight_("colorWeight", "Color weight", 0.f, 0.f, 1.f)
, colorDiffWeight_("colorDiffWeight", "Color difference weight", 0.f, 0.f, 1.f)
, useAssociatedColor_("useAssociatedColor", "Associated color", false)
, TFPointEpsilon_("TFPointEpsilon", "Minimum change threshold", 1e-4f, 0.f, 1e-2f, 1e-3f)
, transferFunction_("transferfunction", "Transfer function")
, workGroupSize_("wgsize", "Work group size", 128, 1, 4096)
, useGLSharing_("glsharing", "Use OpenGL sharing", true)
, importanceUniformGrid3D_(std::make_shared<ImportanceUniformGrid3D>()) {
    addPort(minMaxUniformGrid3DInport_);
    minMaxUniformGrid3DInport_.onChange(
                                        [this]() { setInvalidationReason(InvalidationReason::Volume); });
    volumeDifferenceInfoInport_.setOptional(true);
    addPort(volumeDifferenceInfoInport_);
    addPort(importanceUniformGrid3DOutport_);
    
    addProperty(incrementalImportance);
    addProperty(opacityWeight_);
    addProperty(opacityDiffWeight_);
    addProperty(colorWeight_);
    addProperty(colorDiffWeight_);
    addProperty(useAssociatedColor_);
    addProperty(TFPointEpsilon_);
    // opacityWeight_.onChange([this](){
    // setInvalidationReason(InvalidationReason::TransferFunction); });
    // opacityDiffWeight_.onChange([this](){
    // setInvalidationReason(InvalidationReason::TransferFunction); });
    // colorDiffWeight_.onChange([this](){
    // setInvalidationReason(InvalidationReason::TransferFunction); });
    useAssociatedColor_.onChange(
                                 [this]() { setInvalidationReason(InvalidationReason::TransferFunction); });
    
    addProperty(transferFunction_);
    transferFunction_.onChange(
                               [this]() { setInvalidationReason(InvalidationReason::TransferFunction); });
    addProperty(workGroupSize_);
    addProperty(useGLSharing_);
    kernel_ =
    addKernel("minmaxuniformgrid3dimportance.cl", "classifyMinMaxUniformGrid3DImportanceKernel",
              "", " -D INCREMENTAL_TF_IMPORTANCE");
    timeVaryingKernel_ = addKernel("minmaxuniformgrid3dimportance.cl",
                                   "classifyTimeVaryingMinMaxUniformGrid3DImportanceKernel");
    
    importanceUniformGrid3DOutport_.setData(importanceUniformGrid3D_);
    // Count as unused if cleared
    prevTransferFunction_.clear();
}

void MinMaxUniformGrid3DImportanceCLProcessor::process() {
    if (!kernel_ || !timeVaryingKernel_) {
        return;
    }
    const MinMaxUniformGrid3D *minMaxUniformGrid3D =
    dynamic_cast<const MinMaxUniformGrid3D *>(minMaxUniformGrid3DInport_.getData().get());
    if (!minMaxUniformGrid3D) {
        LogError("minMaxUniformGrid3DInport_ expects MinMaxUniformGrid3D as input");
        return;
    }
    if (glm::any(glm::notEqual(minMaxUniformGrid3D->getDimensions(),
                               importanceUniformGrid3D_->getDimensions()))) {
        importanceUniformGrid3D_->setDimensions(minMaxUniformGrid3D->getDimensions());
        importanceUniformGrid3D_->setCellDimension(minMaxUniformGrid3D->getCellDimension());
        importanceUniformGrid3D_->setModelMatrix(minMaxUniformGrid3D->getModelMatrix());
        importanceUniformGrid3D_->setWorldMatrix(minMaxUniformGrid3D->getWorldMatrix());
    }
    if (static_cast<int>(invalidationFlag_) &
        static_cast<int>(InvalidationReason::TransferFunction)) {
        if (prevTransferFunction_.size() == 0 || !incrementalImportance) {
            updateTransferFunctionData();
        } else {
            updateTransferFunctionDifferenceData();
        }
        prevTransferFunction_ = transferFunction_.get();
    } else if (static_cast<int>(invalidationFlag_) & static_cast<int>(InvalidationReason::Volume)) {
        updateTransferFunctionData();
    }
    size3_t dim = importanceUniformGrid3D_->getDimensions();
    size_t nElements = dim.x * dim.y * dim.z;
    
    size_t localWorkGroupSize(workGroupSize_.get());
    size_t globalWorkGroupSize(getGlobalWorkGroupSize(nElements, localWorkGroupSize));
#ifdef IVW_DETAILED_PROFILING
    IVW_OPENCL_PROFILING(profilingEvent, "")
#else
    cl::Event *profilingEvent = nullptr;
#endif
    
    if (volumeDifferenceInfoInport_.isReady() && prevMinMaxUniformGrid3D_ != nullptr &&
        prevMinMaxUniformGrid3D_.get() != minMaxUniformGrid3D) {
        // Time varying data changed
        auto volumeDifferenceData = dynamic_cast<const DynamicVolumeInfoUniformGrid3D *>(
                                                                                         volumeDifferenceInfoInport_.getData().get());
        if (!volumeDifferenceData) {
            LogError(
                     "volumeDifferenceInfoInport_ expects "
                     "DynamicVolumeInfoUniformGrid3D as input");
            return;
        }
        if (useGLSharing_) {
            SyncCLGL glSync;
            auto minMaxUniformGrid3DCL = minMaxUniformGrid3D->data.getRepresentation<BufferCLGL>();
            auto prevMinMaxUniformGrid3DCL =
            prevMinMaxUniformGrid3D_->data.getRepresentation<BufferCLGL>();
            auto volumeDifferenceInfoCL =
            volumeDifferenceData->data.getRepresentation<BufferCLGL>();
            
            auto importanceUniformGrid3DCL =
            importanceUniformGrid3D_->data.getEditableRepresentation<BufferCLGL>();
            glSync.addToAquireGLObjectList(minMaxUniformGrid3DCL);
            glSync.addToAquireGLObjectList(prevMinMaxUniformGrid3DCL);
            glSync.addToAquireGLObjectList(volumeDifferenceInfoCL);
            glSync.addToAquireGLObjectList(importanceUniformGrid3DCL);
            
            glSync.aquireAllObjects();
            computeImportance(minMaxUniformGrid3DCL, prevMinMaxUniformGrid3DCL,
                              volumeDifferenceInfoCL, nElements, importanceUniformGrid3DCL,
                              globalWorkGroupSize, localWorkGroupSize, profilingEvent);
        } else {
            auto minMaxUniformGrid3DCL = minMaxUniformGrid3D->data.getRepresentation<BufferCL>();
            auto volumeDifferenceInfoCL = volumeDifferenceData->data.getRepresentation<BufferCL>();
            auto prevMinMaxUniformGrid3DCL =
            prevMinMaxUniformGrid3D_->data.getRepresentation<BufferCL>();
            auto importanceUniformGrid3DCL =
            importanceUniformGrid3D_->data.getEditableRepresentation<BufferCL>();
            computeImportance(minMaxUniformGrid3DCL, prevMinMaxUniformGrid3DCL,
                              volumeDifferenceInfoCL, nElements, importanceUniformGrid3DCL,
                              globalWorkGroupSize, localWorkGroupSize, profilingEvent);
        }
        
    } else {
        // Transfer function changed
        if (useGLSharing_) {
            SyncCLGL glSync;
            auto minMaxUniformGrid3DCL = minMaxUniformGrid3D->data.getRepresentation<BufferCLGL>();
            
            auto importanceUniformGrid3DCL =
            importanceUniformGrid3D_->data.getEditableRepresentation<BufferCLGL>();
            glSync.addToAquireGLObjectList(minMaxUniformGrid3DCL);
            glSync.addToAquireGLObjectList(importanceUniformGrid3DCL);
            glSync.aquireAllObjects();
            computeImportance(minMaxUniformGrid3DCL, nElements, importanceUniformGrid3DCL,
                              globalWorkGroupSize, localWorkGroupSize, profilingEvent);
        } else {
            auto minMaxUniformGrid3DCL = minMaxUniformGrid3D->data.getRepresentation<BufferCL>();
            auto importanceUniformGrid3DCL =
            importanceUniformGrid3D_->data.getEditableRepresentation<BufferCL>();
            computeImportance(minMaxUniformGrid3DCL, nElements, importanceUniformGrid3DCL,
                              globalWorkGroupSize, localWorkGroupSize, profilingEvent);
        }
    }
    prevMinMaxUniformGrid3D_ =
    std::dynamic_pointer_cast<const MinMaxUniformGrid3D>(minMaxUniformGrid3DInport_.getData());
    
    invalidationFlag_ = InvalidationReason(0);
}

void MinMaxUniformGrid3DImportanceCLProcessor::computeImportance(
                                                                 const BufferCLBase *minMaxUniformGridCL, size_t nElements,
                                                                 BufferCLBase *importanceUniformGridCL, const size_t &globalWorkGroupSize,
                                                                 const size_t &localWorkgroupSize, cl::Event *event) {
    // Transfer function parameters
    
    try {
        auto positionsCL = tfPointPositions_.getRepresentation<BufferCL>();
        auto colorsCL = tfPointColors_.getRepresentation<BufferCL>();
        // Make weights sum to 1
        auto weightNormalization = colorWeight_.get() + colorDiffWeight_.get() +
        opacityDiffWeight_.get() + opacityWeight_.get();
        if (weightNormalization <= 0.f) {
            weightNormalization = 1.f;
        }
        
        auto labColorNormalizationFactor = getLabColorNormalizationFactor();
        
        int argIndex = 0;
        kernel_->setArg(argIndex++, *minMaxUniformGridCL);
        kernel_->setArg(argIndex++, static_cast<int>(nElements));
        kernel_->setArg(argIndex++, *positionsCL);
        kernel_->setArg(argIndex++, *colorsCL);
        kernel_->setArg(argIndex++,
                        std::min(static_cast<int>(positionsCL->getSize()), tfPointImportanceSize_));
        kernel_->setArg(argIndex++,
                        colorWeight_.get() * labColorNormalizationFactor / (weightNormalization));
        kernel_->setArg(argIndex++, colorDiffWeight_.get() * labColorNormalizationFactor /
                        (weightNormalization));
        kernel_->setArg(argIndex++, opacityDiffWeight_.get() / weightNormalization);
        kernel_->setArg(argIndex++, opacityWeight_.get() / weightNormalization);
        
        kernel_->setArg(argIndex++, *importanceUniformGridCL);
        OpenCL::getPtr()->getQueue().enqueueNDRangeKernel(
                                                          *kernel_, cl::NullRange, globalWorkGroupSize, localWorkgroupSize, NULL, event);
    } catch (cl::Error &err) {
        LogError(getCLErrorString(err));
    }
}

void MinMaxUniformGrid3DImportanceCLProcessor::computeImportance(
                                                                 const BufferCLBase *minMaxUniformGridCL, const BufferCLBase *prevMinMaxUniformGridCL,
                                                                 const BufferCLBase *volumeDifferenceInfoUniformGridCL, size_t nElements,
                                                                 BufferCLBase *importanceUniformGridCL, const size_t &globalWorkGroupSize,
                                                                 const size_t &localWorkgroupSize, cl::Event *event) {
    try {
        auto positionsCL = tfPointPositions_.getRepresentation<BufferCL>();
        auto colorsCL = tfPointColors_.getRepresentation<BufferCL>();
        // Make weights sum to 1
        auto weightNormalization = colorWeight_.get() + colorDiffWeight_.get() +
        opacityDiffWeight_.get() + opacityWeight_.get();
        if (weightNormalization <= 0.f) {
            weightNormalization = 1.f;
        }
        
        auto labColorNormalizationFactor = getLabColorNormalizationFactor();
        int argIndex = 0;
        timeVaryingKernel_->setArg(argIndex++, *minMaxUniformGridCL);
        timeVaryingKernel_->setArg(argIndex++, *prevMinMaxUniformGridCL);
        timeVaryingKernel_->setArg(argIndex++, *volumeDifferenceInfoUniformGridCL);
        timeVaryingKernel_->setArg(argIndex++, static_cast<int>(nElements));
        timeVaryingKernel_->setArg(argIndex++, *positionsCL);
        timeVaryingKernel_->setArg(argIndex++, *colorsCL);
        timeVaryingKernel_->setArg(argIndex++, static_cast<int>(tfPointImportanceSize_));
        timeVaryingKernel_->setArg(
                                   argIndex++, colorWeight_.get() * labColorNormalizationFactor / (weightNormalization));
        timeVaryingKernel_->setArg(
                                   argIndex++,
                                   colorDiffWeight_.get() * labColorNormalizationFactor / (weightNormalization));
        timeVaryingKernel_->setArg(argIndex++, opacityDiffWeight_.get() / weightNormalization);
        timeVaryingKernel_->setArg(argIndex++, opacityWeight_.get() / weightNormalization);
        
        timeVaryingKernel_->setArg(argIndex++, *importanceUniformGridCL);
        OpenCL::getPtr()->getQueue().enqueueNDRangeKernel(*timeVaryingKernel_, cl::NullRange,
                                                          globalWorkGroupSize, localWorkgroupSize,
                                                          NULL, event);
    } catch (cl::Error &err) {
        LogError(getCLErrorString(err));
    }
}

float MinMaxUniformGrid3DImportanceCLProcessor::getLabColorNormalizationFactor() const {
    vec3 labColorSpaceExtent{100.f, 500.f, 400.f};
    return 1.f / glm::length(labColorSpaceExtent);
}

void MinMaxUniformGrid3DImportanceCLProcessor::updateTransferFunctionData() {
    if (transferFunction_.get().size() == 0) {
        tfPointImportanceSize_ = 2;
        if (static_cast<int>(tfPointPositions_.getSize()) < tfPointImportanceSize_) {
            tfPointPositions_.setSize(tfPointImportanceSize_);
            tfPointColors_.setSize(tfPointImportanceSize_);
        }
        auto positions = tfPointPositions_.getEditableRAMRepresentation();
        auto colors = tfPointColors_.getEditableRAMRepresentation();
        (*positions)[0] = 0;
        (*colors)[0] = vec4(0.f);
        (*positions)[1] = 1;
        (*colors)[1] = vec4(0.f);
        
    } else {
        auto firstPoint = transferFunction_.get().get(0);
        auto lastPoint =
        transferFunction_.get().get(static_cast<int>(transferFunction_.get().size() - 1));
        tfPointImportanceSize_ = static_cast<int>(transferFunction_.get().size());
        if (firstPoint.getPosition() > 0.f) {
            ++tfPointImportanceSize_;
        }
        if (lastPoint.getPosition() < 1.f) {
            ++tfPointImportanceSize_;
        }
        
        if (static_cast<int>(tfPointPositions_.getSize()) < tfPointImportanceSize_) {
            tfPointPositions_.setSize(tfPointImportanceSize_);
            tfPointColors_.setSize(tfPointImportanceSize_);
        }
        auto positions = tfPointPositions_.getEditableRAMRepresentation();
        auto colors = tfPointColors_.getEditableRAMRepresentation();
        
        int pointId = 0;
        if (firstPoint.getPosition() > 0.f) {
            (*positions)[0] = 0;
            if (useAssociatedColor_)
            (*colors)[0] = firstPoint.getColor() * firstPoint.getAlpha();
            else
            (*colors)[0] = firstPoint.getColor();
            ++pointId;
        }
        for (auto i = 0u; i < transferFunction_.get().size(); ++i, ++pointId) {
            auto point = transferFunction_.get().get(i);
            (*positions)[pointId] = static_cast<float>(point.getPosition());
            if (useAssociatedColor_)
            (*colors)[pointId] = point.getColor() * point.getAlpha();
            else
            (*colors)[pointId] = point.getColor();
        }
        if (lastPoint.getPosition() < 1.) {
            (*positions)[pointId] = 1.f;
            if (useAssociatedColor_)
            (*colors)[pointId++] = lastPoint.getColor() * lastPoint.getAlpha();
            else
            (*colors)[pointId++] = lastPoint.getColor();
        }
    }
}

void MinMaxUniformGrid3DImportanceCLProcessor::updateTransferFunctionDifferenceData() {
    if (transferFunction_.get().size() == 0 && prevTransferFunction_.size() == 0) {
        tfPointImportanceSize_ = 2;
        if (static_cast<int>(tfPointPositions_.getSize()) < tfPointImportanceSize_) {
            tfPointPositions_.setSize(tfPointImportanceSize_);
            tfPointColors_.setSize(tfPointImportanceSize_);
        }
        auto positions = tfPointPositions_.getEditableRAMRepresentation();
        auto colors = tfPointColors_.getEditableRAMRepresentation();
        (*positions)[0] = 0;
        (*colors)[0] = vec4(0.f);
        (*positions)[1] = 0;
        (*colors)[1] = vec4(0.f);
        
    } else {
        auto maxSize = transferFunction_.get().size() + prevTransferFunction_.size() + 2;
        if (tfPointPositions_.getSize() < maxSize) {
            tfPointPositions_.setSize(maxSize);
            tfPointColors_.setSize(maxSize);
        }
        auto positions = tfPointPositions_.getEditableRAMRepresentation();
        auto colors = tfPointColors_.getEditableRAMRepresentation();
        
        auto firstPoint = transferFunction_.get().get(0);
        //auto lastPoint =
        //    transferFunction_.get().get(static_cast<int>(transferFunction_.get().size() - 1));
        auto prevFirstPoint = prevTransferFunction_.get(0);
        //auto prevLastPoint =
        //    prevTransferFunction_.get(static_cast<int>(prevTransferFunction_.size() - 1));
        
        int outId = 0;
        int id = 0, prevId = 0;
        TFPrimitive p1, p2;
        p1 = p2 = TFPrimitive(firstPoint < prevFirstPoint ? firstPoint.getPosition()
                              : prevFirstPoint.getPosition(),
                              tfPointColorDiff(firstPoint.getColor(), prevFirstPoint.getColor()));
        if (firstPoint.getPosition() != prevFirstPoint.getPosition() &&
            firstPoint.getAlpha() == 0.f && prevFirstPoint.getAlpha() == 0.f) {
            // Moved first point with zero opacity
            if (firstPoint < prevFirstPoint) {
                auto a2 = transferFunction_.get().get(
                                                      std::min(1, static_cast<int>(transferFunction_.get().size() - 1)));
                auto p = mix(firstPoint, a2, prevFirstPoint);
                p2 = TFPrimitive(prevFirstPoint.getPosition(),
                                 tfPointColorDiff(prevFirstPoint.getColor(), p.getColor()));
            } else {
                auto a2 = prevTransferFunction_.get(
                                                    std::min(1, static_cast<int>(prevTransferFunction_.size() - 1)));
                auto p = mix(prevFirstPoint, a2, firstPoint);
                p2 = TFPrimitive(firstPoint.getPosition(),
                                 tfPointColorDiff(firstPoint.getColor(), p.getColor()));
            }
        }
        if (p1.getPosition() > 0.f &&
            (firstPoint.getAlpha() > 0.f || prevFirstPoint.getAlpha() > 0.f) &&
            glm::any(glm::epsilonNotEqual(p1.getColor(), vec4(0), TFPointEpsilon_.get()))) {
            (*positions)[outId] = 0.f;
            (*colors)[outId] = p1.getColor();
            ++outId;
            // This point will be added in the loop
            //(*positions)[outId] = p1.getPosition().x;
            //(*colors)[outId] = p1.getColor();
            //++outId;
        } else {
            // Add a point with zero difference
            (*positions)[outId] = 0.f;
            (*colors)[outId] = vec4(0.f);
            ++outId;
        }
        
while (id < static_cast<int>(transferFunction_.get().size()) || prevId < static_cast<int>(prevTransferFunction_.size())
               || outId < positions->getSize() ) {
            // Only store point if difference is non-zero
            // And the opacity is greater than zero
            if ((glm::any(glm::epsilonNotEqual(p1.getColor(), vec4(0), TFPointEpsilon_.get())) ||
                 glm::any(glm::epsilonNotEqual(p2.getColor(), vec4(0), TFPointEpsilon_.get()))) &&
                (p1.getAlpha() > 0. || p2.getAlpha() > 0.)) {
                if (outId == 1) {
                    // Add point if all previous have been equal.
                    (*positions)[outId] = static_cast<float>(p1.getPosition());
                    (*colors)[outId] = p1.getColor();
                    ++outId;
                }
                (*positions)[outId] = static_cast<float>(p2.getPosition());
                (*colors)[outId] = p2.getColor();
                ++outId;
            }
            // Advance to next point
            const auto a1 = transferFunction_.get().get(
                                                        std::min(id, static_cast<int>(transferFunction_.get().size()) - 1));
            TFPrimitive a2;
            if (id + 1 < static_cast<int>(transferFunction_.get().size()) - 1) {
                a2 = transferFunction_.get().get(id + 1);
            } else {
                const auto a = transferFunction_.get().get(transferFunction_.get().size() - 1);
                a2 = TFPrimitive(1., a.getColor());
            }
            const auto b1 = prevTransferFunction_.get(
                                                      std::min(prevId, static_cast<int>(prevTransferFunction_.size()) - 1));
            TFPrimitive b2;
            if (prevId + 1 < static_cast<int>(prevTransferFunction_.size()) - 1) {
                b2 = prevTransferFunction_.get(prevId + 1);
            } else {
                const auto b = prevTransferFunction_.get(prevTransferFunction_.size() - 1);
                b2 = TFPrimitive(1., b.getColor());
            }
            p1 = p2;
            if (a2 < b2) {
                // Interpolate point in prev TF
                auto p = mix(b1, b2, a2);
                p2 = TFPrimitive(a2.getPosition(), tfPointColorDiff(a2.getColor(), p.getColor()));
                ++id;
                
            } else if (b2 < a2) {
                auto p = mix(a1, a2, b2);
                p2 = TFPrimitive(b2.getPosition(), tfPointColorDiff(b2.getColor(), p.getColor()));
                ++prevId;
            } else {
                p2 =
                TFPrimitive(a2.getAlpha() < b2.getAlpha() ? b2.getPosition() : a2.getPosition(),
                            tfPointColorDiff(a2.getColor(), b2.getColor()));
                ++id;
                ++prevId;
            }
        }
        if (p2.getPosition() < 1. && p2.getAlpha() > 0.) {
            (*positions)[outId] = static_cast<float>(p2.getPosition());
            (*colors)[outId] = p2.getColor();
            ++outId;
        }
        if ((*positions)[outId - 1] < 1.) {
            // Add a point with zero difference
            (*positions)[outId] = 1.;
            (*colors)[outId] = vec4(0.f);
            ++outId;
        }
        tfPointImportanceSize_ = outId;
    }
}

inviwo::vec4 MinMaxUniformGrid3DImportanceCLProcessor::tfPointColorDiff(const vec4 &p1,
                                                                        const vec4 &p2) {
    return glm::abs(p2 * (useAssociatedColor_ ? p2.w : 1.f) -
                    p1 * (useAssociatedColor_ ? p1.w : 1.f));
}

void MinMaxUniformGrid3DImportanceCLProcessor::setInvalidationReason(
                                                                     InvalidationReason invalidationFlag) {
    invalidationFlag_ |= invalidationFlag;
}

TFPrimitive MinMaxUniformGrid3DImportanceCLProcessor::mix(const TFPrimitive &a,
                                                          const TFPrimitive &b,
                                                          const TFPrimitive &t) {
    return mix(a, b, (t.getPosition() - a.getPosition()) / (b.getPosition() - a.getPosition()));
}

inviwo::TFPrimitive MinMaxUniformGrid3DImportanceCLProcessor::mix(const TFPrimitive &a,
                                                                  const TFPrimitive &b, double t) {
    return TFPrimitive(glm::mix(a.getPosition(), b.getPosition(), t),
                       glm::mix(a.getColor(), b.getColor(), t));
}

}  // namespace inviwo
