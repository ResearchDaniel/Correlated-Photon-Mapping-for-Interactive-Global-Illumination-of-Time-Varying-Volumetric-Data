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

#include "dynamicvolumedifferenceanalysis.h"
#include <inviwo/core/datastructures/volume/volumeram.h>

namespace inviwo {

// The Class Identifier has to be globally unique. Use a reverse DNS naming scheme
const ProcessorInfo DynamicVolumeDifferenceAnalysis::processorInfo_{
    "org.inviwo.DynamicVolumeDifferenceAnalysis",      // Class identifier
    "Dynamic Volume Difference Analysis",                // Display name
    "Volume",              // Category
    CodeState::Experimental,  // Code state
    Tags::CPU,               // Tags
};
const ProcessorInfo DynamicVolumeDifferenceAnalysis::getProcessorInfo() const {
    return processorInfo_;
}

DynamicVolumeDifferenceAnalysis::DynamicVolumeDifferenceAnalysis()
    : Processor()
    , inport_("data")
    , outport_("DynamicDataInfo")
    , volumeRegionSize_("region", "Region size", 8, 1, 100) {
    
    addPort(inport_);
    addPort(outport_);

    addProperty(volumeRegionSize_);

}
    
void DynamicVolumeDifferenceAnalysis::process() {
    auto data = inport_.getData();
    auto output = std::make_shared<UniformGrid3DVector>();
    for (auto timeStep = 0; timeStep < data->size(); ++timeStep) {
        auto nextTimeStep = (timeStep + 1) % data->size();
    
        auto curVolume = (*data)[timeStep];
        auto nextVolume = (*data)[nextTimeStep];

    
        const VolumeRAM* curRAMVolume = curVolume->getRepresentation<VolumeRAM>();
        auto nextRAMVolume = nextVolume->getRepresentation<VolumeRAM>();
        auto dim = curVolume->getDimensions();
        auto region = size3_t(volumeRegionSize_.get());
        const size3_t outDim{ glm::ceil(vec3(dim) / static_cast<float>(volumeRegionSize_.get())) };

        std::shared_ptr<DynamicVolumeInfoUniformGrid3D> out = std::make_shared<DynamicVolumeInfoUniformGrid3D>(region);
        // Use same transformation to make sure that they are render at the same location
        out->setModelMatrix(curVolume->getModelMatrix());
        out->setWorldMatrix(curVolume->getWorldMatrix());
        out->setDimensions(outDim);
        auto outRAM = out->data.getEditableRepresentation<BufferRAM>();

        dvec2 dataRange = curVolume->dataMap_.dataRange;
        DataMapper defaultRange(curVolume->getDataFormat());

        double invRange = 1.0 / (dataRange.y - dataRange.x);
        double defaultToDataRange = (defaultRange.dataRange.y - defaultRange.dataRange.x) * invRange;
        double defaultToDataOffset = (dataRange.x - defaultRange.dataRange.x) /
            (defaultRange.dataRange.y - defaultRange.dataRange.x);
    
        for (int z = 0; z < outDim.z; ++z) {
            for (int y = 0; y < outDim.y; ++y) {
                for (int x = 0; x < outDim.x; ++x) {
                    auto offset = size3_t(x, y, z)*region;
                    VolumeRAMDifferenceAnalysisDispatcher disp;
                    curVolume->getDataFormat()->dispatch(disp, curRAMVolume, nextRAMVolume, dataRange, defaultToDataOffset, defaultToDataRange, offset, region, outRAM, VolumeRAM::posToIndex(size3_t(x, y, z), outDim));
                }
            }
        }
        output->emplace_back(out);
    }

    outport_.setData(output);
}

} // namespace

