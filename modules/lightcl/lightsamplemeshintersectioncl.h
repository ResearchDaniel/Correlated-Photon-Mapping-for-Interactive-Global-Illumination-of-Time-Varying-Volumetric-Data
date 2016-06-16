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

#ifndef IVW_LIGHTSAMPLEMESHINTERSECTIONCL_H
#define IVW_LIGHTSAMPLEMESHINTERSECTIONCL_H

#include <modules/lightcl/lightclmoduledefine.h>
#include <inviwo/core/common/inviwo.h>

#include <modules/opencl/inviwoopencl.h>
#include <modules/opencl/buffer/bufferclbase.h>
#include <modules/opencl/kernelowner.h>

#include <modules/lightcl/sample.h>
#include <modules/lightcl/lightsample.h>

namespace inviwo {

/**
 * \class LightSampleMeshIntersectionCL
 *
 * \brief Computes the intersection point with the light sample rays and the mesh.
 *
 */
class IVW_MODULE_LIGHTCL_API LightSampleMeshIntersectionCL : public KernelOwner { 
public:
    LightSampleMeshIntersectionCL(size_t workGroupSize = 128, bool useGLSharing = true);
    virtual ~LightSampleMeshIntersectionCL();

    bool isValid() const { return intersectionKernel_ != nullptr; }

    void meshSampleIntersection(const Mesh* mesh, LightSamples* samples);

    void meshSampleIntersection(const BufferCLBase* verticesCL, const BufferCLBase* indicesCL, size_t nIndices, size_t nSamples, const BufferCLBase* lightSamplesCL, BufferCLBase* intersectionPointsCL, const VECTOR_CLASS<cl::Event>* waitForEvents = nullptr, cl::Event* event = nullptr);

    bool getUseGLSharing() const { return useGLSharing_; }
    void setUseGLSharing(bool val) { useGLSharing_ = val; }
    size_t getWorkGroupSize() const { return workGroupSize_; }
    void setWorkGroupSize(size_t val) { workGroupSize_ = val; }
private:
    bool useGLSharing_;
    size_t workGroupSize_;
    cl::Kernel* intersectionKernel_;
};

} // namespace

#endif // IVW_LIGHTSAMPLEMESHINTERSECTIONCL_H

