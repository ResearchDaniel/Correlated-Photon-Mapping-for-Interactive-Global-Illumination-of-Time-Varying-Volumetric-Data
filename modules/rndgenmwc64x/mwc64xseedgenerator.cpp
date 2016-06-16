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

#include <inviwo/core/datastructures/buffer/bufferram.h>
#include <inviwo/core/util/logcentral.h>
#include <modules/opencl/buffer/buffercl.h>
#include <modules/opencl/buffer/bufferclgl.h>
#include <modules/opencl/inviwoopencl.h>
#include <modules/opencl/kernelmanager.h>
#include <modules/opencl/syncclgl.h>
#include <modules/rndgenmwc64x/mwc64xseedgenerator.h>
#include <stdlib.h>

namespace inviwo {

MWC64XSeedGenerator::MWC64XSeedGenerator(): kernel_(NULL) { 
    cl::Program* program = KernelManager::getPtr()->buildProgram("randstategen.cl");
    kernel_ = KernelManager::getPtr()->getKernel(program, "MWC64X_GenerateRandomState", NULL);
}

MWC64XSeedGenerator::~MWC64XSeedGenerator() {
}


void MWC64XSeedGenerator::generateRandomSeeds(Buffer<uvec2>* buffer, unsigned int seed, bool useGLSharing /*= true*/, size_t localWorkGroupSize /*= 256*/) {
    if (kernel_ == NULL) {
        return;
    }

    srand(seed);
    // MWC64X random number generator
    BufferRAM* bufferRAM = buffer->getEditableRepresentation<BufferRAM>();
    uvec2* randomNumbers = static_cast<uvec2*>(bufferRAM->getData());
    int nRandomSeeds = static_cast<int>(bufferRAM->getSize());

    for (int i = 0; i < nRandomSeeds; ++i) {
        randomNumbers[i].x = static_cast<cl_uint>(rand());
    }
    // Data will be transferred to OpenCL device before new representation is returned.
    if (useGLSharing) {
        SyncCLGL glSync;
        BufferCLGL* randomSeedBufferCL = buffer->getEditableRepresentation<BufferCLGL>();
        glSync.addToAquireGLObjectList(randomSeedBufferCL);
        glSync.aquireAllObjects();
        generateSeeds(randomSeedBufferCL, nRandomSeeds, localWorkGroupSize);
    } else {
        BufferCLBase* randomSeedBufferCL = buffer->getEditableRepresentation<BufferCL>();
        generateSeeds(randomSeedBufferCL, nRandomSeeds, localWorkGroupSize);
    }
}

void MWC64XSeedGenerator::generateSeeds(BufferCLBase* randomSeedBufferCL, int nRandomSeeds, size_t localWorkGroupSize) {
    try
    {
        kernel_->setArg(0, *randomSeedBufferCL);
        kernel_->setArg(1, nRandomSeeds);
        size_t globalWorkSizeX = getGlobalWorkGroupSize(nRandomSeeds, localWorkGroupSize);
        OpenCL::getPtr()->getQueue().enqueueNDRangeKernel(*kernel_, 0, globalWorkSizeX, localWorkGroupSize);
    }
    catch (cl::Error& err)
    {
        LogError(getCLErrorString(err));
    }
}


} // namespace

