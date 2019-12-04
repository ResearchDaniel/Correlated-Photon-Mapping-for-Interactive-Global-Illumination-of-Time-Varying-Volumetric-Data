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

#include <modules/progressivephotonmapping/processor/progressivephotontracercl.h>

#include <modules/opencl/buffer/bufferclgl.h>
#include <modules/opencl/inviwoopencl.h>
#include <modules/opencl/syncclgl.h>
#include <modules/opencl/image/imagecl.h>
#include <modules/opencl/image/imageclgl.h>
#include <modules/opencl/volume/volumeclgl.h>
#include <modules/opencl/volume/volumecl.h>
#include <inviwo/core/datastructures/volume/volumeram.h>
#include <modules/opencl/kernelmanager.h>

#include <modules/lightcl/lightsourcescl.h>

#include <modules/rndgenmwc64x/mwc64xseedgenerator.h>

#include <modules/opengl/clockgl.h>
#include <modules/opengl/texture/textureutils.h>
#include <modules/opengl/buffer/buffergl.h>
#include <modules/opengl/texture/textureunit.h>

#include <modules/radixsortcl/processors/radixsortcl.h>
#ifdef IVW_PROFILING
#define IVW_DETAILED_PROFILING
#endif

namespace inviwo {

const ProcessorInfo ProgressivePhotonTracerCL::processorInfo_{
    "org.inviwo.ProgressivePhotonTracerCL",  // Class identifier
    "ProgressivePhotonTracer",                 // Display name
    "Photons",                                 // Category
    CodeState::Experimental,                   // Code state
    Tags::CL,                                  // Tags
};
const ProcessorInfo ProgressivePhotonTracerCL::getProcessorInfo() const {
    return processorInfo_;
}

ProgressivePhotonTracerCL::ProgressivePhotonTracerCL()
: Processor(), KernelObserver(), KernelOwner()
, volumePort_("volume")
, recomputationImportanceGrid_("recomputationImportance")
, lightSamples_("LightSamples")
, outport_("photons")
, recomputedIndicesPort_("recomputedIndices")
, samplingRate_("samplingRate", "Sampling rate", 1.0f, 1.0f, 15.0f)
, radius_("radius", "Photon radius (# voxels)", 1.f, 0.00001f, 200.f)
, sceneRadianceScaling_("radianceScale", "Scene radiance scale", 1.f, 0.01f, 100.f)
, camera_("camera", "Camera", vec3(0.0f, 0.0f, -2.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), nullptr, InvalidationLevel::Valid)
, maxIncrementalPhotonsToUpdate_("maxIncrementalPhotonsToUpdate", "Max photons per update (%)", 100.f, 0.f, 100.f)
, equalIncrementalImportance_("equalImportance", "Equal importance", false)
, spatialSorting_("spatialSorting", "Spatial sorting", true)
, maxScatteringEvents_("maxScatteringEvents", "Max scattering events", 1, 1, 16)
, noSingleScattering_("noSingleScattering", "No single scattering", false)
// Material properties
, transferFunction_("transferFunction", "Transfer function", TransferFunction())
, advancedMaterial_("material", "Material")
, alphaProp_("alpha", "Progressive alpha", 0.5f, 0.0001f, 1.f)
, workGroupSize_("wgsize", "Work group size", ivec2(8, 8), ivec2(0), ivec2(256))
, useGLSharing_("glsharing", "Use OpenGL sharing", true)
, invalidateRendering_("invalidate", "Invalidate rendering")
, enableProgressiveRefinement_("enableRefinement", "Progressive refinement", false)
, enableProgressivePhotonRecomputation_("enableProgressiveRecomputation", "Progressive recomputation", true)
, clipX_("clipX", "Clip X Slices", 0, 256, 0, 256)
, clipY_("clipY", "Clip Y Slices", 0, 256, 0, 256)
, clipZ_("clipZ", "Clip Z Slices", 0, 256, 0, 256)
, photonData_(std::make_shared<PhotonData>())
, axisAlignedBoundingBoxCL_(8, DataFloat32::get(), BufferUsage::Static, nullptr, CL_MEM_READ_ONLY)
, photonTracer_(workGroupSize_.get(), useGLSharing_)
, progressiveTimer_(Timer::Milliseconds(100), std::bind(&ProgressivePhotonTracerCL::onTimerEvent, this))
, recomputedPhotonIndices_(std::make_shared< RecomputedPhotonIndices >())
{
    addPort(volumePort_);
    volumePort_.onChange([this]() {
        invalidateProgressiveRendering(PhotonData::InvalidationReason::Volume); }
                         );
    addPort(recomputationImportanceGrid_);
    recomputationImportanceGrid_.setOptional(true);
    recomputationImportanceGrid_.onConnect([this]() {
        invalidateProgressiveRendering(PhotonData::InvalidationReason::All); }
                                           );
    
    addPort(lightSamples_);
    lightSamples_.onChange([this]() {
        for (auto lightSourceSample = lightSamples_.begin(), end = lightSamples_.end(); lightSourceSample != end; ++lightSourceSample) {
            if (lightSourceSample->isReset()) {
                invalidateProgressiveRendering(PhotonData::InvalidationReason::Light);
            }
        }
    });
    
    addPort(outport_);
    addPort(recomputedIndicesPort_);
    
    
    
    
    volumePort_.onChange([this] () {
        invalidateProgressiveRendering(PhotonData::InvalidationReason::Volume);
    }
                         );
    //lightSamples_.onChange([this]{ invalidateProgressiveRendering(PhotonData::InvalidationReason::Light); });
    
    addProperty(samplingRate_);
    samplingRate_.onChange(this, &ProgressivePhotonTracerCL::kernelArgChanged);
    addProperty(radius_);
    radius_.onChange([this]{ invalidateProgressiveRendering(PhotonData::InvalidationReason::All); });
    addProperty(maxScatteringEvents_);
    addProperty(noSingleScattering_);
    noSingleScattering_.onChange(this, &ProgressivePhotonTracerCL::noSingleScatteringChanged);
    addProperty(alphaProp_);
    //transferFunction_.setGroupID(advancedMaterial_.getGroupId());
    addProperty(advancedMaterial_);
    addProperty(transferFunction_);
    transferFunction_.onChange([this]{ invalidateProgressiveRendering(PhotonData::InvalidationReason::TransferFunction); });
    advancedMaterial_.phaseFunctionProp.onChange(this, &ProgressivePhotonTracerCL::phaseFunctionChanged);
    // Need to override these to invalidate progressive rendering
    advancedMaterial_.indexOfRefractionProp.onChange(this, &ProgressivePhotonTracerCL::kernelArgChanged);
    advancedMaterial_.roughnessProp.onChange(this, &ProgressivePhotonTracerCL::kernelArgChanged);
    advancedMaterial_.specularColorProp.onChange(this, &ProgressivePhotonTracerCL::kernelArgChanged);
    advancedMaterial_.anisotropyProp.onChange(this, &ProgressivePhotonTracerCL::kernelArgChanged);
    alphaProp_.onChange(this, &ProgressivePhotonTracerCL::kernelArgChanged);
    
    addProperty(workGroupSize_);
    workGroupSize_.onChange([this]() { photonTracer_.workGroupSize(workGroupSize_.get()); });
    addProperty(useGLSharing_);
    useGLSharing_.onChange([this]() { photonTracer_.useGLSharing(useGLSharing_); });
    addProperty(camera_);
    camera_.onChange([this]() {
        //if (enableProgressiveRefinement_) {
        invalidateProgressiveRendering(PhotonData::InvalidationReason::Camera);
        photonData_->setIteration(1);
        //}
    });
    addProperty(maxIncrementalPhotonsToUpdate_);
    addProperty(equalIncrementalImportance_);
    equalIncrementalImportance_.onChange([this]() { photonRecomputationDetector_.setEqualImportance(equalIncrementalImportance_.get()); });
    addProperty(spatialSorting_);
    addProperty(invalidateRendering_);
    addProperty(enableProgressiveRefinement_);
    addProperty(enableProgressivePhotonRecomputation_);
    
    addProperty(clipX_);
    addProperty(clipY_);
    addProperty(clipZ_);
    clipX_.setVisible(false);
    clipY_.setVisible(false);
    clipZ_.setVisible(false);
    clipX_.onChange(this, &ProgressivePhotonTracerCL::onClipChange);
    clipY_.onChange(this, &ProgressivePhotonTracerCL::onClipChange);
    clipZ_.onChange(this, &ProgressivePhotonTracerCL::onClipChange);
    
    photonTracer_.addObserver(this);
    
    enableProgressiveRefinement_.onChange(this, &ProgressivePhotonTracerCL::progressiveRefinementChanged);
    
    
    // Get bounding geometry
    vec4 aabb[2];
    aabb[0] = vec4(0.f);
    aabb[1] = vec4(1.f);
    axisAlignedBoundingBoxCL_.upload(aabb, sizeof(aabb));
    progressiveRefinementChanged();
    
    indexToBuffer_ = addKernel("indextobuffer.cl", "indexToBufferKernel");
    thresholdKernel_ = addKernel("threshold.cl", "thresholdKernel");
    lightSampleHashKernel_ = addKernel("hashlightsample.cl", "hashLightSampleKernel");
    
    auto problem = clogs::ReduceProblem();
    problem.setType(clogs::TYPE_INT);
    reduce_ = std::unique_ptr<clogs::Reduce>(new clogs::Reduce(OpenCL::getPtr()->getContext(), OpenCL::getPtr()->getDevice(), problem));
    recomputationImportanceSorter_ = std::unique_ptr<clogs::Radixsort>(new clogs::Radixsort(OpenCL::getPtr()->getContext(), OpenCL::getPtr()->getDevice(),
                                                                                            dataFormatToClogsType(photonRecomputationImportance_.getDataFormat()), dataFormatToClogsType(recomputedPhotonIndices_->indicesToRecomputedPhotons.getDataFormat())));
    
    // Spatially sort indices. I.e. sorting by index is equivalent to sorting spatially.
#ifdef HASH_SORT_PHOTONS
    recomputationIndexSorter_ = std::unique_ptr<clogs::Radixsort>(new clogs::Radixsort(OpenCL::getPtr()->getContext(), OpenCL::getPtr()->getDevice(),
                                                                                       dataFormatToClogsType(recomputedPhotonIndices_->indicesToRecomputedPhotons.getDataFormat()),
                                                                                       dataFormatToClogsType(recomputedPhotonIndices_->indicesToRecomputedPhotons.getDataFormat())));
#else
    recomputationIndexSorter_ = std::unique_ptr<clogs::Radixsort>(new clogs::Radixsort(OpenCL::getPtr()->getContext(), OpenCL::getPtr()->getDevice(),
                                                                                       dataFormatToClogsType(recomputedPhotonIndices_->indicesToRecomputedPhotons.getDataFormat())));
#endif
}

void ProgressivePhotonTracerCL::process() {
    if (!photonTracer_.isValid()) {
        return;
    }
    size_t nPhotons = 0;
    for (auto lightSourceSample = lightSamples_.begin(), end = lightSamples_.end(); lightSourceSample != end; ++lightSourceSample) {
        nPhotons += lightSourceSample->getSize();
    }
    if (nPhotons != photonData_->getNumberOfPhotons() || maxScatteringEvents_ != photonData_->getMaxPhotonInteractions()) {
        photonData_->setSize(nPhotons, maxScatteringEvents_);
        invalidateProgressiveRendering(PhotonData::InvalidationReason::All);
        
    }
    const Volume* volume = volumePort_.getData().get();
    auto volumeDim = volume->getDimensions();
    float sceneRadius = getSceneRadius();
    // Texture space spacing
    const mat4& textureToIndexMatrix = volumePort_.getData()->getCoordinateTransformer().getTextureToIndexMatrix();
    vec3 voxelSpacing(1.f/glm::length(textureToIndexMatrix[0]), 1.f/glm::length(textureToIndexMatrix[1]), 1.f/glm::length(textureToIndexMatrix[2]));
    
    float stepSize = samplingRate_.get()*std::min(voxelSpacing.x, std::min(voxelSpacing.y, voxelSpacing.z));
    
    
    int maxInteractions = maxScatteringEvents_.get();
    int batch = 0;
    if (static_cast<int>(invalidationFlag_) == 0 ||
        static_cast<int>(invalidationFlag_) &
        (static_cast<int>(PhotonData::InvalidationReason::Light) |
         static_cast<int>(PhotonData::InvalidationReason::Camera) |
         static_cast<int>(PhotonData::InvalidationReason::TransferFunction) |
         static_cast<int>(PhotonData::InvalidationReason::Volume))) {
            photonData_->resetIteration();
        }
    if (photonData_->iteration() == 0) {
        vec4 radiusInTextureSpace = volume->getCoordinateTransformer().getIndexToTextureMatrix()*vec4(vec3(radius_.get()), 0.f);
        float radius = glm::length(vec3(radiusInTextureSpace));
        //radius = photonRadiusScaling+0.001f*(radius_.get()-1.f);
        photonData_->setRadius(radius, sceneRadius); // % of scene size
        photonData_->setIteration(1);
    } else {
        photonData_->advanceToNextIteration(alphaProp_);
    }
    
    std::vector< std::vector<cl::Event> > clEvents;
    // Number of photons to compute this iteration
    auto nPhotonsToCompute = photonData_->getNumberOfPhotons();
    if (!(static_cast<int>(invalidationFlag_) & static_cast<int>(PhotonData::InvalidationReason::Light)) && recomputationImportanceGrid_.isReady() && photonRecomputationDetector_.isValid()) {
        //IVW_CPU_PROFILING("recomputation")
        // Compute update priority and only update changed photons
        
        if (photonRecomputationImportance_.getSize() != photonData_->getNumberOfPhotons()) {
            photonRecomputationImportance_.setSize(photonData_->getNumberOfPhotons());
            resetPhotonImportance(0, photonRecomputationImportance_.getSize());
        }
        if (recomputedPhotonIndices_->indicesToRecomputedPhotons.getSize() != photonData_->getNumberOfPhotons()) {
            recomputedPhotonIndices_->indicesToRecomputedPhotons.setSize(photonData_->getNumberOfPhotons());
            thresholdPhotonRecomputation_.setSize(photonData_->getNumberOfPhotons());
            photonRecomputationHashed_.setSize(photonData_->getNumberOfPhotons());
        }
        
        SyncCLGL glSync(OpenCL::getPtr()->getContext(), OpenCL::getPtr()->getQueue());
        auto indicesToRecomputedPhotonsCL = recomputedPhotonIndices_->indicesToRecomputedPhotons.getEditableRepresentation<BufferCLGL>();
        // Recompute photons based on importance if Transfer function or volume data changed
        if ((static_cast<int>(invalidationFlag_) & (static_cast<int>(PhotonData::InvalidationReason::TransferFunction) | static_cast<int>(PhotonData::InvalidationReason::Volume)))) {
            int offset = 0;
            //IVW_CPU_PROFILING("recomputation")
            auto recomputationImportanceGrid = dynamic_cast<const ImportanceUniformGrid3D*>(recomputationImportanceGrid_.getData().get());
            if (!recomputationImportanceGrid) {
                LogError("UniformGrid3DInport require ImportanceUniformGrid3D as input");
                return;
            }
            
            //{
            //    IVW_CPU_PROFILING("glFinish")
            //        glFinish();
            //}
            photonRecomputationDetector_.setPercentage(static_cast<int>(maxIncrementalPhotonsToUpdate_.get()));
            photonRecomputationDetector_.setIteration(photonRecomputationDetector_.getIteration() + 1);
            clEvents.emplace_back(std::vector<cl::Event>(1));
            for (auto lightSourceSample = lightSamples_.begin(), end = lightSamples_.end();
                 lightSourceSample != end;
                 ++lightSourceSample) {
                //IVW_CPU_PROFILING("photonRecomputationImportance")
                
                
                if (offset > 0) {
                    clEvents.back().emplace_back(cl::Event());
                }
                cl::Event* event = &clEvents.back().back();;
                photonRecomputationDetector_.photonRecomputationImportance(photonData_.get(), offset, volume, recomputationImportanceGrid, *(*lightSourceSample).get(), photonRecomputationImportance_, nullptr, event, &glSync);
                
                offset += static_cast<int>(lightSourceSample->getSize());
            }
            
            
            
            //{ // Scope since photon tracing also synchronizes with GL
            
            auto nElements = recomputedPhotonIndices_->indicesToRecomputedPhotons.getSize();
            auto workGroupSize = 128;
            size_t globalWorkSizeX = getGlobalWorkGroupSize(nElements, workGroupSize);
            
            auto photonImportanceCL = photonRecomputationImportance_.getEditableRepresentation<BufferCL>();
            glSync.addToAquireGLObjectList(indicesToRecomputedPhotonsCL);
            glSync.aquireAllObjects();
            
            //{IVW_CPU_PROFILING("thresholdKernel")
            // Threshold, all photons with importance > 0 will be marked as invalid.
            thresholdKernel_->setArg(0, *photonImportanceCL);
            // Note: Must use inverse threshold since importance is reversed to enable sorting
            //unsigned char threshold = 255;
            const unsigned int threshold = 2147483647u;
            //const float threshold = ldexp(0x1fffffe, 127 - 4 * 6);
            auto thresholdedCL = thresholdPhotonRecomputation_.getEditableRepresentation<BufferCL>();
            thresholdKernel_->setArg(1, threshold);
            thresholdKernel_->setArg(2, static_cast<int>(thresholdPhotonRecomputation_.getSize()));
            thresholdKernel_->setArg(3, *thresholdedCL);
            clEvents.emplace_back(std::vector<cl::Event>(1));
            OpenCL::getPtr()->getAsyncQueue().enqueueNDRangeKernel(*thresholdKernel_, cl::NullRange, globalWorkSizeX, workGroupSize,
                                                                   &clEvents[clEvents.size() - 2], &clEvents.back()[0]);
            //}
            //auto thresholdedCL = thresholdPhotonRecomputation_.getEditableRepresentation<BufferCL>();
            clEvents.emplace_back(std::vector<cl::Event>(2));
            bool blocking = false;
            auto nPhotonsToRecompute = 0;
            //{IVW_CPU_PROFILING("CPU reduce")
            nPhotonsToRecompute = reduceInts(thresholdedCL, thresholdPhotonRecomputation_.getSize(), blocking, &clEvents[clEvents.size() - 2], &clEvents.back()[0], &clEvents.back()[1]);
            //}
            
            // Indexing and sorting can be performed at the same time
            // as thresholding and reduction
            //{IVW_CPU_PROFILING("indexToBuffer_")
            // Reset indices in buffer. I.e. write 0,1,2... at corresponding locations.
            indexToBuffer_->setArg(0, *indicesToRecomputedPhotonsCL);
            indexToBuffer_->setArg(1, static_cast<int>(nElements));
            clEvents.emplace_back(std::vector<cl::Event>(1));
            OpenCL::getPtr()->getQueue().enqueueNDRangeKernel(*indexToBuffer_, cl::NullRange, globalWorkSizeX,
                                                              workGroupSize, &clEvents[clEvents.size() - 4], &clEvents.back()[0]);
            //}
            // Sort indices by importance
            //{IVW_CPU_PROFILING("sortIndicesByImportance")
            clEvents.emplace_back(std::vector<cl::Event>(1));
            sortIndicesByImportance(&photonRecomputationImportance_, photonImportanceCL,
                                    &recomputedPhotonIndices_->indicesToRecomputedPhotons, indicesToRecomputedPhotonsCL,
                                    &clEvents[clEvents.size() - 2], &clEvents.back()[0]);
            //}
            
            
            //copyIndicesEvent[0].wait();
            //LogInfo((remainingPhotonsToUpdate_ >= 0 ?
            //    100.f*static_cast<float>(remainingPhotonsToUpdate_) / static_cast<float>(photonData_->getNumberOfPhotons()) : 0)
            //    << " % invalid photons")
            
            glSync.releaseAllGLObjects(&clEvents.back());
            // Wait for computation of number of invalid photons (reduction)
            clEvents[clEvents.size() - 3].back().wait();
            //auto elapsedTime = std::accumulate(std::begin(clEvents[clEvents.size() - 3]), std::end(clEvents[clEvents.size() - 3]), 0.f,
            //    [](float val, const cl::Event& a) {
            //    return val + a.getElapsedTime(); });
            //LogInfo("Reduce time: " << elapsedTime);
            remainingPhotonsOffset_ = 0;
            if (remainingPhotonsToUpdate_ < 0 || nPhotonsToRecompute > 0) {
                remainingPhotonsToUpdate_ = nPhotonsToRecompute;
            }
            
        }
        //}
        
        int maxPhotonsToUpdate = static_cast<int>((maxIncrementalPhotonsToUpdate_ / 100.f)*photonData_->getNumberOfPhotons());
        nPhotonsToCompute = std::min(remainingPhotonsToUpdate_, maxPhotonsToUpdate);
        if (remainingPhotonsOffset_ > 0) {
            //IVW_CPU_PROFILING("Move photons")
            // Move photons
            //SyncCLGL glSync;
            //glSync.releaseAllGLObjects();
            //auto photonsToRecomputeIndicesCL = recomputedPhotonIndices_->indicesToRecomputedPhotons.getEditableRepresentation<ElementBufferCLGL>();
            glSync.addToAquireGLObjectList(indicesToRecomputedPhotonsCL);
            glSync.aquireAllObjects();
            // Make sure that copying is not overlapping
            size_t itemsLeftToCopy = nPhotonsToCompute*indicesToRecomputedPhotonsCL->getSizeOfElement();
            size_t scrOffset = remainingPhotonsOffset_*indicesToRecomputedPhotonsCL->getSizeOfElement();
            size_t dstOffset = 0;
            while (itemsLeftToCopy > 0) {
                
                clEvents.emplace_back(std::vector<cl::Event>(1));
                std::vector<cl::Event>* waitForEvents = nullptr;
                if (clEvents.size() > 1) {
                    waitForEvents = &clEvents[clEvents.size() - 2];
                }
                size_t srcOffset = dstOffset + remainingPhotonsOffset_*indicesToRecomputedPhotonsCL->getSizeOfElement();
                size_t nElementsToCopy = std::min(itemsLeftToCopy, scrOffset);
                OpenCL::getPtr()->getQueue().enqueueCopyBuffer(indicesToRecomputedPhotonsCL->get(),
                                                               indicesToRecomputedPhotonsCL->get(), srcOffset, dstOffset,
                                                               nElementsToCopy, waitForEvents, &clEvents.back()[0]);
                // Copied N items,
                itemsLeftToCopy -= nElementsToCopy;
                srcOffset += nElementsToCopy;
                dstOffset += nElementsToCopy;
            }
            glSync.releaseAllGLObjects(clEvents.size() > 0 ? &clEvents.back() : nullptr);
        }
        
        recomputedPhotonIndices_->nRecomputedPhotons = static_cast<int>(nPhotonsToCompute);
        
        
        if (recomputedPhotonIndices_->nRecomputedPhotons > 0) {
            if (spatialSorting_) {
                //SyncCLGL glSync;
                //auto photonsToRecomputeIndicesCL = recomputedPhotonIndices_->indicesToRecomputedPhotons.getEditableRepresentation<ElementBufferCLGL>();
                glSync.addToAquireGLObjectList(indicesToRecomputedPhotonsCL);
                glSync.aquireAllObjects();
                //IVW_CPU_PROFILING("sortIndices")
#ifdef HASH_SORT_PHOTONS
                // Hash light samples for sorting
                int offset = 0;
                auto hashedSamplesCL = photonRecomputationHashed_.getEditableRepresentation<BufferCL>();
                auto cellSize = recomputationImportanceGrid->getDimensions();
                auto nBlocks = recomputationImportanceGrid->getDimensions();
                for (auto lightSourceSample = lightSamples_.begin(), end = lightSamples_.end();
                     lightSourceSample != end;
                     ++lightSourceSample) {
                    SyncCLGL glSync;
                    clEvents.emplace_back(std::vector<cl::Event>(1));
                    auto lightSampleCL = lightSourceSample->getLightSamples()->getRepresentation<BufferCLGL>();
                    auto intersectionPointCL = lightSourceSample->getIntersectionPoints()->getRepresentation<BufferCLGL>();
                    glSync.addToAquireGLObjectList(lightSampleCL);
                    glSync.addToAquireGLObjectList(intersectionPointCL);
                    glSync.aquireAllObjects();
                    lightSampleHashKernel_->setArg(0, *lightSampleCL);
                    lightSampleHashKernel_->setArg(1, *intersectionPointCL);
                    lightSampleHashKernel_->setArg(2, static_cast<int>(lightSourceSample->getSize()));
                    lightSampleHashKernel_->setArg(3, *indicesToRecomputedPhotonsCL);
                    lightSampleHashKernel_->setArg(4, static_cast<int>(remainingPhotonsToUpdate_));
                    lightSampleHashKernel_->setArg(5, vec3(cellSize));
                    lightSampleHashKernel_->setArg(6, ivec3(nBlocks));
                    lightSampleHashKernel_->setArg(7, *hashedSamplesCL);
                    lightSampleHashKernel_->setArg(8, offset);
                    
                    size_t workGroupSize = 128;
                    size_t globalWorkGroupSize(getGlobalWorkGroupSize(remainingPhotonsToUpdate_, workGroupSize));
                    OpenCL::getPtr()->getQueue().enqueueNDRangeKernel(*lightSampleHashKernel_, cl::NullRange, globalWorkGroupSize, workGroupSize,
                                                                      &clEvents[clEvents.size() - 2], &clEvents.back()[0]);
                    
                    offset += static_cast<int>(lightSourceSample->getSize());
                }
                clEvents.emplace_back(std::vector<cl::Event>(1));
                sortIndices(&photonRecomputationHashed_, photonRecomputationHashed_.getEditableRepresentation<BufferCL>(),
                            &recomputedPhotonIndices_->indicesToRecomputedPhotons, indicesToRecomputedPhotonsCL, remainingPhotonsToUpdate_, &clEvents[clEvents.size() - 2], &clEvents.back()[0]);
#else
                // Sorting on index seem to give same performance as spatial hashing
                clEvents.emplace_back(std::vector<cl::Event>(1));
                sortIndices(
                            &recomputedPhotonIndices_->indicesToRecomputedPhotons, indicesToRecomputedPhotonsCL,
                            &photonRecomputationHashed_, photonRecomputationHashed_.getEditableRepresentation<BufferCL>(), recomputedPhotonIndices_->nRecomputedPhotons, &clEvents[clEvents.size() - 2], &clEvents.back()[0]);
#endif
                glSync.releaseAllGLObjects(&clEvents.back());
            }
            //IVW_OPENCL_PROFILING(tracingProfilingEvent, "Tracing")
            //
            
            int offset = 0;
            
            for (auto lightSourceSample = lightSamples_.begin(), end = lightSamples_.end(); lightSourceSample != end; ++lightSourceSample) {
                
                clEvents.emplace_back(std::vector<cl::Event>(1));
                // Only wait first iteration
                std::vector<cl::Event>* waitForRecomputationDetection = nullptr;
                if (clEvents.size() > 1) {
                    waitForRecomputationDetection = &clEvents[clEvents.size() - 2];
                }
                
                // Texture space spacing
                const mat4 volumeTextureToWorld = volume->getCoordinateTransformer().getTextureToWorldMatrix();
                
                auto volumeCL = volume->getRepresentation<VolumeCLGL>();
                auto lightSamplesCL = lightSourceSample->getLightSamples()->getRepresentation<BufferCLGL>();
                auto intersectionPointsCL = lightSourceSample->getIntersectionPoints()->getRepresentation<BufferCLGL>();
                auto photonCL = photonData_->photons_.getEditableRepresentation<BufferCLGL>();
                
                auto transferFunctionCL = transferFunction_.get().getData()->getRepresentation<LayerCLGL>();
                //const ElementBufferCLGL* photonsToRecomputeIndicesCL = recomputedPhotonIndices_->indicesToRecomputedPhotons.getRepresentation<ElementBufferCLGL>();;
                
                // Acquire shared representations before using them in OpenGL
                // The SyncCLGL object will take care of synchronization between OpenGL and OpenCL
                //{IVW_CPU_PROFILING("Aquire")
                glSync.addToAquireGLObjectList(volumeCL);
                glSync.addToAquireGLObjectList(lightSamplesCL);
                glSync.addToAquireGLObjectList(intersectionPointsCL);
                glSync.addToAquireGLObjectList(photonCL);
                glSync.addToAquireGLObjectList(transferFunctionCL);
                glSync.addToAquireGLObjectList(indicesToRecomputedPhotonsCL);
                glSync.aquireAllObjects();
                //}
                //{IVW_CPU_PROFILING("Tracing (CPU measurement)")
                photonTracer_.tracePhotons(photonData_.get(), volumeCL, volumeCL->getVolumeStruct(volume), &axisAlignedBoundingBoxCL_
                                           , transferFunctionCL, advancedMaterial_, stepSize, lightSamplesCL, intersectionPointsCL, lightSourceSample->getSize(), indicesToRecomputedPhotonsCL, recomputedPhotonIndices_->nRecomputedPhotons
                                           , photonCL, offset, batch, maxInteractions
                                           , waitForRecomputationDetection, &clEvents.back()[0]);
                //}
                
                
                glSync.releaseAllGLObjects(&clEvents.back());
                
                
                //photonTracer_.tracePhotons(volume, transferFunction_.get(), &axisAlignedBoundingBoxCL_,
                //    advancedMaterial_, &camera_.get(), stepSize, (*lightSourceSample).get(), &recomputedPhotonIndices_->indicesToRecomputedPhotons, recomputedPhotonIndices_->nRecomputedPhotons, offset, batch
                //    , maxInteractions, photonData_.get(), waitForRecomputationDetection, &clEvents.back()[0]);
                offset += static_cast<int>(lightSourceSample->getSize());
            }
            //clEvents.emplace_back(std::vector<cl::Event>(1));
            resetPhotonImportance(remainingPhotonsOffset_, nPhotonsToCompute);
            
        }
        
        
        remainingPhotonsOffset_ += nPhotonsToCompute;
        remainingPhotonsToUpdate_ -= static_cast<int>(nPhotonsToCompute);
        if (remainingPhotonsToUpdate_ > 0 && enableProgressivePhotonRecomputation_) {
            enableProgressiveRefinement_.set(true);
        } else {
            enableProgressiveRefinement_.set(false);
        }
    } else {
        auto offset = 0;
        for (auto lightSourceSample = lightSamples_.begin(), end = lightSamples_.end(); lightSourceSample != end; ++lightSourceSample) {
            clEvents.emplace_back(std::vector<cl::Event>(1));
            photonTracer_.tracePhotons(volume, transferFunction_.get(), &axisAlignedBoundingBoxCL_,
                                       advancedMaterial_, &camera_.get(), stepSize, (*lightSourceSample).get(), nullptr, 0, offset, batch
                                       , maxInteractions, photonData_.get(), nullptr, &clEvents.back()[0]);
            offset += static_cast<int>(lightSourceSample->getSize());
        }
        recomputedPhotonIndices_->nRecomputedPhotons = -1;
        // Will be withdrawn to zero at end of function
        remainingPhotonsToUpdate_ = 0;
        remainingPhotonsOffset_ = 0;
        
        if (photonRecomputationImportance_.getSize() > 0) {
            //IVW_CPU_PROFILING("FillBuffer")
            resetPhotonImportance(0, photonRecomputationImportance_.getSize());
        }
        
    }
    
#ifdef IVW_DETAILED_PROFILING
    if (!clEvents.empty()) {
        try {
            cl::Event* profilingEvent_ = &clEvents.back()[0];
            profilingEvent_->wait();
            std::string logMessage_("Photon tracing: ");
            std::string logSource_(parseTypeIdName(std::string(typeid(this).name())));
            std::stringstream performanceMessage;
            performanceMessage << logMessage_;
            for (auto it = std::begin(clEvents); it != std::end(clEvents); ++it) {
                auto elapsedTime = std::accumulate(std::begin(*it), std::end(*it), 0.f,
                                                   [](float val, const cl::Event& a) {
                                                       return val + a.getElapsedTime(); });
                if (it != std::begin(clEvents)) {
                    performanceMessage << " + ";
                }
                performanceMessage << elapsedTime;
                
            }
            auto elapsedTime = std::accumulate(std::begin(clEvents), std::end(clEvents), 0.f,
                                               [](float val, const std::vector<cl::Event>& a) {
                                                   return val + std::accumulate(std::begin(a), std::end(a), 0.f, [](float val, const cl::Event& a) { return val + a.getElapsedTime(); });
                                               });
            performanceMessage << " = " << elapsedTime << " ms";
            LogInfo(performanceMessage.str());
            //message << std::endl << " " <<
            std::stringstream infoMessage;
            infoMessage << "Computed photons: ";
            infoMessage << nPhotonsToCompute << " = " <<
            100.f*static_cast<float>(nPhotonsToCompute*photonData_->getMaxPhotonInteractions()) / static_cast<float>(photonData_->getNumberOfPhotons()*photonData_->getMaxPhotonInteractions())
            << " %";
            LogInfo(infoMessage.str());
        } catch (cl::Error& err) {
            LogError(getCLErrorString(err));
        }
    }
#endif
    
    
    recomputedIndicesPort_.setData(recomputedPhotonIndices_);
    photonData_->setInvalidationReason(invalidationFlag_);
    invalidationFlag_ = PhotonData::InvalidationReason(0);
    outport_.setData(photonData_);
}

void ProgressivePhotonTracerCL::resetPhotonImportance(size_t offset, size_t nPhotons, const VECTOR_CLASS<cl::Event> *waitForEvents, cl::Event* event) {
    auto photonImportanceCL = photonRecomputationImportance_.getEditableRepresentation<BufferCL>();
    // Reset importance for the photons that were computed
    OpenCL::getPtr()->getQueue().enqueueFillBuffer<unsigned int>(photonImportanceCL->getEditable(), 2147483647u, offset*photonImportanceCL->getSizeOfElement(), nPhotons*photonImportanceCL->getSizeOfElement(), waitForEvents, event);
}

void ProgressivePhotonTracerCL::onKernelCompiled(const cl::Kernel*) {
    invalidateProgressiveRendering(PhotonData::InvalidationReason::All);
    invalidate(InvalidationLevel::InvalidOutput);
}

void inviwo::ProgressivePhotonTracerCL::kernelArgChanged() {
    invalidateProgressiveRendering(PhotonData::InvalidationReason::All);
}

void inviwo::ProgressivePhotonTracerCL::onTimerEvent() {
    invalidationFlag_ |= PhotonData::InvalidationReason::Progressive;
    invalidateRendering_.pressButton();
    
}

void ProgressivePhotonTracerCL::noSingleScatteringChanged() {
    photonTracer_.setNoSingleScattering(noSingleScattering_.get());
}


void ProgressivePhotonTracerCL::invalidateProgressiveRendering(PhotonData::InvalidationReason invalidationFlag) {
    invalidationFlag_ |= invalidationFlag;
}

void ProgressivePhotonTracerCL::evaluateProgressiveRefinement() {
    invalidate(InvalidationLevel::InvalidOutput);
    invalidationFlag_ |= PhotonData::InvalidationReason::Progressive;
}

void ProgressivePhotonTracerCL::progressiveRefinementChanged() {
    photonTracer_.setProgressive(
                                 enableProgressiveRefinement_.get() &
                                 !recomputationImportanceGrid_.isConnected());
    if (enableProgressiveRefinement_.get()) {
        progressiveTimer_.start(Timer::Milliseconds(100));
    } else {
        progressiveTimer_.stop();
    }
}

void ProgressivePhotonTracerCL::phaseFunctionChanged()
{
    advancedMaterial_.phaseFunctionChanged();
    kernelArgChanged();
}

float ProgressivePhotonTracerCL::getSceneRadius() const
{
    auto volume = volumePort_.getData();
    float scale = 1.f;
    
    if (volume) {
        const mat4& volumeTextureToWorld = volume->getCoordinateTransformer().getTextureToWorldMatrix();
        vec3 worldSpaceExtent(glm::length(volumeTextureToWorld[0]), glm::length(volumeTextureToWorld[1]), glm::length(volumeTextureToWorld[2]));
        //scale = std::max(volumeScaling.x, std::max(volumeScaling.y, volumeScaling.z));
        scale = 0.5f*glm::length(worldSpaceExtent);
    }
    
    return scale;
}

void ProgressivePhotonTracerCL::onClipChange() {
    if (!volumePort_.isReady()) {
        return;
    }
    vec4 aabb[2];
    aabb[0] = vec4(static_cast<float>(clipX_.get().x), static_cast<float>(clipY_.get().x), static_cast<float>(clipZ_.get().x), 1.f);
    aabb[1] = vec4(static_cast<float>(clipX_.get().y), static_cast<float>(clipY_.get().y), static_cast<float>(clipZ_.get().y), 1.f);
    vec4 dims = vec4(volumePort_.getData()->getDimensions(), 1.f);
    aabb[0] /= dims;
    aabb[1] /= dims;
    axisAlignedBoundingBoxCL_.upload(&aabb, sizeof(aabb));
    invalidateProgressiveRendering(PhotonData::InvalidationReason::All);
}


void ProgressivePhotonTracerCL::sortIndicesByImportance(const BufferBase* keys, BufferCLBase* keysCL, const BufferBase* data, const BufferCLBase* dataCL, const VECTOR_CLASS<cl::Event> *waitForEvents, cl::Event *event) {
    try {
        if (sortKeysTempBufferSize_ < keys->getSize()*keys->getDataFormat()->getSize() || sortDataTempBufferSize_ < data->getSize()*data->getDataFormat()->getSize()) {
            recomputationImportanceSorter_->setTemporaryBuffers(cl::Buffer(OpenCL::getPtr()->getContext(), CL_MEM_READ_WRITE, keys->getSize()*keys->getDataFormat()->getSize(), NULL),
                                                                cl::Buffer(OpenCL::getPtr()->getContext(), CL_MEM_READ_WRITE, data->getSize()*data->getDataFormat()->getSize(), NULL));
            sortKeysTempBufferSize_ = keys->getSize()*keys->getDataFormat()->getSize();
            sortDataTempBufferSize_ = data->getSize()*data->getDataFormat()->getSize();
        }
        
        recomputationImportanceSorter_->enqueue(OpenCL::getPtr()->getQueue(), keysCL->get(), dataCL->get(), static_cast<unsigned int>(keys->getSize()), 0, waitForEvents, event);
    } catch (std::invalid_argument& e) {
        LogError(e.what());
    } catch (clogs::InternalError& e) {
        LogError(e.what());
    } catch (cl::Error& e) {
        LogError(errorCodeToString(e.err()));
    }
}

void ProgressivePhotonTracerCL::sortIndices(const BufferBase* keys, BufferCLBase* keysCL, const BufferBase* values, BufferCLBase* valuesCL, size_t nElements, const VECTOR_CLASS<cl::Event> *waitForEvents, cl::Event *event) {
    try {
        if (sortIndicesTempBufferSize_ < keys->getSize()*keys->getDataFormat()->getSize()) {
            recomputationIndexSorter_->setTemporaryBuffers(
                                                           cl::Buffer(OpenCL::getPtr()->getContext(), CL_MEM_READ_WRITE, keys->getSize()*keys->getDataFormat()->getSize(), NULL),
                                                           cl::Buffer(OpenCL::getPtr()->getContext(), CL_MEM_READ_WRITE, values->getSize()*values->getDataFormat()->getSize(), NULL));
            sortIndicesTempBufferSize_ = keys->getSize()*keys->getDataFormat()->getSize();
        }
        
        recomputationIndexSorter_->enqueue(OpenCL::getPtr()->getQueue(), keysCL->get(), valuesCL->get(), static_cast<unsigned int>(nElements), 0, waitForEvents, event);
    } catch (std::invalid_argument& e) {
        LogError(e.what());
    } catch (clogs::InternalError& e) {
        LogError(e.what());
    } catch (cl::Error& e) {
        LogError(errorCodeToString(e.err()));
    }
}

int ProgressivePhotonTracerCL::reduceInts(const BufferCLBase* dataCL, size_t nElements, bool blocking, const VECTOR_CLASS<cl::Event> *waitForEvents, cl::Event *readBackEvent /*= nullptr*/, cl::Event *reduceEvent /*= nullptr*/) {
    int result = 0;
    try {
        //IVW_CPU_PROFILING("reduceInts")
        reduce_->enqueue(OpenCL::getPtr()->getQueue(), blocking, dataCL->get(), &result, 0, nElements, waitForEvents, readBackEvent, reduceEvent);
        //OpenCL::getPtr()->getAsyncQueue().flush();
    } catch (std::invalid_argument& e) {
        LogError(e.what());
    } catch (clogs::InternalError& e) {
        LogError(e.what());
    } catch (cl::Error& e) {
        LogError(errorCodeToString(e.err()));
    }
    return result;
}




} // namespace
