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

#include "photontolightvolumeprocessorcl.h"
#include <modules/opencl/syncclgl.h>
#include <modules/opencl/buffer/buffercl.h>
#include <modules/opencl/buffer/bufferclgl.h>
#include <modules/opencl/volume/volumecl.h>
#include <modules/opencl/volume/volumeclgl.h>
#ifdef IVW_PROFILING
#define IVW_DETAILED_PROFILING
#endif
namespace inviwo {

// The Class Identifier has to be globally unique. Use a reverse DNS naming scheme
const ProcessorInfo PhotonToLightVolumeProcessorCL::processorInfo_{
    "org.inviwo.PhotonToLightVolumeProcessorCL",  // Class identifier
    "PhotonToLightVolumeProcessorCL",               // Display name
    "Photons",                                      // Category
    CodeState::Experimental,                        // Code state
    Tags::CL,                                       // Tags
};
const ProcessorInfo PhotonToLightVolumeProcessorCL::getProcessorInfo() const {
    return processorInfo_;
}

PhotonToLightVolumeProcessorCL::PhotonToLightVolumeProcessorCL()
: Processor()
, volumeInport_("volume")
, photons_("photons")
, recomputedPhotonIndicesPort_("recomputedPhotonIndices")
, outport_("lightvolume")
, incrementalRecomputationThreshold_("incrementalRecomputationThreshold", "Max % invalid photons to use add-remove", 50.f, 0.f, 100.f, 10.f)
, volumeSizeOption_("volumeSizeOption", "Light Volume Size")
, volumeDataTypeOption_("volumeDataType", "Output data type")
, information_("Information", "Light volume information")
//, camera_("camera", "Camera", vec3(0.0f, 0.0f, -2.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), nullptr, InvalidationLevel::Valid)
, alignChangedPhotons_("alignChangedPhotons", "Mem-align changed photons", false)
, workGroupSize_("wgsize", "Work group size", 128, 1, 2048)
, useGLSharing_("glsharing", "Use OpenGL sharing", true)
, kernelOwner_(this)
, kernel_(nullptr)
, splatSelectedPhotonsKernel_(nullptr)
, clearFloatsKernel_(nullptr)
, lightVolume_(std::make_shared<Volume>(size3_t(1), DataFloat32::get()))
{
    addPort(volumeInport_);
    addPort(photons_);
    recomputedPhotonIndicesPort_.setOptional(true);
    recomputedPhotonIndicesPort_.onDisconnect([this]() {
        prevPhotons_.setSize(0);
        changedAlignedPhotons_.setSize(0);
    });
    addPort(recomputedPhotonIndicesPort_);
    addPort(outport_);
    
    outport_.onDisconnect([this]() {
        prevPhotons_.setSize(0);
        changedAlignedPhotons_.setSize(0);
    });
    
    volumeInport_.onChange(std::bind(&PhotonToLightVolumeProcessorCL::volumeSizeOptionChanged, this));
    
    addProperty(incrementalRecomputationThreshold_);
    
    volumeSizeOption_.addOption("radius", "Photon radius", 0);
    volumeSizeOption_.addOption("1", "Full of incoming volume", 1);
    volumeSizeOption_.addOption("1/2", "Half of incoming volume", 2);
    volumeSizeOption_.addOption("1/4", "Quarter of incoming volume", 4);
    //volumeSizeOption_.addOption("0", "Custom", 0);
    volumeSizeOption_.setSelectedIndex(0);
    volumeSizeOption_.setCurrentStateAsDefault();
    volumeSizeOption_.onChange(this, &PhotonToLightVolumeProcessorCL::volumeSizeOptionChanged);
    
    //volumeDataTypeOption_.addOption("float16", "float16");
    volumeDataTypeOption_.addOption("float32", "float32");
    //volumeDataTypeOption_.addOption("4xfloat16", "4 x float16");
    volumeDataTypeOption_.addOption("4xfloat32", "4 x float32");
    volumeDataTypeOption_.setSelectedIndex(0);
    volumeDataTypeOption_.setCurrentStateAsDefault();
    volumeDataTypeOption_.onChange([this]() {
        
        if (volumeDataTypeOption_.getSelectedValue() == "float16") {
            lightVolume_ = std::make_shared<Volume>(lightVolume_->getDimensions(), DataFloat16::get());
        } else if (volumeDataTypeOption_.getSelectedValue() == "float32") {
            lightVolume_ = std::make_shared<Volume>(lightVolume_->getDimensions(), DataFloat32::get());
        } else if (volumeDataTypeOption_.getSelectedValue() == "4xfloat16") {
            lightVolume_ = std::make_shared<Volume>(lightVolume_->getDimensions(), DataVec4Float16::get());
        } else {
            lightVolume_ = std::make_shared<Volume>(lightVolume_->getDimensions(), DataVec4Float32::get());
        }
        information_.updateForNewVolume(*lightVolume_, false);
        buildKernel();
    });
    addProperty(volumeSizeOption_);
    addProperty(volumeDataTypeOption_);
    addProperty(information_);
    addProperty(alignChangedPhotons_);
    addProperty(workGroupSize_);
    addProperty(useGLSharing_);
    
    //addProperty(camera_);
    
    outport_.setData(lightVolume_);
    buildKernel();
}

void PhotonToLightVolumeProcessorCL::process() {
    
    if (kernel_ == NULL) {
        return;
    }
    auto photonData = photons_.getData();
    const Volume* volume = volumeInport_.getData().get();
    if (volumeSizeOption_.get() == 0) {
        // Determine size on photon radius
        auto invPhotonRadius = 1.0 / photonData->getRadiusRelativeToSceneSize();
        // Set dimensions depending on photon radius
        // Add one to each dimension to get rid of clamp functions in the photonTracerKernel
        //int maxDim = std::max(volumeDims.x, std::max(volumeDims.y, volumeDims.z));
        //photonMapSizeProp_.set(2+tgt::ivec3(static_cast<int>(static_cast<float>(maxDim)/radiusInVoxels)));
        
        //ivec3 hashTableDimensions(2 + ivec3(static_cast<int>(invPhotonRadius)));
        auto lightVolumeDimensions(size3_t(static_cast<size_t>(std::ceil(invPhotonRadius))));
        if (glm::any(glm::notEqual(lightVolume_->getDimensions(), lightVolumeDimensions))) {
            lightVolume_->setDimensions(lightVolumeDimensions);
            lightVolume_->setModelMatrix(volume->getModelMatrix());
            lightVolume_->setWorldMatrix(volume->getWorldMatrix());
            information_.updateForNewVolume(*lightVolume_, false);
            // Recompute all photons
            prevPhotons_.setSize(0);
            changedAlignedPhotons_.setSize(0);
        }
    }
    
    const size3_t outDim{ lightVolume_->getDimensions() };
    size_t outDimFlattened = outDim.x * outDim.y * outDim.z;
    if (tmpVolume_.getSize() != outDimFlattened*lightVolume_->getDataFormat()->getSize()) {
        
        tmpVolume_.setSize(outDimFlattened*lightVolume_->getDataFormat()->getSize());
    }
    size_t localWorkGroupSize(workGroupSize_.get());
    auto maxRecomputationPhotons = static_cast<int>(photonData->getNumberOfPhotons() * (incrementalRecomputationThreshold_ / incrementalRecomputationThreshold_.getMaxValue()));
    if (copyPrevPhotonsEvent_.size() > 0) {
        //IVW_CPU_PROFILING("Wait for copy prev photons (CPU)")
        // Wait for previous photons to be copied
        cl::WaitForEvents(copyPrevPhotonsEvent_);
        //#ifdef IVW_DETAILED_PROFILING
        //        LogInfo("Copy prev: " << copyPrevPhotonsEvent_[0].getElapsedTime() << " ms");
        //#endif
        copyPrevPhotonsEvent_.clear();
    }
    if (recomputedPhotonIndicesPort_.isReady() && prevPhotons_.getSize() == photonData->photons_.getSize() && recomputedPhotonIndicesPort_.getData()->nRecomputedPhotons > 0 && recomputedPhotonIndicesPort_.getData()->nRecomputedPhotons < maxRecomputationPhotons) {
        SyncCLGL glSync;
        auto recomputedPhotonIndices = recomputedPhotonIndicesPort_.getData();
        size_t globalWorkGroupSize(getGlobalWorkGroupSize(recomputedPhotonIndicesPort_.getData()->nRecomputedPhotons, localWorkGroupSize));
        auto volumeOutCL = lightVolume_->getEditableRepresentation<VolumeCLGL>();
        auto prevPhotonsCL = prevPhotons_.getRepresentation<BufferCL>();
        auto photonsCL = photonData->photons_.getRepresentation<BufferCLGL>();
        auto indicesCL = recomputedPhotonIndices->indicesToRecomputedPhotons.getRepresentation<BufferCLGL>();
        
        glSync.addToAquireGLObjectList(volumeOutCL);
        glSync.addToAquireGLObjectList(photonsCL);
        glSync.addToAquireGLObjectList(indicesCL);
        
        
        if (alignChangedPhotons_) {
            // Is it faster to copy the indexed photons
            // so that they are aligned when accessing?
            // 10 % faster on Geforce 670, but 1-2 % slower on 970
            const VolumeCLGL* volumeCL = volume->getRepresentation<VolumeCLGL>();
            glSync.addToAquireGLObjectList(volumeCL);
            glSync.aquireAllObjects();
            if (changedAlignedPhotons_.getSize() < maxRecomputationPhotons * 4) {
                changedAlignedPhotons_.setSize(maxRecomputationPhotons * 4);
            }
            auto alignedChangedPhotonsCL = changedAlignedPhotons_.getRepresentation<BufferCL>();
            std::vector<cl::Event> copyAlingedPhotonEvents(2);
            std::vector<cl::Event> splatAlingedPhotonEvents(2);
            std::vector<cl::Event>* waitForEvents = nullptr;
            int argIndex = 0;
            copyIndexPhotonsKernel_->setArg(argIndex++, *prevPhotonsCL);
            copyIndexPhotonsKernel_->setArg(argIndex++, *indicesCL);
            copyIndexPhotonsKernel_->setArg(argIndex++, static_cast<int>(recomputedPhotonIndices->nRecomputedPhotons));
            copyIndexPhotonsKernel_->setArg(argIndex++, -1.f);
            copyIndexPhotonsKernel_->setArg(argIndex++, static_cast<int>(photonData->getNumberOfPhotons()));
            copyIndexPhotonsKernel_->setArg(argIndex++, photonData->getMaxPhotonInteractions());
            copyIndexPhotonsKernel_->setArg(argIndex++, *alignedChangedPhotonsCL);
            copyIndexPhotonsKernel_->setArg(argIndex++, 0);
            OpenCL::getPtr()->getAsyncQueue().enqueueNDRangeKernel(
                                                                   *copyIndexPhotonsKernel_, cl::NullRange, globalWorkGroupSize, localWorkGroupSize, waitForEvents, &copyAlingedPhotonEvents[0]);
            
            copyIndexPhotonsKernel_->setArg(0, *photonsCL);
            copyIndexPhotonsKernel_->setArg(3, 1.f);
            copyIndexPhotonsKernel_->setArg(7, static_cast<int>(recomputedPhotonIndices->nRecomputedPhotons));
            OpenCL::getPtr()->getAsyncQueue().enqueueNDRangeKernel(
                                                                   *copyIndexPhotonsKernel_, cl::NullRange, globalWorkGroupSize, localWorkGroupSize, nullptr, &copyAlingedPhotonEvents[1]);
            
            
            
            size_t splatGlobalWorkGroupSize(getGlobalWorkGroupSize(2 * recomputedPhotonIndicesPort_.getData()->nRecomputedPhotons, localWorkGroupSize));
            executeVolumeOperation(volume, volumeCL, volumeOutCL, alignedChangedPhotonsCL, lightVolume_.get(), outDim,
                                   splatGlobalWorkGroupSize, localWorkGroupSize, &copyAlingedPhotonEvents, &splatAlingedPhotonEvents[0], &splatAlingedPhotonEvents[1]);
            
#ifdef IVW_DETAILED_PROFILING
            try {
                // Measure both computation and copy
                cl::WaitForEvents(splatAlingedPhotonEvents);
                float copy = 0;
                for (auto interaction = 0; interaction < copyAlingedPhotonEvents.size(); ++interaction) {
                    copy += copyAlingedPhotonEvents[interaction].getElapsedTime();
                }
                LogInfo("Exec time (copy, computation, copy): "
                        << copy << " + " << splatAlingedPhotonEvents[0].getElapsedTime() << " + " << splatAlingedPhotonEvents[1].getElapsedTime() << " = "
                        << copy + splatAlingedPhotonEvents[0].getElapsedTime() + splatAlingedPhotonEvents[1].getElapsedTime() << " ms");
            } catch (cl::Error& err) {
                LogError(getCLErrorString(err));
            }
#endif
        } else {
            glSync.aquireAllObjects();
            std::vector<cl::Event> removePhotonsEvents(1);
            std::vector<cl::Event> addPhotonsEvents(1);
            cl::Event copyEvent;
            // Remove old contribution
            photonsToLightVolume(volumeOutCL, prevPhotonsCL, indicesCL, *photonData, *recomputedPhotonIndices, -1.f, lightVolume_.get(), outDim,
                                 globalWorkGroupSize,
                                 localWorkGroupSize, nullptr, &removePhotonsEvents[0]);
            // Add new contribution
            photonsToLightVolume(volumeOutCL, photonsCL, indicesCL, *photonData, *recomputedPhotonIndices, 1.f, lightVolume_.get(), outDim,
                                 globalWorkGroupSize,
                                 localWorkGroupSize, &removePhotonsEvents, &addPhotonsEvents[0]);
            
            
            auto tmpVolumeCL = tmpVolume_.getRepresentation<BufferCL>();
            OpenCL::getPtr()->getQueue().enqueueCopyBufferToImage(
                                                                  tmpVolumeCL->get(), volumeOutCL->getEditable(), 0, size3_t(0), size3_t(outDim),
                                                                  &addPhotonsEvents, &copyEvent);
            
#ifdef IVW_DETAILED_PROFILING
            try {
                // Measure both computation and copy
                copyEvent.wait();
                float remove = 0, add = 0;
                for (auto interaction = 0; interaction < removePhotonsEvents.size(); ++interaction) {
                    remove += removePhotonsEvents[interaction].getElapsedTime();
                    add += addPhotonsEvents[interaction].getElapsedTime();
                }
                LogInfo("Exec time (remove, add, copy): "
                        << remove << " + " << add << " + " << copyEvent.getElapsedTime() << " = "
                        << remove + add + copyEvent.getElapsedTime() << " ms");
            } catch (cl::Error& err) {
                LogError(getCLErrorString(err));
            }
#endif
        }
    } else if (prevPhotons_.getSize() != photonData->photons_.getSize() || (recomputedPhotonIndicesPort_.getData()->nRecomputedPhotons < 0) || recomputedPhotonIndicesPort_.getData()->nRecomputedPhotons >= maxRecomputationPhotons) {
        cl::Event events[3];
        //
        // prevPhotons_.getSize() != photonData->photons_.getSize()
        size_t globalWorkGroupSize(getGlobalWorkGroupSize(photonData->getNumberOfPhotons()*photonData->getMaxPhotonInteractions(), localWorkGroupSize));
        
        BufferCL* tmpVolumeCL = tmpVolume_.getEditableRepresentation<BufferCL>();
        //size_t outDimFlattened = outDim.x * outDim.y * outDim.z;
        //clearBuffer(tmpVolumeCL, outDimFlattened, localWorkGroupSize, nullptr, &events[0]);
        OpenCL::getPtr()->getQueue().enqueueFillBuffer<float>(tmpVolumeCL->getEditable(), 0.f, 0, tmpVolume_.getSizeInBytes(), nullptr, &events[0
                                                                                                                                                ]);
        if (useGLSharing_.get()) {
            SyncCLGL glSync;
            const VolumeCLGL* volumeCL = volume->getRepresentation<VolumeCLGL>();
            VolumeCLGL* volumeOutCL = lightVolume_->getEditableRepresentation<VolumeCLGL>();
            const BufferCLGL* photonsCL = photonData->photons_.getRepresentation<BufferCLGL>();
            {
                
                glSync.addToAquireGLObjectList(volumeCL);
                glSync.addToAquireGLObjectList(volumeOutCL);
                glSync.addToAquireGLObjectList(photonsCL);
                glSync.aquireAllObjects();
                
                executeVolumeOperation(volume, volumeCL, volumeOutCL, photonsCL, lightVolume_.get(), outDim,
                                       globalWorkGroupSize,
                                       localWorkGroupSize, nullptr, &events[1], &events[2]);
            }
            
        } else {
            const VolumeCL* volumeCL = volume->getRepresentation<VolumeCL>();
            VolumeCL* volumeOutCL = lightVolume_->getEditableRepresentation<VolumeCL>();
            const BufferCL* photonsCL = photonData->photons_.getRepresentation<BufferCL>();
            BufferCL* tmpVolumeCL = tmpVolume_.getEditableRepresentation<BufferCL>();
            clearBuffer(tmpVolumeCL, outDimFlattened, localWorkGroupSize, nullptr, &events[0]);
            executeVolumeOperation(volume, volumeCL, volumeOutCL, photonsCL, lightVolume_.get(), outDim,
                                   globalWorkGroupSize,
                                   localWorkGroupSize, nullptr, &events[1], &events[2]);
        }
#ifdef IVW_DETAILED_PROFILING
        try {
            // Measure both computation and copy
            events[2].wait();
            LogInfo("Exec time (clear, computation, copy): "
                    << events[0].getElapsedTime() << " + " << events[1].getElapsedTime() << " + " << events[2].getElapsedTime() << " = "
                    << events[0].getElapsedTime() + events[1].getElapsedTime() + events[2].getElapsedTime() << " ms");
        } catch (cl::Error& err) {
            LogError(getCLErrorString(err));
        }
#endif
    }
    
    
    
    if (recomputedPhotonIndicesPort_.isReady() && recomputedPhotonIndicesPort_.getData()->nRecomputedPhotons != 0) {
        copyPrevPhotonsEvent_.emplace_back(cl::Event());
        if (prevPhotons_.getSize() != photonData->photons_.getSize()) {
            prevPhotons_.setSize(photonData->photons_.getSize());
        }
        if (useGLSharing_.get()) {
            //IVW_CPU_PROFILING("Copy prev photons (CPU)")
            auto photonsCL = photonData->photons_.getRepresentation<BufferCLGL>();
            auto prevPhotonsCL = prevPhotons_.getEditableRepresentation<BufferCL>();
            SyncCLGL glSync;
            glSync.addToAquireGLObjectList(photonsCL);
            glSync.aquireAllObjects();
            OpenCL::getPtr()->getAsyncQueue().enqueueCopyBuffer(
                                                                photonsCL->get(), prevPhotonsCL->getEditable(), 0, size_t(0), photonData->photons_.getSizeInBytes(), nullptr, &copyPrevPhotonsEvent_.back());
        } else {
            auto photonsCL = photonData->photons_.getRepresentation<BufferCL>();
            auto prevPhotonsCL = prevPhotons_.getEditableRepresentation<BufferCL>();
            OpenCL::getPtr()->getAsyncQueue().enqueueCopyBuffer(
                                                                photonsCL->get(), prevPhotonsCL->getEditable(), 0, size_t(0), photonData->photons_.getSizeInBytes(), nullptr, &copyPrevPhotonsEvent_.back());
        }
    }
    
}

void PhotonToLightVolumeProcessorCL::executeVolumeOperation(const Volume* volume,
                                                            const VolumeCLBase* volumeCL,
                                                            VolumeCLBase* volumeOutCL, const BufferCLBase* photonsCL, const Volume* volumeOut, const size3_t& outDim,
                                                            const size_t& globalWorkGroupSize,
                                                            const size_t& localWorkgroupSize, std::vector<cl::Event>* waitForEvents, cl::Event* splatEvent, cl::Event* copyEvent) {
    
    BufferCL* tmpVolumeCL = tmpVolume_.getEditableRepresentation<BufferCL>();
    
    try {
        
        int argIndex = 0;
        kernel_->setArg(argIndex++, *volumeCL);
        kernel_->setArg(argIndex++,
                        *(volumeCL->getVolumeStruct(volume)
                          .getRepresentation<BufferCL>()));  // Scaling for 12-bit data
        kernel_->setArg(argIndex++, *tmpVolumeCL);
        
        kernel_->setArg(argIndex++,
                        *(volumeOutCL->getVolumeStruct(volumeOut)
                          .getRepresentation<BufferCL>()));  // Scaling for 12-bit data
        
        kernel_->setArg(argIndex++, ivec4(outDim, 0));
        // Photon params
        kernel_->setArg(argIndex++, *photonsCL);
        kernel_->setArg(argIndex++, static_cast<int>(photons_.getData()->getNumberOfPhotons()));
        kernel_->setArg(argIndex++, static_cast<float>(photons_.getData()->getRadiusRelativeToSceneSize()));
        const PhotonData* inputPhotons = photons_.getData().get();
        // Use photon scale to get equivalent appearance when normalizing light volume
        //auto nPhotonsScale = static_cast<double>(inputPhotons->getNumberOfPhotons()) / static_cast<double>(PhotonData::defaultNumberOfPhotons);
        //float nPhotonsScale = static_cast<float>(PhotonData::defaultNumberOfPhotons) / static_cast<float>(photons_.getData()->getNumberOfPhotons());
        // Scale with lightVolumeSizeScale to get the same look for different light volume sizes.
        //double referenceRadiusVolumeScale = PhotonData::sphereVolume(inputPhotons->getRadiusRelativeToSceneSize()) / PhotonData::sphereVolume(PhotonData::defaultRadiusRelativeToSceneRadius);
        double nPhotons = static_cast<double>(inputPhotons->getNumberOfPhotons());
        double photonVolume = PhotonData::sphereVolume(inputPhotons->getRadiusRelativeToSceneSize());
        kernel_->setArg(argIndex++, static_cast<float>(PhotonData::scaleToMakeLightPowerOfOneVisibleForDirectionalLightSource / (photonVolume*nPhotons)));
        
        OpenCL::getPtr()->getQueue().enqueueNDRangeKernel(
                                                          *kernel_, cl::NullRange, globalWorkGroupSize, localWorkgroupSize, waitForEvents, splatEvent);
        
        std::vector<cl::Event> waitForPhotonToLightVolume(1, *splatEvent);
        
        //photonDensityNormalizationKernel_->setArg(0, *tmpVolumeCL);
        ////LogInfo("N components" << lightVolume_->getDataFormat()->getComponents());
        //photonDensityNormalizationKernel_->setArg(1, static_cast<int>(outDimFlattened));
        //OpenCL::getPtr()->getQueue().enqueueNDRangeKernel(
        //    *photonDensityNormalizationKernel_, cl::NullRange, getGlobalWorkGroupSize(outDimFlattened, localWorkgroupSize), localWorkgroupSize, &waitForPhotonToLightVolume, &events[2]);
        
        
        //std::vector<cl::Event> waitForNormalization(1, events[2]);
        
        OpenCL::getPtr()->getQueue().enqueueCopyBufferToImage(
                                                              tmpVolumeCL->get(), volumeOutCL->getEditable(), 0, size3_t(0), size3_t(outDim),
                                                              &waitForPhotonToLightVolume, copyEvent);
    } catch (cl::Error& err) {
        LogError(getCLErrorString(err));
    }
    
    
}

void PhotonToLightVolumeProcessorCL::clearBuffer(BufferCL* tmpVolumeCL, size_t outDimFlattened, const size_t& localWorkgroupSize, std::vector<cl::Event>* waitForEvents, cl::Event * events) {
    try {
        
        clearFloatsKernel_->setArg(0, *tmpVolumeCL);
        //LogInfo("N components" << lightVolume_->getDataFormat()->getComponents());
        clearFloatsKernel_->setArg(1, static_cast<int>(outDimFlattened));
        OpenCL::getPtr()->getQueue().enqueueNDRangeKernel(
                                                          *clearFloatsKernel_, cl::NullRange, getGlobalWorkGroupSize(outDimFlattened, localWorkgroupSize), localWorkgroupSize, nullptr, events);
        
    } catch (cl::Error& err) {
        LogError(getCLErrorString(err));
    }
}

void PhotonToLightVolumeProcessorCL::photonsToLightVolume(VolumeCLBase* volumeOutCL, const BufferCLBase* photonsCL, const BufferCLBase* photonIndices, const PhotonData& photons, const RecomputedPhotonIndices& recomputedPhotons, float radianceMultiplier, const Volume* volumeOut, const size3_t& outDim,
                                                          const size_t& globalWorkGroupSize,
                                                          const size_t& localWorkgroupSize, std::vector<cl::Event>* waitForEvents, cl::Event* event) {
    
    size_t outDimFlattened = outDim.x * outDim.y * outDim.z;
    
    if (tmpVolume_.getSize() != outDimFlattened*volumeOut->getDataFormat()->getSize()) {
        tmpVolume_.setSize(outDimFlattened*volumeOut->getDataFormat()->getSize());
    }
    BufferCL* tmpVolumeCL = tmpVolume_.getEditableRepresentation<BufferCL>();
    try {
        
        int argIndex = 0;
        splatSelectedPhotonsKernel_->setArg(argIndex++, *tmpVolumeCL);
        
        splatSelectedPhotonsKernel_->setArg(argIndex++,
                                            *(volumeOutCL->getVolumeStruct(volumeOut)
                                              .getRepresentation<BufferCL>()));  // Scaling for 12-bit data
        
        splatSelectedPhotonsKernel_->setArg(argIndex++, ivec4(outDim, 0));
        // Photon params
        splatSelectedPhotonsKernel_->setArg(argIndex++, *photonsCL);
        splatSelectedPhotonsKernel_->setArg(argIndex++, *photonIndices);
        splatSelectedPhotonsKernel_->setArg(argIndex++, static_cast<int>(recomputedPhotons.nRecomputedPhotons));
        splatSelectedPhotonsKernel_->setArg(argIndex++, static_cast<float>(photons.getRadiusRelativeToSceneSize()));
        // Use photon scale to get equivalent appearance when normalizing light volume
        //auto nPhotonsScale = static_cast<double>(photons.getNumberOfPhotons()) / static_cast<double>(PhotonData::defaultNumberOfPhotons);
        //float nPhotonsScale = static_cast<float>(PhotonData::defaultNumberOfPhotons) / static_cast<float>(photons_.getData()->getNumberOfPhotons());
        // Scale with lightVolumeSizeScale to get the same look for different light volume sizes.
        //double referenceRadiusVolumeScale = PhotonData::sphereVolume(photons.getRadiusRelativeToSceneSize()) / PhotonData::sphereVolume(PhotonData::defaultRadiusRelativeToSceneRadius);
        double nPhotons = static_cast<double>(photons.getNumberOfPhotons());
        double photonVolume = PhotonData::sphereVolume(photons.getRadiusRelativeToSceneSize());
        splatSelectedPhotonsKernel_->setArg(argIndex++, static_cast<float>(PhotonData::scaleToMakeLightPowerOfOneVisibleForDirectionalLightSource / (photonVolume*nPhotons)));
        splatSelectedPhotonsKernel_->setArg(argIndex++, radianceMultiplier);
        splatSelectedPhotonsKernel_->setArg(argIndex++, static_cast<int>(photons.getNumberOfPhotons()));
        splatSelectedPhotonsKernel_->setArg(argIndex++, static_cast<int>(photons.getMaxPhotonInteractions()));
        
        OpenCL::getPtr()->getAsyncQueue().enqueueNDRangeKernel(
                                                               *splatSelectedPhotonsKernel_, cl::NullRange, globalWorkGroupSize, localWorkgroupSize, waitForEvents, event);
        
        
    } catch (cl::Error& err) {
        LogError(getCLErrorString(err));
    }
}

void PhotonToLightVolumeProcessorCL::volumeSizeOptionChanged() {
    if (volumeInport_.hasData() && volumeSizeOption_.get() != 0) {
        auto inputVolume = volumeInport_.getData();
        auto newSize = inputVolume->getDimensions() / size3_t(volumeSizeOption_.get());
        if (glm::any(glm::notEqual(newSize, lightVolume_->getDimensions()))) {
            lightVolume_->setDimensions(newSize);
            lightVolume_->setModelMatrix(inputVolume->getModelMatrix());
            lightVolume_->setWorldMatrix(inputVolume->getWorldMatrix());
            information_.updateForNewVolume(*lightVolume_, false);
            // Recompute all photons
            prevPhotons_.setSize(0);
        }
    }
    
}

void PhotonToLightVolumeProcessorCL::buildKernel() {
    std::stringstream defines;
    std::string extensions = OpenCL::getPtr()->getDevice().getInfo<CL_DEVICE_EXTENSIONS>();
    if (volumeDataTypeOption_.getSelectedValue() == "float16" || volumeDataTypeOption_.getSelectedValue() == "4xfloat16") {
        defines << " -D VOLUME_OUTPUT_HALF_TYPE ";
    }
    if (volumeDataTypeOption_.getSelectedValue() == "float16" || volumeDataTypeOption_.getSelectedValue() == "float32") {
        defines << " -D VOLUME_OUTPUT_SINGLE_CHANNEL ";
        clearFloatsKernel_ = kernelOwner_.addKernel("photonstolightvolume.cl", "clearFloatKernel", "", defines.str());
    } else {
        clearFloatsKernel_ = kernelOwner_.addKernel("photonstolightvolume.cl", "clearFloat4Kernel", "", defines.str());
    }
    //kernel_ = kernelOwner_.addKernel("photonstolightvolume.cl", "photonsToLightVolumeKernel", "", defines.str());
    kernel_ = kernelOwner_.addKernel("photonstolightvolume.cl", "splatPhotonsToLightVolumeKernel", "", defines.str());
    splatSelectedPhotonsKernel_ = kernelOwner_.addKernel("photonstolightvolume.cl", "splatSelectedPhotonsToLightVolumeKernel", "", defines.str());
    
    photonDensityNormalizationKernel_ = kernelOwner_.addKernel("photonstolightvolume.cl", "photonDensityNormalizationKernel", "", defines.str());
    copyIndexPhotonsKernel_ = kernelOwner_.addKernel("photonstolightvolume.cl", "copyIndexPhotonsKernel", "", defines.str());
    
}

} // namespace


