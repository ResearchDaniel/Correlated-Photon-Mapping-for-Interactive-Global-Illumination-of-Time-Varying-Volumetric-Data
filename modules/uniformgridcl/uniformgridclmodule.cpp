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

#include <modules/uniformgridcl/uniformgridclmodule.h>
#include <modules/uniformgridcl/uniformgrid3d.h>
#include <modules/uniformgridcl/uniformgrid3dreader.h>
#include <modules/uniformgridcl/uniformgrid3dwriter.h>
#include <modules/uniformgridcl/processors/dynamicvolumedifferenceanalysis.h>
#include <modules/uniformgridcl/processors/uniformgrid3dexport.h>
#include <modules/uniformgridcl/processors/uniformgrid3dplayerprocessor.h>
#include <modules/uniformgridcl/processors/uniformgrid3dsequenceselector.h>
#include <modules/uniformgridcl/processors/uniformgrid3dsourceprocessor.h>
#include <modules/uniformgridcl/processors/volumeminmaxclprocessor.h>
#include <modules/uniformgridcl/processors/volumesequenceplayer.h>


#include <modules/opencl/inviwoopencl.h>
// Autogenerated
#include <modules/uniformgridcl/shader_resources.h>

namespace inviwo {

UniformGridCLModule::UniformGridCLModule(InviwoApplication* app) : InviwoModule(app, "UniformGridCL") {
    
    // Register objects that can be shared with the rest of inviwo here:
    
    // Processors
    registerProcessor<DynamicVolumeDifferenceAnalysis>();
    registerProcessor<UniformGrid3DExport>();
    registerProcessor<UniformGrid3DPlayerProcessor>();
    registerProcessor<UniformGrid3DSequenceSelector>();
    registerProcessor<UniformGrid3DSourceProcessor>();
    registerProcessor<VolumeMinMaxCLProcessor>();
    registerProcessor<VolumeSequencePlayer>();

    registerDataReader(std::make_unique<UniformGrid3DReader>());
    registerDataWriter(std::make_unique<UniformGrid3DWriter>());
    

    registerPort<UniformGrid3DInport>();
    registerPort<UniformGrid3DOutport>();

    // Add a directory to the search path of the KernelManager
    OpenCL::getPtr()->addCommonIncludeDirectory(getPath(ModulePath::CL));
    uniformgridcl::addShaderResources(ShaderManager::getPtr(), { getPath(ModulePath::GLSL) });
}
    
int UniformGridCLModule::getVersion() const { return 1; }

std::unique_ptr<VersionConverter> UniformGridCLModule::getConverter(int version) const {
    return std::make_unique<Converter>(version);
}

UniformGridCLModule::Converter::Converter(int version) : version_(version) {}

bool UniformGridCLModule::Converter::convert(TxElement* root) {
    auto makerules = []() {
        std::vector<xml::IdentifierReplacement> repl = {
            // DynamicVolumeDifferenceAnalysis
            { { xml::Kind::processor("com.inviwo.DynamicVolumeDifferenceAnalysis"),
                xml::Kind::outport("UniformGrid3DBaseSharedPtrVectorOutport") },
                "dynamic data info",
                "DynamicDataInfo" },
            
            // UniformGrid3DExport
            { { xml::Kind::processor("org.inviwo.UniformGrid3DExport"),
                xml::Kind::inport("UniformGrid3DBaseSharedPtrVectorInport") },
                "Uniform grids",
                "UniformGrids" },
            
            // UniformGrid3DPlayerProcessor
            { { xml::Kind::processor("org.inviwo.UniformGrid3DPlayerProcessor"),
                xml::Kind::outport("UniformGrid3DBaseOutport") },
                "Interpolated data",
                "InterpolatedData" },
            
            // VolumeMinMaxCLProcessor
            { { xml::Kind::processor("com.inviwo.VolumeMinMaxCLProcessor"),
                xml::Kind::inport("org.inviwo.VolumeSharedPtrVectorInport") },
                "vector volume",
                "VolumeSequenceInput" },
            { { xml::Kind::processor("com.inviwo.VolumeMinMaxCLProcessor"),
                xml::Kind::outport("UniformGrid3DBaseSharedPtrVectorOutport") },
                "vector output",
                "UniformGrid3DVectorOut" },
            
            // VolumeSequencePlayer
            { { xml::Kind::processor("org.inviwo.VolumeSequencePlayer"),
                xml::Kind::outport("org.inviwo.VolumeOutport") },
                "interpolated volume",
                "InterpolatedVolume" }
            
            
        };
        return repl;
    };
    
    bool res = false;
    switch (version_) {
        case 0: {
            auto repl = makerules();
            res |= xml::changeIdentifiers(root, repl);
        }
        return res;
        
        default:
        return false;  // No changes
    }
    return true;
}
} // namespace
