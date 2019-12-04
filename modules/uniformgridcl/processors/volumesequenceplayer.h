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

#ifndef IVW_VOLUMESEQUENCEPLAYER_H
#define IVW_VOLUMESEQUENCEPLAYER_H

#include <modules/uniformgridcl/uniformgridclmoduledefine.h>
#include <inviwo/core/common/inviwo.h>
#include <inviwo/core/processors/processor.h>
#include <inviwo/core/properties/ordinalproperty.h>
#include <inviwo/core/ports/volumeport.h>

#include <inviwo/core/util/timer.h>

#include <modules/opengl/buffer/framebufferobject.h>
#include <modules/opengl/shader/shader.h>

namespace inviwo {

/** \docpage{org.inviwo.VolumeSequencePlayer, Volume Sequence Player}
 * ![](org.inviwo.VolumeSequencePlayer.png?classIdentifier=org.inviwo.VolumeSequencePlayer)
 * Explanation of how to use the processor.
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


/**
 * \class VolumeSequencePlayer
 * \brief Linearly interpolates between two volumes to create the output at time t.
 */
class IVW_MODULE_UNIFORMGRIDCL_API VolumeSequencePlayer : public Processor {
public:
    VolumeSequencePlayer();
    
    void onTimeStepChange();
    
    virtual ~VolumeSequencePlayer() = default;
    
    virtual void process() override;
    
    virtual const ProcessorInfo getProcessorInfo() const override;
    static const ProcessorInfo processorInfo_;
    
private:
    void onSequenceTimerEvent();
    
    void updateVolumeIndex();
    
    VolumeSequenceInport inport_;
    VolumeOutport outport_;
    
    std::shared_ptr<Volume> outVolume_;
    Shader shader_;
    FrameBufferObject fbo_;
    
    FloatProperty time_;
    IntProperty index_;
    FloatProperty timePerVolume_;
    IntProperty volumesPerSecond_;
    BoolProperty playSequence_;
    
    Timer sequenceTimer_;
};

} // namespace

#endif // IVW_VOLUMESEQUENCEPLAYER_H

