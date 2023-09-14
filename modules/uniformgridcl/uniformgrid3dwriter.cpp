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

#include "uniformgrid3dwriter.h"
#include <inviwo/core/io/datawriterexception.h>

#include <fmt/std.h>

namespace inviwo {
    
UniformGrid3DWriter::UniformGrid3DWriter(): DataWriterType<UniformGrid3DVector>() {
    addExtension(FileExtension("u3d", "Uniform grid 3D"));
}

UniformGrid3DWriter::UniformGrid3DWriter(const UniformGrid3DWriter& rhs) = default;

UniformGrid3DWriter& UniformGrid3DWriter::operator=(const UniformGrid3DWriter& that) = default;

UniformGrid3DWriter* UniformGrid3DWriter::clone() const { return new UniformGrid3DWriter(*this); }

void UniformGrid3DWriter::writeData(const UniformGrid3DVector* vectorData,
    const std::filesystem::path& filePath) const {
    if (vectorData->size() < 1) {
        throw DataWriterException("Error: Cannot write empty vector", IvwContext);
    }
    auto rawPath = filePath;
    rawPath.replace_extension("raw");

    auto overwrite = getOverwrite();
    DataWriter::checkOverwrite(filePath, overwrite);
    DataWriter::checkOverwrite(rawPath, overwrite);
 
    std::string fileName = filePath.stem().string();
    
    auto data = vectorData->front().get();
    // Write the header file content
    std::stringstream ss;
    auto modelMatrix = glm::transpose(data->getModelMatrix());
    auto worldMatrix = glm::transpose(data->getWorldMatrix());
    auto structuredGridDim = data->getDimensions();
    auto cellDim = data->getCellDimension();
    
    writeKeyToString(ss, "RawFile", fileName + ".raw");
    writeKeyToString(ss, "Resolution", size4_t(data->getDimensions(), vectorData->size()));
    writeKeyToString(ss, "Format", data->getDataFormat()->getString());
    writeKeyToString(ss, "ModelMatrix", modelMatrix);
    writeKeyToString(ss, "WorldMatrix", worldMatrix);
    writeKeyToString(ss, "CellDimensions", cellDim);
    
    std::ofstream f(filePath.c_str());
    
    if (f.good()) {
        f << ss.str();
    }
    else {
        throw DataWriterException(IVW_CONTEXT_CUSTOM("UniformGrid3DWriter::writeData"),
            "Could not write to file: {}", filePath);
    }
    
    f.close();
    
    std::fstream fout(rawPath.c_str(), std::ios::out | std::ios::binary);
    
    if (fout.good()) {
        for (auto element : *vectorData) {
            fout.write((char*)element->getData(), element->getSizeInBytes());
        }
        
    }
    else {
        throw DataWriterException(IVW_CONTEXT_CUSTOM("UniformGrid3DWriter::writeData"),
            "Could not write to raw file: {}", rawPath);
    }
    
    fout.close();
}

}  // namespace inviwo

