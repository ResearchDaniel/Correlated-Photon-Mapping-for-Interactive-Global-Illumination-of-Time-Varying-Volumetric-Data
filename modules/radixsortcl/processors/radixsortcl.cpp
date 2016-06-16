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

//#include "../src/tune.h"
#include "radixsortcl.h"
#include <warn/push>
#include <warn/ignore/conversion>
#include <warn/ignore/dll-interface-base>
#include <warn/ignore/unused-variable>

#include <warn/pop>
#include <inviwo/core/datastructures/buffer/bufferramprecision.h>
#include <modules/opencl/buffer/buffercl.h>
#include <modules/opencl/syncclgl.h>




namespace inviwo {

const ProcessorInfo RadixSortCL::processorInfo_{
    "org.inviwo.RadixSortCL",  // Class identifier
    "RadixSortCL",             // Display name
    "Sorting",                 // Category
    CodeState::Experimental,   // Code state
    Tags::None,                // Tags
};
const ProcessorInfo RadixSortCL::getProcessorInfo() const {
    return processorInfo_;
}

clogs::Type dataFormatToClogsType(const DataFormatBase* dataFormat) {
    clogs::Type result;
    switch (dataFormat->getId()) {
    case DataFormatId::NotSpecialized:
        break;
    case DataFormatId::Float16:
        result = clogs::Type(clogs::BaseType::TYPE_HALF, 1);
        break;
    case DataFormatId::Float32:
        result = clogs::Type(clogs::BaseType::TYPE_FLOAT, 1);
        break;
    case DataFormatId::Float64:
        result = clogs::Type(clogs::BaseType::TYPE_DOUBLE, 1);
        break;
    case DataFormatId::Int8:
        result = clogs::Type(clogs::BaseType::TYPE_CHAR, 1);
        break;
    case DataFormatId::Int16:
        result = clogs::Type(clogs::BaseType::TYPE_SHORT, 1);
        break;
    case DataFormatId::Int32:
        result = clogs::Type(clogs::BaseType::TYPE_INT, 1);
        break;
    case DataFormatId::Int64:
        result = clogs::Type(clogs::BaseType::TYPE_LONG, 1);
        break;
    case DataFormatId::UInt8:
        result = clogs::Type(clogs::BaseType::TYPE_UCHAR, 1);
        break;
    case DataFormatId::UInt16:
        result = clogs::Type(clogs::BaseType::TYPE_USHORT, 1);
        break;
    case DataFormatId::UInt32:
        result = clogs::Type(clogs::BaseType::TYPE_UINT, 1);
        break;
    case DataFormatId::UInt64:
        result = clogs::Type(clogs::BaseType::TYPE_ULONG, 1);
        break;
    case DataFormatId::Vec2Float16:
        result = clogs::Type(clogs::BaseType::TYPE_HALF, 2);
        break;
    case DataFormatId::Vec2Float32:
        result = clogs::Type(clogs::BaseType::TYPE_FLOAT, 2);
        break;
    case DataFormatId::Vec2Float64:
        result = clogs::Type(clogs::BaseType::TYPE_DOUBLE, 2);
        break;
    case DataFormatId::Vec2Int8:
        result = clogs::Type(clogs::BaseType::TYPE_CHAR, 2);
        break;
    case DataFormatId::Vec2Int16:
        result = clogs::Type(clogs::BaseType::TYPE_SHORT, 2);
        break;
    case DataFormatId::Vec2Int32:
        result = clogs::Type(clogs::BaseType::TYPE_INT, 2);
        break;
    case DataFormatId::Vec2Int64:
        result = clogs::Type(clogs::BaseType::TYPE_LONG, 2);
        break;
    case DataFormatId::Vec2UInt8:
        result = clogs::Type(clogs::BaseType::TYPE_UCHAR, 2);
        break;
    case DataFormatId::Vec2UInt16:
        result = clogs::Type(clogs::BaseType::TYPE_USHORT, 2);
        break;
    case DataFormatId::Vec2UInt32:
        result = clogs::Type(clogs::BaseType::TYPE_UINT, 2);
        break;
    case DataFormatId::Vec2UInt64:
        result = clogs::Type(clogs::BaseType::TYPE_ULONG, 2);
        break;
    case DataFormatId::Vec3Float16:
        result = clogs::Type(clogs::BaseType::TYPE_HALF, 3);
        break;
    case DataFormatId::Vec3Float32:
        result = clogs::Type(clogs::BaseType::TYPE_FLOAT, 3);
        break;
    case DataFormatId::Vec3Float64:
        result = clogs::Type(clogs::BaseType::TYPE_DOUBLE, 3);
        break;
    case DataFormatId::Vec3Int8:
        result = clogs::Type(clogs::BaseType::TYPE_CHAR, 3);
        break;
    case DataFormatId::Vec3Int16:
        result = clogs::Type(clogs::BaseType::TYPE_SHORT, 3);
        break;
    case DataFormatId::Vec3Int32:
        result = clogs::Type(clogs::BaseType::TYPE_INT, 3);
        break;
    case DataFormatId::Vec3Int64:
        result = clogs::Type(clogs::BaseType::TYPE_LONG, 3);
        break;
    case DataFormatId::Vec3UInt8:
        result = clogs::Type(clogs::BaseType::TYPE_UCHAR, 3);
        break;
    case DataFormatId::Vec3UInt16:
        result = clogs::Type(clogs::BaseType::TYPE_USHORT, 3);
        break;
    case DataFormatId::Vec3UInt32:
        result = clogs::Type(clogs::BaseType::TYPE_UINT, 3);
        break;
    case DataFormatId::Vec3UInt64:
        result = clogs::Type(clogs::BaseType::TYPE_ULONG, 3);
        break;
    case DataFormatId::Vec4Float16:
        result = clogs::Type(clogs::BaseType::TYPE_HALF, 4);
        break;
    case DataFormatId::Vec4Float32:
        result = clogs::Type(clogs::BaseType::TYPE_FLOAT, 4);
        break;
    case DataFormatId::Vec4Float64:
        result = clogs::Type(clogs::BaseType::TYPE_DOUBLE, 4);
        break;
    case DataFormatId::Vec4Int8:
        result = clogs::Type(clogs::BaseType::TYPE_CHAR, 4);
        break;
    case DataFormatId::Vec4Int16:
        result = clogs::Type(clogs::BaseType::TYPE_SHORT, 4);
        break;
    case DataFormatId::Vec4Int32:
        result = clogs::Type(clogs::BaseType::TYPE_INT, 4);
        break;
    case DataFormatId::Vec4Int64:
        result = clogs::Type(clogs::BaseType::TYPE_LONG, 4);
        break;
    case DataFormatId::Vec4UInt8:
        result = clogs::Type(clogs::BaseType::TYPE_UCHAR, 4);
        break;
    case DataFormatId::Vec4UInt16:
        result = clogs::Type(clogs::BaseType::TYPE_USHORT, 4);
        break;
    case DataFormatId::Vec4UInt32:
        result = clogs::Type(clogs::BaseType::TYPE_USHORT, 4);
        break;
    case DataFormatId::Vec4UInt64:
        result = clogs::Type(clogs::BaseType::TYPE_ULONG, 4);
        break;
    case DataFormatId::NumberOfFormats:
        break;
    }
    return result;
}

RadixSortCL::RadixSortCL()
    : keysPort_("unsortedKeys")
    , inputPort_("unsortedData")
    , outputPort_("sortedData")
    , radixSort_(NULL) 
    , prevKeysCL_(NULL)
    , prevDataCL_(NULL) {
    addPort(keysPort_);
    addPort(inputPort_);
    addPort(outputPort_);
}

void RadixSortCL::process() {

    IVW_OPENCL_PROFILING(profilingEvent, "")
    try {
        SyncCLGL syncGL;
        const BufferCLGL* keysCL = keysPort_.getData()->getRepresentation<BufferCLGL>();
        const BufferCLGL* dataCL = inputPort_.getData()->getRepresentation<BufferCLGL>();

        syncGL.addToAquireGLObjectList(dataCL);
        syncGL.addToAquireGLObjectList(keysCL);
        syncGL.aquireAllObjects();
        if (radixSort_ == NULL || prevKeysCL_ != keysCL || prevDataCL_ != dataCL) {
            delete radixSort_;
            radixSort_ = new clogs::Radixsort(OpenCL::getPtr()->getContext(), OpenCL::getPtr()->getDevice(), dataFormatToClogsType(keysCL->getDataFormat()), dataFormatToClogsType(dataCL->getDataFormat()));
            radixSort_->setTemporaryBuffers(cl::Buffer(OpenCL::getPtr()->getContext(), CL_MEM_READ_WRITE, keysCL->getSize()*keysCL->getSizeOfElement(), NULL), 
                                            cl::Buffer(OpenCL::getPtr()->getContext(), CL_MEM_READ_WRITE, dataCL->getSize()*dataCL->getSizeOfElement(), NULL));
            prevKeysCL_ = keysCL;
            prevDataCL_ = dataCL;
        }
        radixSort_->enqueue(OpenCL::getPtr()->getQueue(), keysCL->get(), dataCL->get(), static_cast<unsigned int>(keysCL->getSize()), 0, NULL, profilingEvent);
    } catch (std::invalid_argument& e) {
        LogError(e.what());
    } catch (clogs::InternalError& e) {
        LogError(e.what());
    } catch (cl::Error& e) {
        LogError(errorCodeToString(e.err()));
    }
    
    //const vec4* data = static_cast<const vec4*>(inputPort_.getData()->getRepresentation<BufferRAM>()->getData());
    //const unsigned int* keysData = static_cast<const unsigned int*>(keysPort_.getData()->getRepresentation<BufferRAM>()->getData());
    //LogInfo("After sorting");
    //std::stringstream ss;
    //for (int i = 0; i < keysPort_.getData()->getSize(); ++i) {
    //    ss << i << " key key: " << keysData[i]  << " Data: " << glm::to_string(data[i]) << std::endl;
    //    //LogInfo(i << " key key: " << keysData[i]  << " Data: " << glm::to_string(data[i]));
    //}
    //LogInfo(ss.str());

    // Passthrough input to avoid copying all input 
    // This is only done for performance and does not comply with
    // standard inviwo usage pattern
    outputPort_.setData(inputPort_.getData());
}


} // namespace

