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

#include "uniformgrid3dsourceprocessor.h"
#include <inviwo/core/util/filesystem.h>
#include <inviwo/core/io/datareaderexception.h>
#include <inviwo/core/io/datareaderfactory.h>

namespace inviwo {
    
// The Class Identifier has to be globally unique. Use a reverse DNS naming scheme
const ProcessorInfo UniformGrid3DSourceProcessor::processorInfo_{
    "org.inviwo.UniformGrid3DSourceProcessor",      // Class identifier
    "Uniform Grid3D Source",                // Display name
    "UniformGrid3D",              // Category
    CodeState::Experimental,  // Code state
    Tags::CPU,               // Tags
};

const ProcessorInfo UniformGrid3DSourceProcessor::getProcessorInfo() const {
    return processorInfo_;
}

UniformGrid3DSourceProcessor::UniformGrid3DSourceProcessor()
: Processor()
, outport_("data")
, file_("filename", "File")
, reload_("reload", "Reload data")
, elementSelector_("Sequence", "Sequence")
, isDeserializing_(false) {
    file_.setContentType("UniformGrid3D");
    file_.setDisplayName("UniformGrid3D file");
    
    file_.onChange([this]() { load(); });
    reload_.onChange([this]() { load(); });
    
    elementSelector_.setVisible(false);
    
    addFileNameFilters();
    
    addPort(outport_);
    
    addProperty(file_);
    addProperty(reload_);
    addProperty(elementSelector_);
}
void UniformGrid3DSourceProcessor::load(bool) {
    if (isDeserializing_ || file_.get().empty()) return;
    
    auto rf = InviwoApplication::getPtr()->getDataReaderFactory();
    
    std::string ext = filesystem::getFileExtension(file_.get());
    if (auto volVecReader = rf->getReaderForTypeAndExtension<UniformGrid3DVector>(ext)) {
        try {
            auto volumes = volVecReader->readData(file_.get());
            std::swap(volumes, volumes_);
        } catch (DataReaderException const& e) {
            LogProcessorError("Could not load data: " << file_.get() << ", " << e.getMessage());
        }
    } else if (auto volreader = rf->getReaderForTypeAndExtension<UniformGrid3DBase>(ext)) {
        try {
            auto volume(volreader->readData(file_.get()));
            auto volumes = std::make_shared<UniformGrid3DVector>();
            volumes->push_back(volume);
            std::swap(volumes, volumes_);
        } catch (DataReaderException const& e) {
            LogProcessorError("Could not load data: " << file_.get() << ", " << e.getMessage());
        }
    } else {
        LogProcessorError("Could not find a data reader for file: " << file_.get());
    }
    
    if (volumes_ && !volumes_->empty() && (*volumes_)[0]) {
        elementSelector_.updateMax(volumes_->size());
        elementSelector_.setVisible(volumes_->size() > 1);
    }
}

void UniformGrid3DSourceProcessor::addFileNameFilters() {
    auto rf = InviwoApplication::getPtr()->getDataReaderFactory();
    auto extensions = rf->getExtensionsForType<UniformGrid3DBase>();
    file_.clearNameFilters();
    file_.addNameFilter(FileExtension("*", "All Files"));
    for (auto& ext : extensions) {
        file_.addNameFilter(ext.description_ + " (*." + ext.extension_ + ")");
    }
    extensions = rf->getExtensionsForType<UniformGrid3DVector>();
    for (auto& ext : extensions) {
        file_.addNameFilter(ext.description_ + " (*." + ext.extension_ + ")");
    }
}

void UniformGrid3DSourceProcessor::process() {
    if (!isDeserializing_ && volumes_ && !volumes_->empty()) {
        //size_t index =
        //    std::min(volumes_->size() - 1, static_cast<size_t>(elementSelector_.index_.get() - 1));
        
        //if (!(*volumes_)[index]) return;
        
        //outport_.setData((*volumes_)[index]);
        outport_.setData(volumes_);
    }
}

void UniformGrid3DSourceProcessor::deserialize(Deserializer& d) {
    {
        isDeserializing_ = true;
        Processor::deserialize(d);
        addFileNameFilters();
        isDeserializing_ = false;
    }
    load(true);
}

} // namespace

