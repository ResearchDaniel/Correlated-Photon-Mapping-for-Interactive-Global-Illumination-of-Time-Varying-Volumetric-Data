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

#include "uniformgrid3dexport.h"

 // add by myself
#ifndef  _My_env
#define	 _My_env
#include "inviwo/core/util/filesystem.h"
#include "inviwo/core/io/datareaderfactory.h"
#include "inviwo/core/common/inviwoapplication.h"
#include "inviwo/core/io/datawriterfactory.h"
#include "inviwo/core/properties/buttonproperty.h"
#endif // ! _My_env


namespace inviwo {

// The Class Identifier has to be globally unique. Use a reverse DNS naming scheme
const ProcessorInfo UniformGrid3DExport::processorInfo_{
    "org.inviwo.UniformGrid3DExport",      // Class identifier
    "Uniform Grid 3D Export",                // Display name
    "UniformGrid3D",              // Category
    CodeState::Experimental,  // Code state
    Tags::CPU,               // Tags
};
const ProcessorInfo UniformGrid3DExport::getProcessorInfo() const {
    return processorInfo_;
}

UniformGrid3DExport::UniformGrid3DExport()
    : Processor()
    , inport_("UniformGrids")
    , file_("volumeFileName", "Uniform grid 3D file name",
    filesystem::getPath(PathType::Volumes, "/newvolume.u3d"), "Uniform grid 3D")
    , exportButton_("export", "Export", InvalidationLevel::Valid)
    , overwrite_("overwrite", "Overwrite", false) {
    
	// own code 
	/*std::vector<FileExtension> ext;
	for (auto& ext :
		InviwoApplication::getPtr()->getDataWriterFactory()->getExtensionsForType<UniformGrid3DVector>()) {
		std::stringstream ss;
		ss << ext.description_ << " (*." << ext.extension_ << ")";
		file_.addNameFilter(ss.str());
	}*/

    for (auto& ext :
        InviwoApplication::getPtr()->getDataWriterFactory()->getExtensionsForType<UniformGrid3DVector>()) {
        std::stringstream ss;
        ss << ext.description_ << " (*." << ext.extension_ << ")";
        file_.addNameFilter(ss.str());
    }

    addPort(inport_);
    addProperty(file_);
    file_.setAcceptMode(FileProperty::AcceptMode::Save);
    exportButton_.onChange(this, &UniformGrid3DExport::exportData);
    addProperty(exportButton_);
    addProperty(overwrite_);
}
    
void UniformGrid3DExport::process() {
    //outport_.setData(myImage);
}

void UniformGrid3DExport::exportData() {
    auto data = inport_.getData();

    if (data && !file_.get().empty()) {
        std::string fileExtension = filesystem::getFileExtension(file_.get());
        auto factory = getNetwork()->getApplication()->getDataWriterFactory();
        if (auto writer = factory->getWriterForTypeAndExtension<UniformGrid3DVector>(fileExtension)) {
            try {
                writer->setOverwrite(overwrite_.get());               
                writer->writeData(data.get(), file_.get());
                LogInfo("Volume exported to disk: " << file_.get());
            } catch (DataWriterException const& e) {
                util::log(e.getContext(), e.getMessage(), LogLevel::Error);
            }
        } else {
            LogError("Error: Cound not find a writer for the specified extension and data type");
        }
    } else if (file_.get().empty()) {
        LogWarn("Error: Please specify a file to write to");
    } else if (!data) {
        LogWarn("Error: Please connect a volume to export");
    }
}

} // namespace

