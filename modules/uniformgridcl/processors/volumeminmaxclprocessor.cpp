/*********************************************************************************
 *
 * Inviwo - Interactive Visualization Workshop
 *
 * Copyright (c) 2016 Daniel Jönsson
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
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


#include "volumeminmaxclprocessor.h"
#include <inviwo/core/datastructures/buffer/buffer.h>
#include <inviwo/core/datastructures/volume/volumeram.h>
#include <modules/opencl/syncclgl.h>
#include <modules/opencl/buffer/buffercl.h>
#include <modules/opencl/buffer/bufferclgl.h>
#include <modules/opencl/volume/volumecl.h>
#include <modules/opencl/volume/volumeclgl.h>

#include <modules/opencl/inviwoopencl.h>

namespace inviwo {
    
// The Class Identifier has to be globally unique. Use a reverse DNS naming scheme
const ProcessorInfo VolumeMinMaxCLProcessor::processorInfo_{
    "org.inviwo.VolumeMinMaxCLProcessor",  // Class identifier
    "Min-max uniform grid",                  // Display name
    "Volume",                                // Category
    CodeState::Experimental,                 // Code state
    Tags::CL,                                // Tags
};
const ProcessorInfo VolumeMinMaxCLProcessor::getProcessorInfo() const {
    return processorInfo_;
}

VolumeMinMaxCLProcessor::VolumeMinMaxCLProcessor()
: Processor()
, ProcessorKernelOwner(this)
, inport_("volume")
, outport_("output")
, vectorInport_("VolumeSequenceInput")
, vectorOutport_("UniformGrid3DVectorOut")
, volumeRegionSize_("region", "Region size", 8, 1, 100)
, workGroupSize_("wgsize", "Work group size", ivec3(4), ivec3(0), ivec3(256))
, useGLSharing_("glsharing", "Use OpenGL sharing", true)
//, volumeOut_(new MinMaxUniformGrid3D(size3_t(volumeRegionSize_.get())))
, kernel_(nullptr) {
    addPort(inport_);
    addPort(outport_);
    
    addPort(vectorInport_);
    addPort(vectorOutport_);
    
    inport_.setOptional(true);
    vectorInport_.setOptional(true);
    
    addProperty(volumeRegionSize_);
    addProperty(workGroupSize_);
    addProperty(useGLSharing_);
    std::stringstream defines;
    
    //volumeRegionSize_.onChange([this]() {volumeOut_->setCellDimension(size3_t(volumeRegionSize_.get())); });
    
    kernel_ = addKernel("uniformgrid/volumeminmax.cl", "volumeMinMaxKernel");
    
}

void VolumeMinMaxCLProcessor::process() {
    if (kernel_ == nullptr) {
        return;
    }
    
    if (vectorInport_.isReady()) {
        outport_.setData(nullptr);
        auto volumes = vectorInport_.getData().get();
        std::shared_ptr<UniformGrid3DVector> output = std::make_shared<UniformGrid3DVector>();
        auto memSize = OpenCL::getPtr()->getDevice().getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>();
        
        for (auto elem : *volumes)
        {
            auto result = compute(elem.get());
            if (result) {
                // Remove GPU representations
                auto dim = elem->getDimensions();
                if (dim.x*dim.y*dim.z* elem->getDataFormat()->getSize()*output->size() > memSize / 3) {
                    elem->removeOtherRepresentations(elem->getRepresentation<VolumeRAM>());
                    auto ramRep = result->data.getRAMRepresentation();
                    result->data.removeOtherRepresentations(ramRep);
                }
                
                output->emplace_back(std::shared_ptr<UniformGrid3DBase>(result.release()));
            }
            
        }
        vectorOutport_.setData(output);
    }
    if (inport_.isReady()) {
        auto result = compute(inport_.getData().get());
        if (result)
        outport_.setData(std::shared_ptr<const UniformGrid3DBase>(result.release()));
    }
    
}

void VolumeMinMaxCLProcessor::executeVolumeOperation(const Volume* volume,
                                                     const VolumeCLBase* volumeCL,
                                                     BufferCLBase* volumeOutCL, const size3_t& outDim,
                                                     const size3_t& globalWorkGroupSize,
                                                     const size3_t& localWorkgroupSize) {
    IVW_OPENCL_PROFILING(profilingEvent, "")
    try {
        int argIndex = 0;
        kernel_->setArg(argIndex++, *volumeCL);
        kernel_->setArg(argIndex++,
                        *(volumeCL->getVolumeStruct(volume)
                          .getRepresentation<BufferCL>()));  // Scaling for 12-bit data
        kernel_->setArg(argIndex++, *volumeOutCL);
        kernel_->setArg(argIndex++, ivec4(outDim, 0));
        kernel_->setArg(argIndex++, ivec4(volumeRegionSize_.get()));
        
        OpenCL::getPtr()->getQueue().enqueueNDRangeKernel(
                                                          *kernel_, cl::NullRange, globalWorkGroupSize, localWorkgroupSize, nullptr, profilingEvent);
        
    } catch (cl::Error& err) {
        LogError(getCLErrorString(err));
    }
}

std::unique_ptr<MinMaxUniformGrid3D> VolumeMinMaxCLProcessor::compute(const Volume* volume) {
    const size3_t dim{ volume->getDimensions() };
    const size3_t outDim{ glm::ceil(vec3(dim) / static_cast<float>(volumeRegionSize_.get())) };
    // const DataFormatBase* volFormat = inport_.getData()->getDataFormat(); // Not used
    std::unique_ptr<MinMaxUniformGrid3D> volumeOut_(new MinMaxUniformGrid3D(size3_t(volumeRegionSize_.get())));
    if (volumeOut_->getDimensions() != outDim) {
        // Use same transformation to make sure that they are render at the same location
        volumeOut_->setModelMatrix(volume->getModelMatrix());
        volumeOut_->setWorldMatrix(volume->getWorldMatrix());
        volumeOut_->setDimensions(outDim);
    }
    
    size3_t localWorkGroupSize(workGroupSize_.get());
    size3_t globalWorkGroupSize(getGlobalWorkGroupSize(outDim.x, localWorkGroupSize.x),
                                getGlobalWorkGroupSize(outDim.y, localWorkGroupSize.y),
                                getGlobalWorkGroupSize(outDim.z, localWorkGroupSize.z));
    
    if (useGLSharing_.get()) {
        SyncCLGL glSync;
        const VolumeCLGL* volumeCL = volume->getRepresentation<VolumeCLGL>();
        BufferCLGL* volumeOutCL = volumeOut_->data.getEditableRepresentation<BufferCLGL>();
        
        glSync.addToAquireGLObjectList(volumeCL);
        glSync.addToAquireGLObjectList(volumeOutCL);
        glSync.aquireAllObjects();
        
        executeVolumeOperation(volume, volumeCL, volumeOutCL, outDim, globalWorkGroupSize,
                               localWorkGroupSize);
    } else {
        const VolumeCL* volumeCL = volume->getRepresentation<VolumeCL>();
        BufferCLGL* volumeOutCL = volumeOut_->data.getEditableRepresentation<BufferCLGL>();
        executeVolumeOperation(volume, volumeCL, volumeOutCL, outDim, globalWorkGroupSize,
                               localWorkGroupSize);
    }
    return volumeOut_;
}

} // namespace


