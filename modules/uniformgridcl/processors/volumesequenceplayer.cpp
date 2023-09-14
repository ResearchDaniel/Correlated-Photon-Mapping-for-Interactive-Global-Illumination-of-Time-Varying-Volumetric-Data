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

#include "volumesequenceplayer.h"

#include <modules/opengl/shader/shaderutils.h>
#include <modules/opengl/texture/textureutils.h>
#include <modules/opengl/texture/textureunit.h>
#include <modules/opengl/volume/volumegl.h>


namespace inviwo {

// The Class Identifier has to be globally unique. Use a reverse DNS naming scheme
const ProcessorInfo VolumeSequencePlayer::processorInfo_{
    "org.inviwo.VolumeSequencePlayer",      // Class identifier
    "Volume Sequence Player",               // Display name
    "Volume",                               // Category
    CodeState::Experimental,                // Code state
    Tags::GL,                               // Tags
};
const ProcessorInfo VolumeSequencePlayer::getProcessorInfo() const {
    return processorInfo_;
}

VolumeSequencePlayer::VolumeSequencePlayer()
: Processor()
, inport_("volumeSequence")
, outport_("InterpolatedVolume")
, shader_("volume_gpu.vert", "volume_gpu.geom", "volume_mix.frag", true)
, time_("time", "Time", 0.f, 0.f, 0.f)
, index_("selectedSequenceIndex", "Sequence index", 1, 1, 1)
, timePerVolume_("timePerVolume", "Time Per Volume (s)", 1.f, 0.01f, 10.f, 0.01f)
, volumesPerSecond_("volumesPerSecond", "Frame rate", 10, 1, 60, 1, InvalidationLevel::Valid)
, sequenceTimer_(Timer::Milliseconds(1000 / volumesPerSecond_.get()), [this](){ onSequenceTimerEvent(); })
, playSequence_("playSequence", "Play Sequence", false)
{
    addPort(inport_);
    inport_.onChange([this]() {
        onTimeStepChange();
        
    });
    addPort(outport_);
    addProperty(time_);
    time_.onChange([this](){ updateVolumeIndex(); });
    addProperty(index_);
    index_.setReadOnly(true);
    addProperty(timePerVolume_);
    timePerVolume_.onChange([this]() {
        onTimeStepChange();
    });
    
    addProperty(volumesPerSecond_);
    volumesPerSecond_.onChange([this]() { sequenceTimer_.setInterval(Timer::Milliseconds(1000 / volumesPerSecond_.get())); });
    addProperty(playSequence_);
    playSequence_.onChange([this]() {
        time_.setReadOnly(playSequence_);
        
        if (playSequence_) {
            sequenceTimer_.setInterval(Timer::Milliseconds(1000 / volumesPerSecond_.get()));
            sequenceTimer_.start();
        } else {
            sequenceTimer_.stop();
        }
    });
}

void VolumeSequencePlayer::process() {
    auto volumes = inport_.getData();
    float integerTime;
    // Time between two volumes
    float t = std::modf(time_ / timePerVolume_, &integerTime);
    auto timeStep = index_ - 1;
    auto nextTimeStep = (timeStep + 1) % volumes->size();
    if (volumes->size() > 1) {
        bool reattach = false;
        auto inputVol0 = volumes->at(timeStep);
        auto inputVol1 = volumes->at(nextTimeStep);
        if (!outVolume_ || outVolume_->getDimensions() != inputVol0->getDimensions()
            || outVolume_->getDataFormat() != inputVol0->getDataFormat()) {
            reattach = true;
            outVolume_ = std::make_shared<Volume>(std::make_shared<VolumeGL>(inputVol0->getDimensions(), inputVol0->getDataFormat()));
            outVolume_->setModelMatrix(inputVol0->getModelMatrix());
            outVolume_->setWorldMatrix(inputVol0->getWorldMatrix());
            // pass meta data on
            outVolume_->copyMetaDataFrom(*inputVol0);
            outVolume_->dataMap_ = inputVol0->dataMap_;
            
        }
        TextureUnit vol0Unit, vol1Unit;
        utilgl::bindTexture(*inputVol0, vol0Unit);
        utilgl::bindTexture(*inputVol1, vol1Unit);
        shader_.activate();
        
        shader_.setUniform("volume", vol0Unit.getUnitNumber());
        utilgl::setShaderUniforms(shader_, *inputVol0, "volumeParameters");
        shader_.setUniform("volume1", vol1Unit.getUnitNumber());
        utilgl::setShaderUniforms(shader_, *inputVol1, "volume1Parameters");
        shader_.setUniform("weight", t);
        fbo_.activate();
        glViewport(0, 0, static_cast<GLsizei>(outVolume_->getDimensions().x), static_cast<GLsizei>(outVolume_->getDimensions().y));
        if (reattach) {
            auto outVolumeGL = outVolume_->getEditableRepresentation<VolumeGL>();
            outVolume_->invalidateAllOther(outVolumeGL);
            fbo_.attachColorTexture(outVolumeGL->getTexture().get(), 0);
        }
        
        utilgl::multiDrawImagePlaneRect(static_cast<int>(outVolume_->getDimensions().z));
        
        shader_.deactivate();
        fbo_.deactivate();
        
        outport_.setData(outVolume_);
    } else {
        outport_.setData(volumes->at(timeStep));
    }
}

void VolumeSequencePlayer::onSequenceTimerEvent() {
    auto time = time_.get();
    time = time + static_cast<float>(1000 / volumesPerSecond_.get())/1000.f;
    // Wrap around time
    if (time > time_.getMaxValue()) {
        time -= time_.getMaxValue();
    }
    time_.set(time);
    updateVolumeIndex();
    
    
}

void VolumeSequencePlayer::updateVolumeIndex() {
    float integerTime;
    // Time between two volumes
    std::modf(time_ / timePerVolume_, &integerTime);
    auto timeStep = static_cast<size_t>(integerTime) % index_.getMaxValue();
    if (timeStep != (index_ - 1)) {
        index_.set(static_cast<int>(timeStep + 1));
    }
}

void VolumeSequencePlayer::onTimeStepChange() {
    if (inport_.hasData()) {
        auto volumes = inport_.getData();
        time_.setMaxValue(time_.getMinValue() + static_cast<float>(volumes->size() - 1) * timePerVolume_);
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

