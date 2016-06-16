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

#include "lightsample.h"

namespace inviwo {

LightSample::LightSample()  {
    
}

LightSample::~LightSample()  {
    
}

inviwo::vec3 LightSample::getDirection() const {
    vec2 cosAngles = glm::cos(encodedDirection);
    vec2 sinAngles = glm::sin(encodedDirection);
    return vec3{ sinAngles.x*cosAngles.y,
        sinAngles.x*sinAngles.y,
        cosAngles.x };
}

void LightSample::setDirection(vec3 dir) {
    float phi = atan2(dir.y, dir.x);
    //if ( !isfinite(phi) ) {
    //if(dir.y < 0.f) phi = -0.5f*M_PI;
    //else            phi = 0.5f*M_PI;
    //}
    // Important: clamp dir.z to avoid NaN
    float theta = acos(glm::clamp(dir.z, -1.f, 1.f));
    encodedDirection = vec2{ theta, phi };
}

LightSamples::LightSamples(size_t nSamples /*= 0*/)
    : lightSamples_(nSamples*sizeof(LightSample)), intersectionPoints_(nSamples) {

}

LightSamples::~LightSamples() {

}

void LightSamples::setSize(size_t nSamples) {
    lightSamples_.setSize(nSamples*sizeof(LightSample));
    intersectionPoints_.setSize(nSamples);
}

size_t LightSamples::getSize() const {
    return lightSamples_.getSize() / sizeof(LightSample);

}

} // namespace

