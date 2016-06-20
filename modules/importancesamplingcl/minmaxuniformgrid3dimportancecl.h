/*********************************************************************************
 *
 * Copyright (c) 2016, Daniel Jönsson
 * All rights reserved.
 * 
 * This work is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
 * http://creativecommons.org/licenses/by-nc/4.0/
 * 
 * You are free to:
 * 
 * Share — copy and redistribute the material in any medium or format
 * Adapt — remix, transform, and build upon the material
 * The licensor cannot revoke these freedoms as long as you follow the license terms.
 * Under the following terms:
 * 
 * Attribution — You must give appropriate credit, provide a link to the license, and indicate if changes were made. You may do so in any reasonable manner, but not in any way that suggests the licensor endorses you or your use.
 * NonCommercial — You may not use the material for commercial purposes.
 * No additional restrictions — You may not apply legal terms or technological measures that legally restrict others from doing anything the license permits.
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

#ifndef IVW_MINMAXUNIFORMGRID3DIMPORTANCECL_H
#define IVW_MINMAXUNIFORMGRID3DIMPORTANCECL_H

#include <modules/importancesamplingcl/importancesamplingclmoduledefine.h>
#include <inviwo/core/common/inviwo.h>
#include <inviwo/core/datastructures/buffer/buffer.h>
#include <inviwo/core/datastructures/image/image.h>
#include <inviwo/core/datastructures/transferfunction.h>
#include <inviwo/core/datastructures/volume/volume.h>

#include <modules/opencl/inviwoopencl.h>
#include <modules/opencl/buffer/buffercl.h>
#include <modules/opencl/image/layerclbase.h>
#include <modules/opencl/volume/volumeclbase.h>
#include <modules/opencl/kernelowner.h>

#include <modules/uniformgridcl/minmaxuniformgrid3d.h>

namespace inviwo {

/**
 * \class MinMaxUniformGrid3DImportanceCL
 *
 * \brief Compute importance as seen from input directions
 *
 */
class IVW_MODULE_IMPORTANCESAMPLINGCL_API MinMaxUniformGrid3DImportanceCL : KernelOwner {
public:
    MinMaxUniformGrid3DImportanceCL();
    virtual ~MinMaxUniformGrid3DImportanceCL(){}
    bool computeImportance(const Volume* origVolume, const MinMaxUniformGrid3D* uniformGrid3D, const TransferFunction* transferFunction, const Image* entryPoints, const Image* exitPoints, const uvec2& workGroupSize, bool useGLSharing);
    Buffer<float>* getImportance() { return &importance_; }

protected:
    bool computeImportance(const Volume* origVolume, const MinMaxUniformGrid3D* uniformGrid3D, const BufferCLBase* uniformGridCL, const TransferFunction* transferFunction, const cl::Image& entryPoints, const cl::Image& exitPoints, BufferCLBase* importanceBuf, const size2_t& globalWorkGroupSize, const size2_t& localWorkgroupSize, cl::Event* event);
private:

    Buffer<float> importance_;

    cl::Kernel* tracerKernel_;

};

} // namespace

#endif // IVW_MINMAXUNIFORMGRID3DIMPORTANCECL_H

