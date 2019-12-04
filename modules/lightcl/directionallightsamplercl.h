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

#ifndef IVW_DIRECTIONALLIGHTSAMPLERCL_H
#define IVW_DIRECTIONALLIGHTSAMPLERCL_H

#include <modules/lightcl/lightclmoduledefine.h>
#include <inviwo/core/common/inviwo.h>
#include <inviwo/core/datastructures/light/baselightsource.h>
#include <inviwo/core/datastructures/geometry/mesh.h>

#include <modules/opencl/inviwoopencl.h>
#include <modules/opencl/buffer/bufferclbase.h>
#include <modules/opencl/kernelowner.h>

#include <modules/lightcl/sample.h>
#include <modules/lightcl/lightsourcesamplercl.h>

namespace inviwo {

/**
 * \class DirectionalLightSampleCL
 *
 * \brief Compute the position, power and direction of a directional light source. 
 * 
 * Samples the light source given inputs in [0 1]^2 + pdf.
 * The extent on the light source is computed by projecting the mesh
 * onto the light source plane and computing the optimal oriented bounding 
 * box.
 */
class IVW_MODULE_LIGHTCL_API DirectionalLightSamplerCL : public LightSourceSamplerCL,  public KernelOwner {
public:
    DirectionalLightSamplerCL(size_t workGroupSize = 128, bool useGLSharing = true);
    virtual ~DirectionalLightSamplerCL();

    bool isValid() const { return kernel_ != nullptr; }

    virtual void sampleLightSource(const Mesh* mesh, LightSamples& lightSamplesOut, const VECTOR_CLASS<cl::Event>* waitForEvents = nullptr, cl::Event* event = nullptr);

    void sampleLightSource(const Mesh* mesh, const SampleBuffer* samples, const LightSource* light, LightSamples& lightSamplesOut);

    void sampleLightSource(const BufferCLBase* samplesCL, vec3 radiance, vec3 lightDirection, vec3 lightOrigin, vec3 u, vec3 v, float area, size_t nSamples, BufferCLBase* lightSamplesCL, const VECTOR_CLASS<cl::Event>* waitForEvents = nullptr, cl::Event* event = nullptr);

    bool getUseGLSharing() const { return useGLSharing_; }
    void setUseGLSharing(bool val) { useGLSharing_ = val; }
    size_t getWorkGroupSize() const { return workGroupSize_; }
    void setWorkGroupSize(size_t val) { workGroupSize_ = val; }
private:
    SampleBuffer samples_;
    bool useGLSharing_;
    size_t workGroupSize_;
    cl::Kernel* kernel_;
};

} // namespace

#endif // IVW_DIRECTIONALLIGHTSAMPLECL_H

