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

#ifndef IVW_UNIFORMGRID3DPLAYERPROCESSOR_H
#define IVW_UNIFORMGRID3DPLAYERPROCESSOR_H

#include <modules/uniformgridcl/uniformgridclmoduledefine.h>
#include <inviwo/core/common/inviwo.h>
#include <inviwo/core/processors/processor.h>
#include <inviwo/core/properties/ordinalproperty.h>
#include <inviwo/core/properties/boolproperty.h>
#include <inviwo/core/util/timer.h>

#include <modules/uniformgridcl/uniformgrid3d.h>
#include <modules/uniformgridcl/buffermixercl.h>

namespace inviwo {
namespace util {
    struct IVW_MODULE_UNIFORMGRIDCL_API UniformGrid3DMixDispatcher {
        using type = void;
        template <class T>
        void dispatch(const UniformGrid3DBase* x, const UniformGrid3DBase* y, float t, std::shared_ptr<UniformGrid3DBase> out) {
            typedef typename T::type F;
            bufferMixer_.mix(static_cast<const UniformGrid3D<F>*>(x)->data, static_cast<const UniformGrid3D<F>*>(y)->data, t
                , static_cast<UniformGrid3D<F>*>(out.get())->data, nullptr);
        }
        BufferMixerCL bufferMixer_;
    };
}
/** \docpage{org.inviwo.UniformGrid3DPlayerProcessor, Uniform Grid3DPlayer Processor}
 * ![](org.inviwo.UniformGrid3DPlayerProcessor.png?classIdentifier=org.inviwo.UniformGrid3DPlayerProcessor)
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
 * \class UniformGrid3DPlayerProcessor
 * \brief <brief description> 
 * <Detailed description from a developer prespective>
 */
class IVW_MODULE_UNIFORMGRIDCL_API UniformGrid3DPlayerProcessor : public Processor { 
public:
    UniformGrid3DPlayerProcessor();
    virtual ~UniformGrid3DPlayerProcessor() = default;
     
    virtual void process() override;

    void onSequenceTimerEvent();
    void updateVolumeIndex();
    void onTimeStepChange();
    virtual const ProcessorInfo getProcessorInfo() const override;
    static const ProcessorInfo processorInfo_;

private:
    UniformGrid3DVectorInport inport_;
    UniformGrid3DOutport outport_;

    std::shared_ptr<UniformGrid3DBase> outData_;
    std::shared_ptr<UniformGrid3DBase> outDataPingPong_;
    FloatProperty time_;
    IntProperty index_;
    FloatProperty timePerElement_;
    IntProperty frameRate_;
    BoolProperty playSequence_;

    Timer sequenceTimer_;

    util::UniformGrid3DMixDispatcher bufferMixer_;
};


} // namespace

#endif // IVW_UNIFORMGRID3DPLAYERPROCESSOR_H

