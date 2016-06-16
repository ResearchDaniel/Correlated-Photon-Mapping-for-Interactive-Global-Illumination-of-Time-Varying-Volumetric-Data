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

#include "uniformgrid3dvectorsource.h"
#include <inviwo/core/io/datareaderfactory.h>
#include <inviwo/core/util/filesystem.h>
#include <inviwo/core/io/datareaderexception.h>

namespace inviwo {

// The Class Identifier has to be globally unique. Use a reverse DNS naming scheme
const ProcessorInfo UniformGrid3DVectorSource::processorInfo_{
    "org.inviwo.UniformGrid3DVectorSource",      // Class identifier
    "Uniform Grid 3D Vector Source",                // Display name
    "UniformGrid3D",              // Category
    CodeState::Experimental,  // Code state
    Tags::CPU,               // Tags
};

const ProcessorInfo UniformGrid3DVectorSource::getProcessorInfo() const {
    return processorInfo_;
}

UniformGrid3DVectorSource::UniformGrid3DVectorSource()
    : Processor()
    , outport_("data")
    , file_("filename", "File")
    , reload_("reload", "Reload data")
    , isDeserializing_(false) {
    file_.setContentType("UniformGrid3D");
    file_.setDisplayName("UniformGrid3D file");

    file_.onChange([this]() { load(); });
    reload_.onChange([this]() { load(); });

    addFileNameFilters();

    addPort(outport_);

    addProperty(file_);
    addProperty(reload_);
}

void UniformGrid3DVectorSource::load(bool deserialize /*= false*/) {
    if (isDeserializing_ || file_.get().empty()) return;

    auto rf = InviwoApplication::getPtr()->getDataReaderFactory();
    std::string ext = filesystem::getFileExtension(file_.get());
    if (auto reader = rf->getReaderForTypeAndExtension<UniformGrid3DVector>(ext)) {
        try {
            data_ = reader->readData(file_.get());
        } catch (DataReaderException const& e) {
            LogProcessorError("Could not load data: " << file_.get() << ", " << e.getMessage());
        }
    } else {
        LogProcessorError("Could not find a data reader for file: " << file_.get());
    }

}

void UniformGrid3DVectorSource::addFileNameFilters() {
    auto rf = InviwoApplication::getPtr()->getDataReaderFactory();
    auto extensions = rf->getExtensionsForType<UniformGrid3DVector>();
    file_.clearNameFilters();
    file_.addNameFilter(FileExtension("*", "All Files"));
    for (auto& ext : extensions) {
        file_.addNameFilter(ext.description_ + " (*." + ext.extension_ + ")");
    }
}

void UniformGrid3DVectorSource::process() {
    if (!isDeserializing_ && data_ && !data_->empty()) {
        outport_.setData(data_);
    }
}

void UniformGrid3DVectorSource::deserialize(Deserializer& d) {
    {
        isDeserializing_ = true;
        Processor::deserialize(d);
        addFileNameFilters();
        isDeserializing_ = false;
    }
    load(true);
}



} // namespace

