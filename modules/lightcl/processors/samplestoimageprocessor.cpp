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

#include "samplestoimageprocessor.h"

namespace inviwo {

// The Class Identifier has to be globally unique. Use a reverse DNS naming scheme
const ProcessorInfo SamplesToImageProcessor::processorInfo_{
    "org.inviwo.SamplesToImageProcessor",  // Class identifier
    "Samples To Image Processor",          // Display name
    "Image",                               // Category
    CodeState::Experimental,               // Code state
    Tags::CPU,                             // Tags
};
const ProcessorInfo SamplesToImageProcessor::getProcessorInfo() const {
    return processorInfo_;
}

SamplesToImageProcessor::SamplesToImageProcessor()
    : Processor()
    , samplesPort_("Samples")
    , outport_("outport", DataFloat32::get())
    , sampleValue_("sampleVal", "Single sample value", 0.1f, 0.f, 1.f)
    , imageOut_{ std::make_shared<Image>(size2_t{ 0 }, DataFloat32::get()) }
{
    addPort(samplesPort_);
    addPort(outport_);
    addProperty(sampleValue_);

    outport_.setData(imageOut_);
}
    
void SamplesToImageProcessor::process() {
    auto samples = samplesPort_.getData();

    auto valuesRam = samples->getRAMRepresentation();
    auto imageRam = imageOut_->getColorLayer()->getEditableRepresentation<LayerRAM>();
    for (auto y = 0; y < imageRam->getDimensions().y; ++y) {
        for (auto x = 0; x < imageRam->getDimensions().x; ++x) {
            imageRam->setFromDouble(size2_t(x, y), 0);
        }
    }
    for (auto i = 0; i < samples->getSize(); ++i) {
        auto data = valuesRam->get(i);
        auto location = glm::clamp(size2_t(data.xy() *  (vec2(imageOut_->getDimensions()))), size2_t{ 0 }, imageOut_->getDimensions() - size2_t{ 1 });
        auto prevVal = imageRam->getAsNormalizedDouble(location);
        imageRam->setFromDouble(location, prevVal + sampleValue_);
    }

}

} // namespace


