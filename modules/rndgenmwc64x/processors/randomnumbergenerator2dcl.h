/*********************************************************************************
 *
 * Inviwo - Interactive Visualization Workshop
 *
 * Copyright (c) 2014-2016 Inviwo Foundation
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

#ifndef IVW_RANDOM_NUMBER_GENERATOR_2D_CL_H
#define IVW_RANDOM_NUMBER_GENERATOR_2D_CL_H

#include <inviwo/core/common/inviwo.h>
#include <inviwo/core/datastructures/buffer/buffer.h>
#include <inviwo/core/processors/processor.h>
#include <inviwo/core/ports/imageport.h>
#include <inviwo/core/properties/buttonproperty.h>
#include <modules/opencl/buffer/buffercl.h>
#include <modules/opencl/image/layerclbase.h>
#include <modules/opencl/inviwoopencl.h>
#include <modules/opencl/kernelowner.h>


#include <modules/rndgenmwc64x/rndgenmwc64xmoduledefine.h>


namespace inviwo {

/*
 * Generate random numbers and store them into an image (single 32-bit float 
 */
/** \docpage{org.inviwo.RandomNumberGenerator2DCL, Random Number Generator 2D}
 * ![](org.inviwo.RandomNumberGenerator2DCL.png?classIdentifier=org.inviwo.RandomNumberGenerator2DCL)
 *
 * ...
 * 
 * 
 * ### Outports
 *   * __samples__ ...
 * 
 * ### Properties
 *   * __Use OpenGL sharing__ ...
 *   * __N samples__ ...
 *   * __Regenerate__ ...
 *   * __Seed number__ ...
 *   * __Work group size__ ...
 *
 */
class IVW_MODULE_RNDGENMWC64X_API RandomNumberGenerator2DCL : public Processor, public ProcessorKernelOwner {

public:
    RandomNumberGenerator2DCL();
    ~RandomNumberGenerator2DCL() = default;
    
    virtual const ProcessorInfo getProcessorInfo() const override;
    static const ProcessorInfo processorInfo_;

    virtual void process();
protected:
    void nRandomNumbersChanged();
    void regenerate();
    void generateNumbers(BufferCL* rndState, LayerCLBase* data, cl::Event* profilingEvent);
private:
    ImageOutport randomNumbersPort_;

    IntVec2Property nRandomNumbers_;
    ButtonProperty regenerateNumbers_;
    IntProperty seed_;
    IntProperty workGroupSize_;
    BoolProperty useGLSharing_;

    Buffer<uvec2> randomState_;
    cl::Kernel* kernel_;
};

}

#endif // IVW_RANDOM_NUMBER_GENERATOR_2D_CL_H
