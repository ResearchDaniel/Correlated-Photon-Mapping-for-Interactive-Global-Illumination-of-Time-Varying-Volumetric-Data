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

#include <modules/uniformgridcl/processors/uniformgrid3dsourceprocessor.h>

#include <inviwo/core/common/factoryutil.h>
#include <inviwo/core/io/datareader.h>              // for DataReaderType
#include <inviwo/core/io/datareaderexception.h>     // for DataReaderException
#include <inviwo/core/ports/outportiterable.h>      // for OutportIterableImpl<>::const_iterator
#include <inviwo/core/processors/processorinfo.h>   // for ProcessorInfo
#include <inviwo/core/processors/processorstate.h>  // for CodeState, CodeState::Stable
#include <inviwo/core/processors/processortags.h>   // for Tags, Tags::CPU
#include <inviwo/core/properties/fileproperty.h>    // for FileProperty
#include <modules/base/processors/datasource.h>     // for DataSource
#include <modules/uniformgridcl/uniformgrid3d.h>

#include <functional>  // for __base
#include <memory>      // for shared_ptr

namespace inviwo {
class InviwoApplication;

// The Class Identifier has to be globally unique. Use a reverse DNS naming scheme
const ProcessorInfo UniformGrid3DSourceProcessor::processorInfo_{
    "org.inviwo.UniformGrid3DSourceProcessor",  // Class identifier
    "Uniform Grid3D Source",                    // Display name
    "UniformGrid3D",                            // Category
    CodeState::Stable,                          // Code state
    Tags::CPU,                                  // Tags
    "Loads a UniformGrid3D or UniformGrid3D Sequence from a given file. "
    "If a UniformGrid3D sequence is loaded a slider to select a volume is shown. "_help };

const ProcessorInfo UniformGrid3DSourceProcessor::getProcessorInfo() const {
    return processorInfo_;
}

UniformGrid3DSourceProcessor::UniformGrid3DSourceProcessor(InviwoApplication* app, const std::filesystem::path& filename)
    : DataSource<UniformGrid3DVector, UniformGrid3DVectorOutport>(util::getDataReaderFactory(app), filename, "UniformGrid3D")
{
    filePath.setDisplayName("UniformGrid3D file");
}

} // namespace

