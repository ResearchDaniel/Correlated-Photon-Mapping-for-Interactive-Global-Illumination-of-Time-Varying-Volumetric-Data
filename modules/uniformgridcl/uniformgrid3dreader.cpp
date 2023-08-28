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

#include "uniformgrid3dreader.h"
#include <inviwo/core/io/datareaderexception.h>
#include <inviwo/core/util/filesystem.h>
#include <inviwo/core/util/formatconversion.h>
#include <inviwo/core/util/formats.h>
#include <inviwo/core/util/formatdispatching.h>
#include <inviwo/core/util/raiiutils.h>

#include <fmt/std.h>

namespace inviwo {
    
UniformGrid3DReader::UniformGrid3DReader() : DataReaderType<UniformGrid3DVector>() {
    addExtension(FileExtension("u3d", "Uniform Grid 3D"));
}

UniformGrid3DReader::UniformGrid3DReader(const UniformGrid3DReader& rhs)
: DataReaderType<UniformGrid3DVector>(rhs) {}

UniformGrid3DReader& UniformGrid3DReader::operator=(const UniformGrid3DReader& that) {
    if (this != &that) {
        DataReaderType<UniformGrid3DVector>::operator=(that);
    }
    
    return *this;
}

UniformGrid3DReader* UniformGrid3DReader::clone() const { return new UniformGrid3DReader(*this); }

std::shared_ptr<UniformGrid3DVector> UniformGrid3DReader::readData(const std::filesystem::path& filePath) {
    
    const auto fileDirectory = filePath.parent_path();
    // Read the file content
    auto f = open(filePath);
    std::string textLine;
    std::filesystem::path rawFile_;
    std::string formatFlag = "";
    glm::mat4 modelMatrix(1.0f);
    glm::mat4 worldMatrix(1.0f);
    size3_t cellDimensions(0);
    glm::size4_t resolution(0);
    
    const DataFormatBase* format_ = nullptr;
    
    std::vector<std::string> parts;
    std::string key;
    std::string value;
    
    while (!f.eof()) {
        getline(f, textLine);
        
        textLine = trim(textLine);
        
        if (textLine == "" || textLine[0] == '#' || textLine[0] == '/') continue;
        parts = splitString(splitString(textLine, '#')[0], ':');
        if (parts.size() != 2) continue;
        
        key = toLower(trim(parts[0]));
        value = trim(parts[1]);
        
        std::stringstream ss(value);
        if (key == "objectfilename" || key == "rawfile") {
            rawFile_ = fileDirectory / value;
        } else if (key == "resolution" || key == "dimensions") {
            ss >> resolution.x;
            ss >> resolution.y;
            ss >> resolution.z;
            ss >> resolution.w;
        } else if (key == "format") {
            ss >> formatFlag;
            format_ = DataFormatBase::get(formatFlag);
        } else if (key == "modelmatrix") {
            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j) {
                    ss >> modelMatrix[i][j];
                }
            }
            modelMatrix = glm::transpose(modelMatrix);
        } else if (key == "worldmatrix") {
            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j) {
                    ss >> worldMatrix[i][j];
                }
            }
            worldMatrix = glm::transpose(worldMatrix);
        } else if (key == "celldimensions") {
            ss >> cellDimensions.x;
            ss >> cellDimensions.y;
            ss >> cellDimensions.z;
        }
    };
    
    // Check if other dat files where specified, and then only consider them as a sequence
    auto dataVector = std::make_shared<UniformGrid3DVector>();
    
    if (resolution == size4_t(0)) {
        throw DataReaderException(
            IVW_CONTEXT, "Error: Unable to find \"Resolution\" tag in file: {}", filePath);
    }
    else if (format_ == nullptr) {
        throw DataReaderException(
            IVW_CONTEXT, "Error: Unable to find \"Format\" tag in file: {}", filePath);
    }
    else if (format_->getId() == DataFormatId::NotSpecialized) {
        throw DataReaderException(
            IVW_CONTEXT,
            "Error: Invalid format string found: {} in {} \nThe valid formats are:\n"
            "FLOAT16, FLOAT32, FLOAT64, INT8, INT16, INT32, INT64, UINT8, UINT16, UINT32, "
            "UINT64, Vec2FLOAT16, Vec2FLOAT32, Vec2FLOAT64, Vec2INT8, Vec2INT16, "
            "Vec2INT32, Vec2INT64, Vec2UINT8, Vec2UINT16, Vec2UINT32, Vec2UINT64, "
            "Vec3FLOAT16, Vec3FLOAT32, Vec3FLOAT64, Vec3INT8, Vec3INT16, Vec3INT32, "
            "Vec3INT64, Vec3UINT8, Vec3UINT16, Vec3UINT32, Vec3UINT64, Vec4FLOAT16, "
            "Vec4FLOAT32, Vec4FLOAT64, Vec4INT8, Vec4INT16, Vec4INT32, Vec4INT64, "
            "Vec4UINT8, Vec4UINT16, Vec4UINT32, Vec4UINT64",
            formatFlag, filePath);
    }
    
    std::shared_ptr<UniformGrid3DBase> data =
    dispatching::dispatch<std::shared_ptr<UniformGrid3DBase>, dispatching::filter::All>(format_->getId(), util::UniformGrid3DDispatcher(), size3_t(resolution), size3_t(cellDimensions), BufferUsage::Static);
    
    if (!data) {
        throw DataReaderException(
            IVW_CONTEXT, "Error: Unsupported data fromat \"Format\" tag in file: {}", filePath);
    }
    data->setModelMatrix(modelMatrix);
    data->setWorldMatrix(worldMatrix);
    data->setDimensions(size3_t(resolution));
    
    size_t bytes = resolution.x * resolution.y * resolution.z * (format_->getSize());
    
    std::fstream fin(rawFile_.c_str(), std::ios::in | std::ios::binary);
    util::OnScopeExit close([&fin]() { fin.close(); });
    
    if (fin.good()) {
        for (size_t t = 0; t < resolution.w; ++t) {
            if (t == 0)
            dataVector->push_back(std::move(data));
            else
            dataVector->push_back(
                                  std::shared_ptr<UniformGrid3DBase>(dataVector->front()->clone()));
            
            fin.read(static_cast<char*>(dataVector->back()->getData()), bytes);
        }
    } else {
        throw DataReaderException(
            IVW_CONTEXT, "Error: Unable to read from  file: {}", rawFile_.string());
    }
    
    std::string size = util::formatBytesToString(bytes * resolution.w);
    // if (enableLogOutput_) {
    //    LogInfo("Loaded volume sequence: " << filePath << " size: " << size);
    //}
    return dataVector;
}

}  // namespace inviwo
