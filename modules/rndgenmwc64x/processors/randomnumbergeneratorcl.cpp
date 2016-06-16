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

#include <modules/rndgenmwc64x/processors/randomnumbergeneratorcl.h>

#include <inviwo/core/datastructures/buffer/bufferram.h>
#include <modules/opencl/inviwoopencl.h>

namespace inviwo {

const ProcessorInfo RandomNumberGeneratorCL::processorInfo_{
    "org.inviwo.RandomNumberGeneratorCL",  // Class identifier
    "Random Number Generator",             // Display name
    "Random numbers",                      // Category
    CodeState::Stable,                     // Code state
    Tags::CL,                              // Tags
};
const ProcessorInfo RandomNumberGeneratorCL::getProcessorInfo() const {
    return processorInfo_;
}

RandomNumberGeneratorCL::RandomNumberGeneratorCL()
    : Processor()
    , ProcessorKernelOwner(this)
    , randomNumbersPort_("samples")
    , nRandomNumbers_("nSamples", "N samples", 256, 1, 100000000)
    , regenerateNumbers_("genRnd", "Regenerate")
    , seed_("seed", "Seed number", 0, 0, std::numeric_limits<int>::max())
    , workGroupSize_("wgsize", "Work group size", 256, 1, 2048)
    , useGLSharing_("glsharing", "Use OpenGL sharing", true)
    , randomNumbersOut_(std::make_shared<Buffer<float>>(nRandomNumbers_.get())) {

    addPort(randomNumbersPort_);

    addProperty(nRandomNumbers_);
    addProperty(regenerateNumbers_);
    addProperty(seed_);
    addProperty(workGroupSize_);
    addProperty(useGLSharing_);

    nRandomNumbers_.onChange([this]() { randomNumbersOut_->setSize(nRandomNumbers_); });
    seed_.onChange([this]() { randomNumberGenerator_.setSeed(seed_); });
    workGroupSize_.onChange([this]() { randomNumberGenerator_.setWorkGroupSize(workGroupSize_); });
    useGLSharing_.onChange([this]() { randomNumberGenerator_.setUseGLSharing(useGLSharing_); });
    regenerateNumbers_.onChange(this, &RandomNumberGeneratorCL::regenerate);

    randomNumbersPort_.setData(randomNumbersOut_);
}



void RandomNumberGeneratorCL::process() {
    IVW_OPENCL_PROFILING(profilingEvent, "")
    
    randomNumberGenerator_.generate(*randomNumbersOut_, nullptr, profilingEvent);
    
    // For debugging purposes    
    //const BufferRAM* data = randomNumbersPort_.getData()->getRepresentation<BufferRAM>();
    //const float *d = static_cast<const float*>(data->getData()); 
    //for (int i = 0; i < data->getSize(); ++i) {
    //    std::cout << i << ": " << (float)d[i] << std::endl; 
    //}

}

void RandomNumberGeneratorCL::regenerate() {
    invalidate(InvalidationLevel::InvalidOutput);
}

} // inviwo namespace

