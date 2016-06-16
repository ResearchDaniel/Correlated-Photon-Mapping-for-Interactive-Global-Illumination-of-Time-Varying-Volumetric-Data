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

#include "uniformgrid3dplayerprocessor.h"

namespace inviwo {

// The Class Identifier has to be globally unique. Use a reverse DNS naming scheme
const ProcessorInfo UniformGrid3DPlayerProcessor::processorInfo_{
    "org.inviwo.UniformGrid3DPlayerProcessor",      // Class identifier
    "Uniform Grid 3D Player Processor",                // Display name
    "UniformGrid3D",              // Category
    CodeState::Experimental,  // Code state
    Tags::CL,               // Tags
};
const ProcessorInfo UniformGrid3DPlayerProcessor::getProcessorInfo() const {
    return processorInfo_;
}

UniformGrid3DPlayerProcessor::UniformGrid3DPlayerProcessor()
    : Processor()
    , inport_("Sequence")
    , outport_("InterpolatedData")
    , time_("time", "Time", 0.f, 0.f, 0.f)
    , index_("selectedSequenceIndex", "Sequence index", 1, 1, 1)
    , timePerElement_("timePerElement", "Time Per element (s)", 1.f, 0.01f, 10.f, 0.01f)
    , playSequence_("playSequence", "Play Sequence", false)
    , frameRate_("frameRate", "Frame rate", 10, 1, 60, 1, InvalidationLevel::Valid)
    , sequenceTimer_(1000 / frameRate_.get(), [this](){ onSequenceTimerEvent(); }) {
    
    addPort(inport_);
    inport_.onChange([this]() {
        onTimeStepChange();

    });
    addPort(outport_);
    addProperty(time_);
    time_.onChange([this](){ updateVolumeIndex(); });
    addProperty(index_);
    index_.setReadOnly(true);
    addProperty(timePerElement_);
    timePerElement_.onChange([this]() {
        onTimeStepChange();
    });

    addProperty(frameRate_);
    frameRate_.onChange([this]() { sequenceTimer_.setInterval(1000 / frameRate_.get()); });
    addProperty(playSequence_);
    playSequence_.onChange([this]() {
        time_.setReadOnly(playSequence_);

        if (playSequence_) {
            sequenceTimer_.setInterval(1000 / frameRate_.get());
            sequenceTimer_.start();
        } else {
            sequenceTimer_.stop();
        }
    });
}
    
void UniformGrid3DPlayerProcessor::process() {
    auto elements = inport_.getData();
    float integerTime;
    // Time between two volumes
    float t = std::modf(time_ / timePerElement_, &integerTime);
    auto timeStep = index_ - 1;
    auto nextTimeStep = (timeStep + 1) % elements->size();
    if (elements->size() > 1) {
        std::swap(outData_, outDataPingPong_);
        auto input0 = elements->at(timeStep);
        auto input1 = elements->at(nextTimeStep);
        if (!outData_ || outData_->getDimensions() != input0->getDimensions()
            || outData_->getDataFormat() != input0->getDataFormat()) {
            outData_ = std::shared_ptr<UniformGrid3DBase>(input0->clone());
            //outData_ = input0->getDataFormat()->dispatch(util::UniformGrid3DDispatcher(), input0->getDimensions(), input0->getCellDimension(), BufferUsage::Static);
            outData_->setModelMatrix(input0->getModelMatrix());
            outData_->setWorldMatrix(input0->getWorldMatrix());
            // pass meta data on
            //outData_->copyMetaDataFrom(*input0);
            //outData_->dataMap_ = input0->dataMap_;

        }
        input0->getDataFormat()->dispatch(bufferMixer_, input0.get(), input1.get(), t, outData_);
        //bufferMixer_.mix(*input0->dataget(), *input1, t, *outData_, nullptr);
        outport_.setData(outData_);
    } else {
        outport_.setData(elements->at(timeStep));
    }
}

void UniformGrid3DPlayerProcessor::onSequenceTimerEvent() {
    auto time = time_.get();
    time = time + static_cast<float>(1000 / frameRate_.get()) / 1000.f;
    // Wrap around time
    if (time > time_.getMaxValue()) {
        time -= time_.getMaxValue();
    }
    time_.set(time);
    updateVolumeIndex();


}

void UniformGrid3DPlayerProcessor::updateVolumeIndex() {
    float integerTime;
    // Time between two volumes
    auto timeStep = static_cast<size_t>(integerTime) % index_.getMaxValue();
    if (timeStep != (index_-1)) {
        index_.set(static_cast<int>(timeStep + 1));
    }
}

void UniformGrid3DPlayerProcessor::onTimeStepChange() {
    if (inport_.hasData()) {
        auto volumes = inport_.getData();
        time_.setMaxValue(time_.getMinValue() + static_cast<float>(volumes->size() - 1) * timePerElement_);
        if (time_ > time_.getMaxValue()) {
            time_.set(time_.getMinValue());
        }
        index_.setMaxValue(static_cast<int>(volumes->size()));
        if (index_ > index_.getMaxValue()) {
            index_.set(index_.getMinValue());
        }
    }
}


} // namespace

