/*********************************************************************************
 *
 * Inviwo - Interactive Visualization Workshop
 *
 * Copyright (c) 2015-2016 Inviwo Foundation
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

#include "mwc64xrandomnumbergenerator.h"
#include <modules/rndgenmwc64x/mwc64xseedgenerator.h>
#include <modules/opencl/buffer/bufferclgl.h>
#include <modules/opencl/syncclgl.h>

namespace inviwo {

MWC64XRandomNumberGenerator::MWC64XRandomNumberGenerator(bool useGLSharing /*= true*/) 
: KernelOwner(), useGLSharing_(useGLSharing) {
    kernel_ = addKernel("randomnumbergenerator.cl", "randomNumberGeneratorKernel");
}

void MWC64XRandomNumberGenerator::generate(Buffer<float>& randomNumbersOut, const VECTOR_CLASS<cl::Event>* waitForEvents /*= nullptr*/, cl::Event* event /*= nullptr*/) {
    if (randomNumbersOut.getSize() != randomState_.getSize()) {
        randomState_.setSize(randomNumbersOut.getSize());

        MWC64XSeedGenerator seedGenerator;
        seedGenerator.generateRandomSeeds(&randomState_, seed_, false);
    }

    BufferCL* rndState = randomState_.getEditableRepresentation<BufferCL>();
    if (useGLSharing_) {
        SyncCLGL glSync;
        BufferCLGL* data = randomNumbersOut.getEditableRepresentation<BufferCLGL>();
        // Acquire shared representations before using them in OpenGL
        // The SyncCLGL object will take care of synchronization between OpenGL and OpenCL
        glSync.addToAquireGLObjectList(data);
        glSync.aquireAllObjects();
        generateNumbers(rndState, data, waitForEvents, event);
    } else {
        BufferCL* data = randomNumbersOut.getEditableRepresentation<BufferCL>();
        generateNumbers(rndState, data, waitForEvents, event);


    }
}

void MWC64XRandomNumberGenerator::setSeed(int val) {
    seed_ = val;
    if (randomState_.getSize() > 0) {
        MWC64XSeedGenerator seedGenerator;
        seedGenerator.generateRandomSeeds(&randomState_, seed_, false);
    }

}

void MWC64XRandomNumberGenerator::generateNumbers(BufferCL* rndState, BufferCLBase* data, const VECTOR_CLASS<cl::Event>* waitForEvents /*= nullptr*/, cl::Event* event /*= nullptr*/) {
    int argIndex = 0;
    kernel_->setArg(argIndex++, *rndState);
    kernel_->setArg(argIndex++, static_cast<int>(randomState_.getSize()));
    kernel_->setArg(argIndex++, *data);
    size_t globalWorkSizeX = getGlobalWorkGroupSize(randomState_.getSize(), workGroupSize_);

    OpenCL::getPtr()->getQueue().enqueueNDRangeKernel(*kernel_, cl::NullRange, globalWorkSizeX, workGroupSize_, waitForEvents, event);
}

} // namespace

