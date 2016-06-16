/*********************************************************************************
 *
 * Inviwo - Interactive Visualization Workshop
 *
 * Copyright (c) 2013-2016 Inviwo Foundation
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

#ifndef IVW_MWC64X_SEED_GENERATOR_H
#define IVW_MWC64X_SEED_GENERATOR_H

#include <inviwo/core/common/inviwomodule.h>
#include <modules/rndgenmwc64x/rndgenmwc64xmoduledefine.h>
#include <modules/opencl/inviwoopencl.h>
#include <modules/opencl/buffer/bufferclbase.h>

namespace inviwo {

// Generating seed numbers for MWC64X can be very time consuming.
// This class performs the generation on the GPU to speed up the process.
class IVW_MODULE_RNDGENMWC64X_API MWC64XSeedGenerator {

public:
    /**
     * Creates and compiles OpenCL kernel.
     *
     */
    MWC64XSeedGenerator();
    virtual ~MWC64XSeedGenerator();

    /**
     * Generate random state for MWC64X random number generator in OpenCL.
     *
     * @param buffer uvec2 buffer to be filled with random seeds
     * @param seed Start seed number
     * @param useGLSharing True if BufferCLGL should be used, otherwise BufferCL 
     * @param localWorkGroupSize Local work group size to be used by the kernel
     */
    void generateRandomSeeds(Buffer<uvec2>* buffer, unsigned int seed, bool useGLSharing = true, size_t localWorkGroupSize = 256);

protected:
    void generateSeeds( BufferCLBase* randomSeedBufferCL, int nRandomSeeds, size_t localWorkGroupSize );

    cl::Kernel* kernel_;
};

} // namespace

#endif // IVW_MWC64X_SEED_GENERATOR_H