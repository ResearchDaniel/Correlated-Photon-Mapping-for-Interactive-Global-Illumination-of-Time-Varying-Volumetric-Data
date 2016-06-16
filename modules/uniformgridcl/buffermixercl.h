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

#ifndef IVW_BUFFERMIXERCL_H
#define IVW_BUFFERMIXERCL_H

#include <modules/uniformgridcl/uniformgridclmoduledefine.h>
#include <inviwo/core/common/inviwo.h>
#include <inviwo/core/datastructures/buffer/buffer.h>

#include <modules/opencl/inviwoopencl.h>
#include <modules/opencl/kernelowner.h>
#include <modules/opencl/buffer/bufferclbase.h>

namespace inviwo {

/**
 * \class BufferMixerCL
 * \brief mix two buffers
 * 
 */
class IVW_MODULE_UNIFORMGRIDCL_API BufferMixerCL : public KernelOwner {
public:
    BufferMixerCL(const size_t& workgroupSize = 128, bool useGLSharing = true);
    virtual ~BufferMixerCL();

    void mix(const BufferBase& x, const BufferBase& y, float a, BufferBase& out, const VECTOR_CLASS<cl::Event> *waitForEvents, cl::Event *event = nullptr);

    void mix(const BufferCLBase* xCL, const BufferCLBase* yCL, float a, BufferCLBase* outCL, size_t nElements, const VECTOR_CLASS<cl::Event> * waitForEvents, cl::Event * event);

    void compileKernel();

    size_t workGroupSize() const { return workGroupSize_; }
    void workGroupSize(size_t val) { workGroupSize_ = val; }
    bool useGLSharing() const { return useGLSharing_; }
    void useGLSharing(bool val) { useGLSharing_ = val; }
protected:
    const DataFormatBase* format_;
    cl::Kernel* kernel_;
    size_t workGroupSize_;
    bool useGLSharing_;
};

} // namespace

#endif // IVW_BUFFERMIXERCL_H

