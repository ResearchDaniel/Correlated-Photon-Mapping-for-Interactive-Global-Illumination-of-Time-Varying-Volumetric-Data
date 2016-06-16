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

#ifndef IVW_MWC64XRANDOMNUMBERGENERATOR_H
#define IVW_MWC64XRANDOMNUMBERGENERATOR_H

#include <modules/rndgenmwc64x/rndgenmwc64xmoduledefine.h>
#include <inviwo/core/common/inviwo.h>

#include <modules/opencl/inviwoopencl.h>
#include <modules/opencl/buffer/buffercl.h>
#include <modules/opencl/kernelowner.h>

#include <modules/opencl/buffer/bufferclbase.h>
namespace inviwo {

/**
 * \class MWC64XRandomNumberGenerator
 *
 * \brief Generate N random numbers in parallel, each number stream will have its own seed. 
 *
 */
class IVW_MODULE_RNDGENMWC64X_API MWC64XRandomNumberGenerator: public KernelOwner { 
public:
    MWC64XRandomNumberGenerator(bool useGLSharing = true);
    virtual ~MWC64XRandomNumberGenerator() = default;

    /** 
     * \brief Fill randomNumbersOut with random numbers.
     *
     * [0 randomNumbersOut.getSize()-1] streams will be used, 
     * each with its own seed.
     * 
     * @param Buffer<float> & randomNumbersOut Buffer to fill 
     * @param const VECTOR_CLASS<cl::Event> * waitForEvents Events to wait for
     * @param cl::Event * event 
     */
    void generate(Buffer<float>& randomNumbersOut, const VECTOR_CLASS<cl::Event>* waitForEvents = nullptr, cl::Event* event = nullptr);
    
    int getSeed() const { return seed_; }
    /** 
     * \brief Seed to be used for srand to initiate each random stream.
     *
     * @see MWC64XSeedGenerator
     * @param int val Random seed.
     */
    void setSeed(int val);

    bool getUseGLSharing() const { return useGLSharing_; }
    void setUseGLSharing(bool val) { useGLSharing_ = val; }
    size_t getWorkGroupSize() const { return workGroupSize_; }
    void setWorkGroupSize(size_t val) { workGroupSize_ = val; }
private:
    void generateNumbers(BufferCL* rndState, BufferCLBase* data, const VECTOR_CLASS<cl::Event>* waitForEvents = nullptr, cl::Event* event = nullptr);
    Buffer<uvec2> randomState_;
    size_t workGroupSize_ = 256;
    cl::Kernel* kernel_;
    int seed_ = 0;
    bool useGLSharing_;



    
};

} // namespace

#endif // IVW_MWC64XRANDOMNUMBERGENERATOR_H

