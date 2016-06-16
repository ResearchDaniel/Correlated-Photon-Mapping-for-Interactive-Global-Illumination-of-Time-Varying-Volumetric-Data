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

#ifndef IVW_RADIXSORTCL_H
#define IVW_RADIXSORTCL_H

#include <modules/radixsortcl/radixsortclmoduledefine.h>
#include <clogs/clogs.h>
#include <inviwo/core/common/inviwo.h>
#include <inviwo/core/ports/bufferport.h>
#include <inviwo/core/processors/processor.h>
#include <inviwo/core/properties/buttonproperty.h>
#include <modules/opencl/buffer/bufferclbase.h>



namespace inviwo {

IVW_MODULE_RADIXSORTCL_API clogs::Type dataFormatToClogsType(const DataFormatBase* dataFormat);

/** \docpage{org.inviwo.RadixSortCL, RadixSortCL}
 * ![](org.inviwo.RadixSortCL.png?classIdentifier=org.inviwo.RadixSortCL)
 *
 * Sort data in ascending order based on keys. 
 * Uses default OpenCL device to perform radix sort.
 * 
 * ### Inports
 *   * __keysPort___ Keys to sort
 *   * __unsortedData__ Data belonging to keys
 * 
 * ### Outports
 *   * __sortedData__ Sorted data
 * 
 * ### Properties
 *
 */
class IVW_MODULE_RADIXSORTCL_API RadixSortCL : public Processor { 
public:
    RadixSortCL();
    virtual ~RadixSortCL() {}

    virtual const ProcessorInfo getProcessorInfo() const override;
    static const ProcessorInfo processorInfo_;

protected:
    virtual void process();

    BufferInport keysPort_; ///< Keys to sort
    BufferInport inputPort_; ///< Data belonging to keys
    BufferOutport outputPort_; ///< Sorted data

    // Sorting algorithm
    clogs::Radixsort* radixSort_;
    // Cached values to determine if input changed
    const BufferCLBase* prevKeysCL_;
    const BufferCLBase* prevDataCL_; 
};

} // namespace

#endif // IVW_RADIXSORTCL_H

