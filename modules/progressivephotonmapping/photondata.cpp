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

#include <modules/progressivephotonmapping/photondata.h>

namespace inviwo {
const float PhotonData::defaultRadiusRelativeToSceneRadius{ 0.0153866f };
const float PhotonData::defaultSceneRadius{ 1.1447142425533318678080422119397f }; // 0.5 * length(vec3(2))
const double PhotonData::scaleToMakeLightPowerOfOneVisibleForDirectionalLightSource{ 1./M_PI }; // 1/(2*pi)



PhotonData::~PhotonData() {
}

void PhotonData::copyParamsFrom(const PhotonData& rhs) {
    maxPhotonInteractions_ = rhs.maxPhotonInteractions_;
    sceneRadius_ = rhs.sceneRadius_;
    
    worldSpaceRadius_ = rhs.worldSpaceRadius_;
    iteration_ = rhs.iteration_;
}

void PhotonData::setSize(size_t numberOfPhotons, int maxPhotonInteractions) {
    maxPhotonInteractions_ = maxPhotonInteractions;
    if (numberOfPhotons > 0) {
        photons_.setSize(numberOfPhotons * 2 * maxPhotonInteractions);
    }

}

void PhotonData::setRadius(double radiusRelativeToSceneSize, double sceneRadius) {
    sceneRadius_ = sceneRadius;
    worldSpaceRadius_ = radiusRelativeToSceneSize*sceneRadius;
    //LogInfo("Scene radius: " << sceneRadius_ << " Wold space radius: " << worldSpaceRadius_);
}

void PhotonData::advanceToNextIteration(double alpha) {
    setRadius(progressiveSphereRadius(getRadius(), iteration_, alpha));
    iteration_++;
}

double PhotonData::progressiveSphereRadius(double radius, int iteration, double alpha) {
    // See: http://www.cs.jhu.edu/~misha/ReadingSeminar/Papers/Knaus11.pdf
    // eq. 20: r_(i+1) = r_i*((i+alpha)/(i+1) )^(1/3)
    // Sphere
    return radius*std::pow((static_cast<double>(iteration)+alpha) / (1.0 + static_cast<double>(iteration)), 1. / 3.);
    // Disc
    //radius = prevRadius_*std::sqrt((static_cast<float>(iteration_)+alphaProp_.get() )/(1.f+static_cast<float>(iteration_)));
}

double PhotonData::sphereVolume(double radius) {
    return std::pow(radius, 3) * (M_PI*4. / 3.);
}

double PhotonData::getRelativeIrradianceScale() const {
    // Scale with lightVolumeSizeScale to get the same look for different light volume sizes.
    double referenceRadiusVolumeScale = sphereVolume(getRadiusRelativeToSceneSize()) / sphereVolume(defaultRadiusRelativeToSceneRadius);
   
    // Works when the light volume is normalized by the number of photons per voxel.

    // Use photon scale to get equivalent appearance when normalizing light volume 
    double nPhotonsScale = static_cast<double>(getNumberOfPhotons()) / static_cast<double>(defaultNumberOfPhotons);
    return referenceRadiusVolumeScale*nPhotonsScale;
}

void PhotonData::setInvalidationReason(InvalidationReason val) {
    invalidationFlag_ = val;
}

void Photon::setDirection(vec3 dir) {
    float phi = atan2(dir.y, dir.x);
    //if ( !isfinite(phi) ) {
    //if(dir.y < 0.f) phi = -0.5f*M_PI;
    //else            phi = 0.5f*M_PI;
    //}
    // Important: clamp dir.z to avoid NaN
    float theta = acos(glm::clamp(dir.z, -1.f, 1.f));
    encodedDirection = vec2{ theta, phi };
}

vec3 Photon::getDirection() const {
    vec2 cosAngles = glm::cos(encodedDirection);
    vec2 sinAngles = glm::sin(encodedDirection);
    return vec3{ sinAngles.x*cosAngles.y,
        sinAngles.x*sinAngles.y,
        cosAngles.x };
}

} // namespace
