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

#ifndef IVW_DIRECTIONAL_LIGHT_SAMPLER_CL_PROCESSOR_H
#define IVW_DIRECTIONAL_LIGHT_SAMPLER_CL_PROCESSOR_H

#include <inviwo/core/common/inviwo.h>
#include <inviwo/core/datastructures/light/baselightsource.h>
#include <inviwo/core/ports/meshport.h>
#include <inviwo/core/ports/bufferport.h>
#include <inviwo/core/processors/processor.h>
#include <modules/opencl/inviwoopencl.h>
#include <modules/opencl/buffer/bufferclbase.h>
#include <modules/opencl/kernelowner.h>
#include <modules/lightcl/sample.h>
#include <modules/lightcl/lightsample.h>
#include <modules/lightcl/lightsamplemeshintersectioncl.h>
#include <modules/lightcl/directionallightsamplercl.h>


namespace inviwo {

class IVW_MODULE_LIGHTCL_API DirectionalLightSamplerCLProcessor : public Processor, public KernelObserver {

public:
    DirectionalLightSamplerCLProcessor();
    ~DirectionalLightSamplerCLProcessor() = default;
    
    virtual const ProcessorInfo getProcessorInfo() const override;
    static const ProcessorInfo processorInfo_;

    virtual void onKernelCompiled(const cl::Kernel* kernel) override { invalidate(InvalidationLevel::InvalidOutput); };

    virtual void process() override;

private:
    MeshInport boundingVolume_;
    SampleInport samplesPort_;
    DataInport<LightSource> lights_; 
    LightSamplesOutport lightSamplesPort_;

	IntProperty workGroupSize_;
    BoolProperty useGLSharing_;

    DirectionalLightSamplerCL lightSampler_;
    LightSampleMeshIntersectionCL lightSampleMeshIntersector_;
    std::shared_ptr< LightSamples > lightSamples_;
};

}

#endif // IVW_DIRECTIONAL_LIGHT_SAMPLER_CL_PROCESSOR_H
