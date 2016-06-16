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


#ifndef IVW_UNIFORMGRID3D_H
#define IVW_UNIFORMGRID3D_H

#include <modules/uniformgridcl/uniformgridclmoduledefine.h>
#include <inviwo/core/common/inviwo.h>
#include <inviwo/core/datastructures/spatialdata.h>
#include <inviwo/core/ports/port.h>

namespace inviwo {

/**
 * \class UniformGrid3D
 *
 * \brief Uniform subdivision of the 3D space. 
 * Each grid cell contains information about the data in the grid cell.
 *
 *  _____________
 * |      |      |
 * | Cell |      |
 * |______|______|
 * |      |      |
 * |      |      |
 * |______|______|
 *
 * Cell coordinate is easily computed using input position p: 
 * cellCoordinate = floor(p/cellDimension)  
 * 
 * The underlying data is stored in a linear array:
 * id = cellCoordinate.x + cellCoordinate.y*dimension.x + cellCoordinate.z*dimension.x*dimension.y
 */
class IVW_MODULE_UNIFORMGRIDCL_API UniformGrid3DBase : public StructuredGridEntity<3> {
public:
    UniformGrid3DBase(size3_t cellDimension = size3_t(1));
    virtual ~UniformGrid3DBase() = default;
    virtual UniformGrid3DBase* clone() const override = 0;

    virtual void* getData() = 0;
    virtual const void* getData() const = 0;
    virtual size_t getSizeInBytes() const = 0;
    virtual const DataFormatBase* getDataFormat() const = 0;

    size3_t getCellDimension() const { return cellDimension_; }
    void setCellDimension(size3_t val) { cellDimension_ = val; }
private:
    size3_t cellDimension_; //< Size of one grid cell
};

using UniformGrid3DInport = DataInport<UniformGrid3DBase>;
using UniformGrid3DOutport = DataOutport<UniformGrid3DBase>;
using UniformGrid3DVector = std::vector< std::shared_ptr<UniformGrid3DBase> >;
using UniformGrid3DVector = std::vector< std::shared_ptr<UniformGrid3DBase> >;
using UniformGrid3DVectorInport = DataInport<UniformGrid3DVector>;
using UniformGrid3DVectorOutport = DataOutport<UniformGrid3DVector>;

template<>
struct port_traits<UniformGrid3DBase> {
    static std::string class_identifier() { return "UniformGrid3DBase"; }
    static uvec3 color_code() { return uvec3(239, 100, 0); } 
    static std::string data_info(const UniformGrid3DBase* data) { return "UniformGrid3DBase"; }
};

template < typename T>
class UniformGrid3D : public UniformGrid3DBase {
public:
    UniformGrid3D(size3_t gridDimensions, size3_t cellDimension, BufferUsage usage = BufferUsage::Static);
    UniformGrid3D(size3_t cellDimension = size3_t(1));
    virtual ~UniformGrid3D() = default;
    virtual UniformGrid3D* clone() const;
    /**
    * Resize to dimension. This is destructive, the data will not be
    * preserved.
    */
    void setDimensions(const size3_t& dim) override;

    virtual void* getData() override;
    virtual const void* getData() const override;
    virtual size_t getSizeInBytes() const override;
    virtual const DataFormatBase* getDataFormat() const;

    Buffer<T> data;
private:
};

namespace util {

struct IVW_MODULE_UNIFORMGRIDCL_API UniformGrid3DDispatcher {
    using type = std::shared_ptr < UniformGrid3DBase >;
    template <class T>
    std::shared_ptr<UniformGrid3DBase> dispatch(size3_t gridDimensions, size3_t cellDimension, BufferUsage usage) {
        typedef typename T::type F;
        return std::make_shared<UniformGrid3D<F>>(gridDimensions, cellDimension, usage);
    }
};

}  // namespace

template < typename T>
inviwo::UniformGrid3D<T>::UniformGrid3D(size3_t gridDimensions, size3_t cellDimension, BufferUsage usage)
    : UniformGrid3DBase(cellDimension), data(gridDimensions.x*gridDimensions.y*gridDimensions.z, usage) {
    StructuredGridEntity<3>::setDimensions(gridDimensions);
}
template < typename T>
size_t inviwo::UniformGrid3D<T>::getSizeInBytes() const  {
    return data.getSizeInBytes();
}

template < typename T>
const DataFormatBase* inviwo::UniformGrid3D<T>::getDataFormat() const {
    return data.getDataFormat();
}

template < typename T >
UniformGrid3D<T>::UniformGrid3D(size3_t cellDimension /*= size3_t(1)*/)
    : UniformGrid3DBase(cellDimension) {
}

template < typename T >
UniformGrid3D<T>* UniformGrid3D<T>::clone() const {
    return new UniformGrid3D<T>(*this);
}

template < typename T >
void UniformGrid3D<T>::setDimensions(const size3_t& dim) {
    StructuredGridEntity<3>::setDimensions(dim);
    data.setSize(dim.x*dim.y*dim.z);
}

template <typename T>
void* UniformGrid3D<T>::getData() {
    return data.getEditableRAMRepresentation()->getData();
}

template <typename T>
const void* UniformGrid3D<T>::getData() const {
    return data.getRAMRepresentation()->getData();
}

} // namespace

#endif // IVW_UNIFORMGRID3D_H

