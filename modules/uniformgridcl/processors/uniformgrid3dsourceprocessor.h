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

#ifndef IVW_UNIFORMGRID3DSOURCEPROCESSOR_H
#define IVW_UNIFORMGRID3DSOURCEPROCESSOR_H

#include <modules/uniformgridcl/uniformgridclmoduledefine.h>
#include <inviwo/core/common/inviwo.h>
#include <inviwo/core/processors/processor.h>
#include <inviwo/core/properties/ordinalproperty.h>
#include <inviwo/core/properties/fileproperty.h>
#include <inviwo/core/properties/buttonproperty.h>
#include <modules/base/properties/sequencetimerproperty.h>
#include <modules/uniformgridcl/uniformgrid3d.h>

namespace inviwo {

/** \docpage{org.inviwo.UniformGrid3DSourceProcessor, Uniform Grid3DSource Processor}
 * ![](org.inviwo.UniformGrid3DSourceProcessor.png?classIdentifier=org.inviwo.UniformGrid3DSourceProcessor)
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
 * \class UniformGrid3DSourceProcessor
 * \brief <brief description> 
 * <Detailed description from a developer prespective>
 */
class IVW_MODULE_UNIFORMGRIDCL_API UniformGrid3DSourceProcessor : public Processor { 
public:
    UniformGrid3DSourceProcessor();
    virtual ~UniformGrid3DSourceProcessor() = default;
     
    virtual void process() override;

    virtual const ProcessorInfo getProcessorInfo() const override;
    static const ProcessorInfo processorInfo_;
    virtual void deserialize(Deserializer& d) override;

private:
    void load(bool deserialize = false);
    void addFileNameFilters();

    std::shared_ptr<UniformGrid3DVector> volumes_;

    UniformGrid3DVectorOutport outport_;
    FileProperty file_;
    ButtonProperty reload_;

    SequenceTimerProperty elementSelector_;
    bool isDeserializing_;
};

} // namespace

#endif // IVW_UNIFORMGRID3DSOURCEPROCESSOR_H

