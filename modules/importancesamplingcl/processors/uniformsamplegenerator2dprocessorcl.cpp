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

#include <modules/importancesamplingcl/processors/uniformsamplegenerator2dprocessorcl.h>

#include <modules/opencl/inviwoopencl.h>
#include <modules/opencl/buffer/bufferclgl.h>
#include <modules/opencl/syncclgl.h>

namespace inviwo {

const ProcessorInfo UniformSampleGenerator2DProcessorCL::processorInfo_{
    "org.inviwo.UniformSampleGenerator2DCL",  // Class identifier
    "UniformSampleGenerator2D",                 // Display name
    "Sampling",                                 // Category
    CodeState::Experimental,                    // Code state
    Tags::CL,                                   // Tags
};
const ProcessorInfo UniformSampleGenerator2DProcessorCL::getProcessorInfo() const {
    return processorInfo_;
}

UniformSampleGenerator2DProcessorCL::UniformSampleGenerator2DProcessorCL()
    : Processor(), ProcessorKernelOwner(this)
    , samplesPort_("samples")
    , directionalSamplesPort_("DirectionalSamples")
    , sampleGeneratorPort_("SampleGenerator")
    , nSamples_("nSamples", "N samples", ivec2(256), ivec2(2), ivec2(2048))
    , workGroupSize_("wgsize", "Work group size", ivec2(8, 8), ivec2(0), ivec2(256))
    , useGLSharing_("glsharing", "Use OpenGL sharing", true)
    , sampleGenerator_(useGLSharing_.get())
    , samples_(std::make_shared<SampleBuffer>())
    , directionalSamples_(std::make_shared<SampleBuffer>())
{

    addPort(samplesPort_);
    addPort(directionalSamplesPort_);

    addProperty(nSamples_);
    addProperty(workGroupSize_);
    addProperty(useGLSharing_);

    samplesPort_.setData(samples_);
    directionalSamplesPort_.setData(directionalSamples_);
}

void UniformSampleGenerator2DProcessorCL::process() {
    size2_t nSamples(nSamples_.get());

    if (directionalSamplesPort_.isConnected()) {
        if (nSamples.x * nSamples.y != samples_->getSize() || nSamples.x * nSamples.y != directionalSamples_->getSize()) {
            samples_->setSize(nSamples.x * nSamples.y);
            directionalSamples_->setSize(nSamples.x * nSamples.y);
        }
        sampleGenerator_.generateNextSamples(*samples_, *directionalSamples_);
    } else {
        if (nSamples.x * nSamples.y != samples_->getSize()) {
            samples_->setSize(nSamples.x * nSamples.y);
        }
        if (directionalSamples_->getSize() != 0) {
            directionalSamples_->setSize(0);
        }
        sampleGenerator_.generateNextSamples(*samples_);
    }

}

} // inviwo namespace

