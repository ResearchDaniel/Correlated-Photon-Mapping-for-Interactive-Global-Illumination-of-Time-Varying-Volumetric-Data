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

#ifndef IVW_DYNAMICVOLUMEDIFFERENCEANALYSIS_H
#define IVW_DYNAMICVOLUMEDIFFERENCEANALYSIS_H

#include <modules/uniformgridcl/uniformgridclmoduledefine.h>
#include <inviwo/core/common/inviwo.h>
#include <inviwo/core/processors/processor.h>
#include <inviwo/core/datastructures/buffer/bufferram.h>
#include <inviwo/core/datastructures/buffer/bufferramprecision.h>
#include <modules/base/properties/sequencetimerproperty.h>
#include <inviwo/core/ports/volumeport.h>
#include <modules/uniformgridcl/uniformgrid3d.h>


namespace inviwo {

/** \docpage{org.inviwo.DynamicVolumeDifferenceAnalysis, Dynamic Volume Difference Analysis}
 * ![](org.inviwo.DynamicVolumeDifferenceAnalysis.png?classIdentifier=org.inviwo.DynamicVolumeDifferenceAnalysis)
 * Analyze time varying data.
 *
 * ### Inports
 *   * __<Inport1>__ <description>.
 *
 * ### Outports
 *   * __<Outport1>__ <description>.
 * 
 * ### Properties
 *   * __<Prop1>__ <description>.
 *   * __<Prop2>__ <description>
 */
using DynamicVolumeInfoDataType = DataFloat32::type;
using DynamicVolumeInfoUniformGrid3D = UniformGrid3D<DynamicVolumeInfoDataType>;
using DynamicVolumeInfoUniformGrid3DVector = std::vector<std::shared_ptr<DynamicVolumeInfoUniformGrid3D>>;
using DynamicVolumeInfoUniformGrid3DInport = DataInport<DynamicVolumeInfoUniformGrid3D>;
using DynamicVolumeInfoUniformGrid3DOutport = DataOutport<DynamicVolumeInfoUniformGrid3D>;
/**
 * \class DynamicVolumeDifferenceAnalysis
 * \brief Analyze time varying data.
 * 
 */
class IVW_MODULE_UNIFORMGRIDCL_API DynamicVolumeDifferenceAnalysis : public Processor { 
public:
    DynamicVolumeDifferenceAnalysis();
    virtual ~DynamicVolumeDifferenceAnalysis() = default;
     
    virtual void process() override;

    virtual const ProcessorInfo getProcessorInfo() const override;
    static const ProcessorInfo processorInfo_;
private:
    VolumeSequenceInport inport_;
    UniformGrid3DVectorOutport outport_;
    IntProperty volumeRegionSize_;
    

};

struct IVW_MODULE_UNIFORMGRIDCL_API VolumeRAMDifferenceAnalysisDispatcher {
    //using type = std::shared_ptr<VolumeRAM>;
    using type = void;
    template <class T>
    void dispatch(const VolumeRepresentation* in, const VolumeRepresentation* next, dvec2 dataRange, double dataOffset, double dataScaling,
        size3_t offset, size3_t region,
        BufferRepresentation* out, size_t outIndex);
};

template <class DataType>
void VolumeRAMDifferenceAnalysisDispatcher::dispatch(const VolumeRepresentation* in, const VolumeRepresentation* next, dvec2 dataRange, double dataOffset, double dataScaling,
    size3_t offset, size3_t region,
    BufferRepresentation* out, size_t outIndex) {

    using T = typename DataType::type;
    using P = typename util::same_extent<T, double>::type;
    const VolumeRAMPrecision<T>* volume = dynamic_cast<const VolumeRAMPrecision<T>*>(in);
    const VolumeRAMPrecision<T>* nextVolume = dynamic_cast<const VolumeRAMPrecision<T>*>(next);
    BufferRAMPrecision<DynamicVolumeInfoDataType>* outVolume = dynamic_cast<BufferRAMPrecision<DynamicVolumeInfoDataType>*>(out);
    if (!volume || !next || !outVolume) return;

    // determine parameters
    const size3_t dataDims{ volume->getDimensions() };

    auto startCoord = offset;
    auto endCoord = glm::min(startCoord + region, dataDims);

    const T* src = static_cast<const T*>(volume->getData());
    const T* srcNext = static_cast<const T*>(nextVolume->getData());
    //DynamicVolumeInfoDataType* dst = static_cast<DynamicVolumeInfoDataType*>(outVolume->getData());
    auto startElementId = startCoord.x + (startCoord.y * dataDims.x) + (startCoord.z * dataDims.x * dataDims.y);
    T minElementA = src[startElementId];
    T maxElementA = src[startElementId];
    P absDiffSum(0);
    P absDiffMax(0);
    for (auto z = startCoord.z; z < endCoord.z; ++z) {
        for (auto y = startCoord.y; y < endCoord.y; ++y) {
            //auto volumePos = (y * dataDims.x) + (z * dataDims.x * dataDims.y);
            //auto volumePosEnd = ((y+1) * dataDims.x) + (z * dataDims.x * dataDims.y);
            //std::pair<T, T> minMaxElemen = std::minmax_element(src + volumePos, src + volumePosEnd, glm::max);
            for (auto x = startCoord.x; x < endCoord.x; ++x) {
                size_t volumePos = x + (y * dataDims.x) + (z * dataDims.x * dataDims.y);
                T valueA = src[volumePos];
                minElementA = glm::min(minElementA, valueA);
                maxElementA = glm::max(maxElementA, valueA);
                T valueB = srcNext[volumePos];
                // Compute data value difference. dataOffset is subtracted so no need to include it
                P diff = dataScaling * (P(valueB) - P(valueA));
                absDiffSum += (glm::abs(diff));
                absDiffMax = glm::max(glm::abs(diff), absDiffMax);
            }
        }
    }
    double dMinElement = dataOffset + dataScaling * (volume->getDataFormat()->valueToNormalizedDouble(&minElementA));
    double dMaxElement = dataOffset + dataScaling * (volume->getDataFormat()->valueToNormalizedDouble(&maxElementA));
    //DynamicVolumeInfoDataType minMaxVal(dMinElement*(std::numeric_limits<unsigned short>::max()), dMaxElement*(std::numeric_limits<unsigned short>::max()));
    //outVolume->getDataFormat()->vec2DoubleToValue(dvec2(dMinElement, dMaxElement), &minMaxVal);
    //outVolume->set(outIndex, minMaxVal);
    //outVolume->set(outIndex, static_cast<float>(util::accumulate(0.0, absDiffSum, [](P a, P b) { return std::max(a, b); })));
    // Mean absolute difference
    outVolume->set(outIndex, util::glm_convert<float, P>((absDiffSum / static_cast<double>(region.x*region.y*region.z) - dataRange.x) / (dataRange.y - dataRange.x)));

    // Maximum absolute difference
    //outVolume->set(outIndex, util::glm_convert<float, P>((absDiffMax / static_cast<double>(region.x*region.y*region.z) - dataRange.x) / (dataRange.y - dataRange.x)));
}


} // namespace

#endif // IVW_DYNAMICVOLUMEDIFFERENCEANALYSIS_H

