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

#include <modules/rndgenmwc64x/processors/randomnumbergenerator2dcl.h>
#include <modules/rndgenmwc64x/mwc64xseedgenerator.h>

#include <inviwo/core/datastructures/buffer/bufferram.h>
#include <modules/opencl/inviwoopencl.h>
#include <modules/opencl/buffer/buffercl.h>
#include <modules/opencl/buffer/bufferclgl.h>
#include <modules/opencl/image/imagecl.h>
#include <modules/opencl/image/imageclgl.h>
#include <modules/opencl/syncclgl.h>
#include <modules/opencl/kernelmanager.h>

namespace inviwo {

const ProcessorInfo RandomNumberGenerator2DCL::processorInfo_{
    "org.inviwo.RandomNumberGenerator2DCL",  // Class identifier
    "Random Number Generator 2D",            // Display name
    "Random numbers",                        // Category
    CodeState::Stable,                       // Code state
    Tags::CL,                                // Tags
};
const ProcessorInfo RandomNumberGenerator2DCL::getProcessorInfo() const {
    return processorInfo_;
}

RandomNumberGenerator2DCL::RandomNumberGenerator2DCL()
    : Processor()
    , ProcessorKernelOwner(this)
    , randomNumbersPort_("samples", DataFloat32::get(), false)
    , nRandomNumbers_("nSamples", "N samples", ivec2(128), ivec2(2), ivec2(2048))
    , regenerateNumbers_("genRnd", "Regenerate")
    , seed_("seed", "Seed number", 0, 0, std::numeric_limits<int>::max())
    , workGroupSize_("wgsize", "Work group size", 256, 1, 2048)
    , useGLSharing_("glsharing", "Use OpenGL sharing", true)
    , kernel_(NULL) {

    addPort(randomNumbersPort_);

    addProperty(nRandomNumbers_);
    addProperty(regenerateNumbers_);
    addProperty(seed_);
    addProperty(workGroupSize_);
    addProperty(useGLSharing_);

    nRandomNumbers_.onChange(this, &RandomNumberGenerator2DCL::nRandomNumbersChanged);
    regenerateNumbers_.onChange(this, &RandomNumberGenerator2DCL::regenerate);

    kernel_ = addKernel("randomnumbergenerator.cl", "randomNumberGenerator2DKernel");
    nRandomNumbersChanged();
}

void RandomNumberGenerator2DCL::process() {
    if (kernel_ == NULL) {
        return;
    }
    IVW_OPENCL_PROFILING(profilingEvent, "")
    BufferCL* rndState = randomState_.getEditableRepresentation<BufferCL>();
    if (useGLSharing_.get()) {
        SyncCLGL glSync;
        LayerCLGL* data = randomNumbersPort_.getEditableData()->getEditableRepresentation<ImageCLGL>()->getLayerCL();
        // Acquire shared representations before using them in OpenGL
        // The SyncCLGL object will take care of synchronization between OpenGL and OpenCL
        glSync.addToAquireGLObjectList(data);
        glSync.aquireAllObjects();
        generateNumbers(rndState, data, profilingEvent);
    } else {
        LayerCL* data = randomNumbersPort_.getEditableData()->getEditableRepresentation<ImageCL>()->getLayerCL();
        generateNumbers(rndState, data, profilingEvent);
    }

}


void RandomNumberGenerator2DCL::nRandomNumbersChanged() {
    if (randomNumbersPort_.getData()) {
        size_t nRandomNumbers = nRandomNumbers_.get().x*nRandomNumbers_.get().y;
        randomState_.setSize(nRandomNumbers);
        
        MWC64XSeedGenerator seedGenerator;
        seedGenerator.generateRandomSeeds(&randomState_, seed_.get(), false);

        randomNumbersPort_.setDimensions(nRandomNumbers_.get());
    }

}

void RandomNumberGenerator2DCL::regenerate() {
    invalidate(InvalidationLevel::InvalidOutput);
}

void RandomNumberGenerator2DCL::generateNumbers(BufferCL* rndState, LayerCLBase* data, cl::Event* profilingEvent) {
    try {
        int argIndex = 0;
        kernel_->setArg(argIndex++, *rndState);
        kernel_->setArg(argIndex++, nRandomNumbers_.get());
        kernel_->setArg(argIndex++, *data);
        size_t workGroupSizeX = static_cast<size_t>(workGroupSize_.get());
        size_t globalWorkSizeX = getGlobalWorkGroupSize(nRandomNumbers_.get().x*nRandomNumbers_.get().y, workGroupSizeX);

        OpenCL::getPtr()->getQueue().enqueueNDRangeKernel(*kernel_, cl::NullRange, globalWorkSizeX, workGroupSizeX, NULL, profilingEvent);

    } catch (cl::Error& err) {
        LogError(getCLErrorString(err));
    }
}

} // inviwo namespace

