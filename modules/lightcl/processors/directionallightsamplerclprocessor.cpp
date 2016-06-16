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

#include <modules/lightcl/processors/directionallightsamplerclprocessor.h>

namespace inviwo {

// The Class Identifier has to be globally unique. Use a reverse DNS naming scheme
const ProcessorInfo DirectionalLightSamplerCLProcessor::processorInfo_{
    "org.inviwo.DirectionalLightSamplerCL",  // Class identifier
    "Directional light sampler",               // Display name
    "Light source",                            // Category
    CodeState::Experimental,                   // Code state
    Tags::CL,                                  // Tags
};
const ProcessorInfo DirectionalLightSamplerCLProcessor::getProcessorInfo() const {
    return processorInfo_;
}

DirectionalLightSamplerCLProcessor::DirectionalLightSamplerCLProcessor()
    : Processor(), KernelObserver()
    , boundingVolume_("SceneGeometry")
    , samplesPort_("samples")
    , lightSamplesPort_("LightSamples")
    , lights_("light")
	, workGroupSize_("wgsize", "Work group size", 64, 1, 4096)
    , useGLSharing_("glsharing", "Use OpenGL sharing", true)
    , lightSamples_(std::make_shared<LightSamples>())
    , lightSampler_(workGroupSize_.get())
{
    addPort(boundingVolume_);
    addPort(samplesPort_);
    addPort(lights_);

    addPort(lightSamplesPort_);

	addProperty(workGroupSize_);

    lights_.onChange([this]() {lightSamples_->resetIteration(); });

    lightSampler_.setWorkGroupSize(workGroupSize_);
    workGroupSize_.onChange([this]() { lightSampler_.setWorkGroupSize(workGroupSize_.get()); });
    useGLSharing_.onChange([this]() { lightSampler_.setUseGLSharing(useGLSharing_); });

    addObservation(&lightSampler_);
    addObservation(&lightSampleMeshIntersector_);
    
}

void DirectionalLightSamplerCLProcessor::process() {
    if (!lightSampler_.isValid() || !lightSampleMeshIntersector_.isValid()) {
		return;
	}
    auto samples = samplesPort_.getData();
    auto mesh = boundingVolume_.getData();
    const LightSource* light = lights_.getData().get();
    lightSampler_.sampleLightSource(mesh.get(), samples.get(), light, *lightSamples_.get());
    lightSampleMeshIntersector_.meshSampleIntersection(mesh.get(), lightSamples_.get());
    lightSamplesPort_.setData(lightSamples_);
}

} // inviwo namespace
