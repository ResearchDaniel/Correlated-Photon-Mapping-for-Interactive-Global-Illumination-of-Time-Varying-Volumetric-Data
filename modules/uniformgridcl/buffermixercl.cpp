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

#include "buffermixercl.h"
#include <modules/opencl/buffer/buffercl.h>
#include <modules/opencl/buffer/bufferclgl.h>
#include <modules/opencl/syncclgl.h>

namespace inviwo {

BufferMixerCL::BufferMixerCL(const size_t& workgroupSize, bool useGLSharing /*= false*/)
    : KernelOwner(), workGroupSize_(workgroupSize), useGLSharing_(useGLSharing), format_(nullptr), kernel_(nullptr)  {
    
}

BufferMixerCL::~BufferMixerCL()  {
    
}

void BufferMixerCL::mix(const BufferBase& x, const BufferBase& y, float a, BufferBase& out, const VECTOR_CLASS<cl::Event> *waitForEvents, cl::Event *event) {

    if (format_ == nullptr || format_ != x.getDataFormat()) {
        format_ = x.getDataFormat();
        compileKernel();
    }
    if (useGLSharing_) {
        SyncCLGL glSync;
        auto xCL = x.getRepresentation<BufferCLGL>();
        auto yCL = y.getRepresentation<BufferCLGL>();
        auto outCL = out.getEditableRepresentation<BufferCLGL>();
        glSync.addToAquireGLObjectList(xCL);
        glSync.addToAquireGLObjectList(yCL);
        glSync.addToAquireGLObjectList(outCL);
        glSync.aquireAllObjects();
        mix(xCL, yCL, a, outCL, out.getSize(), waitForEvents, event);
    } else {
        auto xCL = x.getRepresentation<BufferCL>();
        auto yCL = y.getRepresentation<BufferCL>();
        auto outCL = out.getEditableRepresentation<BufferCL>();
        mix(xCL, yCL, a, outCL, out.getSize(), waitForEvents, event);
    }


}

void BufferMixerCL::mix(const BufferCLBase* xCL, const BufferCLBase* yCL, float a, BufferCLBase* outCL, size_t nElements, const VECTOR_CLASS<cl::Event> * waitForEvents, cl::Event * event) {
    int argIndex = 0;
    kernel_->setArg(argIndex++, *xCL);
    kernel_->setArg(argIndex++, *yCL);
    kernel_->setArg(argIndex++, a);
    kernel_->setArg(argIndex++, static_cast<unsigned int>(nElements));
    kernel_->setArg(argIndex++, *outCL);


    size_t globalWorkSizeX = getGlobalWorkGroupSize(nElements, workGroupSize_);
    OpenCL::getPtr()->getQueue().enqueueNDRangeKernel(*kernel_, cl::NullRange, globalWorkSizeX,
        workGroupSize_, waitForEvents, event);
}

std::string dataFormatToOpenCLType(const DataFormatBase* dataFormat) {
    std::string result;
    switch (dataFormat->getId()) {
    case DataFormatId::NotSpecialized:
        break;
    case DataFormatId::Float16:
        result = "half";
        break;
    case DataFormatId::Float32:
        result = "float";
        break;
    case DataFormatId::Float64:
        result = "double";
        break;
    case DataFormatId::Int8:
        result = "uchar";
        break;
    case DataFormatId::Int16:
        result = "short";
        break;
    case DataFormatId::Int32:
        result = "int";
        break;
    case DataFormatId::Int64:
        result = "long";
        break;
    case DataFormatId::UInt8:
        result = "uchar";
        break;
    case DataFormatId::UInt16:
        result = "ushort";
        break;
    case DataFormatId::UInt32:
        result = "uint";
        break;
    case DataFormatId::UInt64:
        result = "ulong";
        break;
    case DataFormatId::Vec2Float16:
        result = "half2";
        break;
    case DataFormatId::Vec2Float32:
        result = "float2";
        break;
    case DataFormatId::Vec2Float64:
        result = "double2";
        break;
    case DataFormatId::Vec2Int8:
        result = "char2";
        break;
    case DataFormatId::Vec2Int16:
        result = "short2";
        break;
    case DataFormatId::Vec2Int32:
        result = "int2";
        break;
    case DataFormatId::Vec2Int64:
        result = "long2";
        break;
    case DataFormatId::Vec2UInt8:
        result = "uchar2";
        break;
    case DataFormatId::Vec2UInt16:
        result = "ushort2";
        break;
    case DataFormatId::Vec2UInt32:
        result = "uint2";
        break;
    case DataFormatId::Vec2UInt64:
        result = "ulong2";
        break;
    case DataFormatId::Vec3Float16:
        result = "half3";
        break;
    case DataFormatId::Vec3Float32:
        result = "float3";
        break;
    case DataFormatId::Vec3Float64:
        result = "double3";
        break;
    case DataFormatId::Vec3Int8:
        result = "char3";
        break;
    case DataFormatId::Vec3Int16:
        result = "short3";
        break;
    case DataFormatId::Vec3Int32:
        result = "int3";
        break;
    case DataFormatId::Vec3Int64:
        result = "long3";
        break;
    case DataFormatId::Vec3UInt8:
        result = "uchar3";
        break;
    case DataFormatId::Vec3UInt16:
        result = "ushort3";
        break;
    case DataFormatId::Vec3UInt32:
        result = "uint3";
        break;
    case DataFormatId::Vec3UInt64:
        result = "ulong3";
        break;
    case DataFormatId::Vec4Float16:
        result = "half4";
        break;
    case DataFormatId::Vec4Float32:
        result = "float4";
        break;
    case DataFormatId::Vec4Float64:
        result = "double4";
        break;
    case DataFormatId::Vec4Int8:
        result = "char4";
        break;
    case DataFormatId::Vec4Int16:
        result = "short4";
        break;
    case DataFormatId::Vec4Int32:
        result = "int4";
        break;
    case DataFormatId::Vec4Int64:
        result = "long4";
        break;
    case DataFormatId::Vec4UInt8:
        result = "uchar4";
        break;
    case DataFormatId::Vec4UInt16:
        result = "ushort4";
        break;
    case DataFormatId::Vec4UInt32:
        result = "uint4";
        break;
    case DataFormatId::Vec4UInt64:
        result = "ulong4";
        break;
    case DataFormatId::NumberOfFormats:
        break;
    }
    return result;
}

void BufferMixerCL::compileKernel() {
    if (kernel_)
        removeKernel(kernel_);

    std::stringstream header;
    header << " #define MIX_T " << dataFormatToOpenCLType(format_) << '\n';
    if (format_->getComponents() > 1 && format_->getNumericType() != NumericType::Float) {
        header << " #define CONVERT_T_TO_FLOAT convert_float" << format_->getComponents() << " \n";
        header << " #define CONVERT_FLOAT_TO_T convert_" << dataFormatToOpenCLType(format_) << "\n";
    } else if (format_->getNumericType() == NumericType::Float){
        //header << " #define CONVERT_T  \n";
        //header << " #define CONVERT_FLOAT_TO_T   \n";
    }

    kernel_ = addKernel("buffermixer.cl", "mixKernel", header.str());
    if (!kernel_) {
        std::string msg("Could not compile kernel in buffermix.cl with header " + header.str());
        throw std::exception(msg.c_str());
    }
       
}

} // namespace

