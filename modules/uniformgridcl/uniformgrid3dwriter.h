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

#ifndef IVW_UNIFORMGRID3DWRITER_H
#define IVW_UNIFORMGRID3DWRITER_H

#include <modules/uniformgridcl/uniformgridclmoduledefine.h>
#include <inviwo/core/common/inviwo.h>
#include <inviwo/core/io/datawriter.h>
#include <inviwo/core/util/filesystem.h>
#include <modules/uniformgridcl/uniformgrid3d.h>

namespace inviwo {
    
/**
 * \class UniformGrid3DWriter
 * \brief VERY_BRIEFLY_DESCRIBE_THE_CLASS
 * DESCRIBE_THE_CLASS
 */
class IVW_MODULE_UNIFORMGRIDCL_API UniformGrid3DWriter: public DataWriterType<UniformGrid3DVector> {
public:
    UniformGrid3DWriter();
    UniformGrid3DWriter(const UniformGrid3DWriter& rhs);
    UniformGrid3DWriter& operator=(const UniformGrid3DWriter& that);
    virtual UniformGrid3DWriter* clone() const;
    virtual ~UniformGrid3DWriter() = default;
    
    virtual void writeData(const UniformGrid3DVector* data, const std::filesystem::path& path) const;
    template<typename S>
    void writeKeyToString(std::stringstream& ss, const std::string& key, const S& vec) const {
        ss << key << ": " << vec << std::endl;
    }
    
    template<typename S>
    void writeKeyToString(std::stringstream& ss, const std::string& key, const glm::tvec2<S, glm::defaultp>& vec) const {
        ss << key << ": " << vec.x << " " << vec.y << std::endl;
    }
    template<typename S>
    void writeKeyToString(std::stringstream& ss, const std::string& key, const glm::tvec3<S, glm::defaultp>& vec) const{
        ss << key << ": " << vec.x << " " << vec.y << " " << vec.z << std::endl;
    }
    template<typename S>
    void writeKeyToString(std::stringstream& ss, const std::string& key, const glm::tvec4<S, glm::defaultp>& vec) const{
        ss << key << ": " << vec.x << " " << vec.y << " " << vec.z << " " << vec.w << std::endl;
    }
    template<typename S>
    void writeKeyToString(std::stringstream& ss, const std::string& key, const glm::tmat4x4<S, glm::defaultp>& mat) const{
        ss << key << ":";
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                ss << " " << mat[i][j];
            }
            ss << std::endl;
        }
    }
    
    void writeKeyToString(std::stringstream& ss, const std::string& key, const std::string& str) const{
        ss << key << ": " << str << std::endl;
    }
};

} // namespace

#endif // IVW_UNIFORMGRID3DWRITER_H
