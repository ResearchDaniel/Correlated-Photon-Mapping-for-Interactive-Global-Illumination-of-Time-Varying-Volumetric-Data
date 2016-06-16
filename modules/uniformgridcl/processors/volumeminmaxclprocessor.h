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


#ifndef IVW_VOLUMEMINMAXCLPROCESSOR_H
#define IVW_VOLUMEMINMAXCLPROCESSOR_H

#include <modules/uniformgridcl/uniformgridclmoduledefine.h>
#include <inviwo/core/common/inviwo.h>
#include <inviwo/core/datastructures/buffer/buffer.h>
#include <inviwo/core/ports/volumeport.h>
#include <inviwo/core/processors/processor.h>

#include <modules/opencl/inviwoopencl.h>
#include <modules/opencl/kernelowner.h>
#include <modules/opencl/buffer/bufferclbase.h>
#include <modules/opencl/volume/volumeclbase.h>

#include <modules/uniformgridcl/minmaxuniformgrid3d.h>

namespace inviwo {

/** \docpage{<classIdentifier>, VolumeMinMaxCLProcessor}
 * Computes the minimum and maximum data value for each sub-region of the input volume. *
 *
 * ### Inports
 *   * __<Inport1>__ <description>.
 *
 * ### Outports
 *   * __<Outport1>__ <description>.
 *
 * ### Properties
 *   * __<Prop1>__ <description>.
 *   * __<Prop2>__ <description>
 */


/**
 * \class VolumeMinMaxCLProcessor
 *
 * \brief <brief description> 
 *
 * <Detailed description from a developer prespective>
 */
class IVW_MODULE_UNIFORMGRIDCL_API VolumeMinMaxCLProcessor : public Processor, public ProcessorKernelOwner {
public:
    virtual const ProcessorInfo getProcessorInfo() const override;
    static const ProcessorInfo processorInfo_;
    VolumeMinMaxCLProcessor();
    virtual ~VolumeMinMaxCLProcessor() = default;
     
    virtual void process() override;

    std::unique_ptr<MinMaxUniformGrid3D> compute(const Volume* volume);

    virtual bool isReady() const override;

    void executeVolumeOperation(const Volume* volume, const VolumeCLBase* volumeCL, BufferCLBase* volumeOutCL, const size3_t& outDim, const size3_t& globalWorkGroupSize, const size3_t& localWorkgroupSize);
private:
    VolumeInport inport_;
    UniformGrid3DOutport outport_;

    // Vector in/out
    VolumeSequenceInport vectorInport_;
    UniformGrid3DVectorOutport vectorOutport_;

    IntProperty volumeRegionSize_;
    IntVec3Property workGroupSize_;
    BoolProperty useGLSharing_;

    
    cl::Kernel* kernel_;
};

} // namespace

#endif // IVW_VOLUMEMINMAXCLPROCESSOR_H

