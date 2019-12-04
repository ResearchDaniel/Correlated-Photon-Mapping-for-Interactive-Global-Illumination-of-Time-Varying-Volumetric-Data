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

#ifndef IVW_PHOTON_DATA_H
#define IVW_PHOTON_DATA_H

#include <modules/progressivephotonmapping/progressivephotonmappingmoduledefine.h>
#include <inviwo/core/common/inviwo.h>

#include <inviwo/core/datastructures/buffer/buffer.h>
#include <inviwo/core/datastructures/datatraits.h>
#include <inviwo/core/ports/port.h>


namespace inviwo {


struct Photon {
    // (float8)(photonPos.x, photonPos.y, photonPos.z, photonPower.x, photonPower.y, photonPower.z, dirAngles.x, dirAngles.y);
    vec3 pos;
    vec3 power; // RGB
    vec2 encodedDirection; // Storing the direction encoded to make struct 8 float * 4 byte = 32 byte => aligned reads
    
    void setDirection(vec3 dir);
    vec3 getDirection() const;
    
};

struct RecomputedPhotonIndices {
    Buffer<unsigned int> indicesToRecomputedPhotons;
    int nRecomputedPhotons = -1; // -1 means uninitialized
    bool isInitialized() const { return nRecomputedPhotons != -1; }
    void setUninitialized() { nRecomputedPhotons = -1; }
};

class IVW_MODULE_PROGRESSIVEPHOTONMAPPING_API PhotonData {
public:
    enum class InvalidationReason {
        Camera              = 1 << 0,
        TransferFunction    = 1 << 1,
        Light               = 1 << 2,
        Progressive         = 1 << 3,
        Volume              = 1 << 4,
        All = Camera | TransferFunction | Light | Progressive | Volume
    };
    
    PhotonData() = default;
    PhotonData(const PhotonData& other) = default;
    
    virtual ~PhotonData();
    
    
    void copyParamsFrom(const PhotonData& rhs);
    void setSize(size_t numberOfPhotons, int maxPhotonInteractions);
    size_t getNumberOfPhotons() const { return photons_.getSize() / (2 * maxPhotonInteractions_); }
    int getMaxPhotonInteractions() const { return maxPhotonInteractions_; }
    
    void setRadius(double radiusRelativeToSceneSize, double sceneRadius);
    /**
     * \brief Sets the radius of the next iteration and increases the iteration count by one.
     * See: http ://www.cs.jhu.edu/~misha/ReadingSeminar/Papers/Knaus11.pdf
     * eq. 20: r_(i+1) = r_i*((i+alpha)/(i+1) )^(1/3)
     * (Sphere)
     * The variance and the expected value of the average error vanish if 0 < alpha < 1,
     where ? controls the relative rate of decrease.
     * @param float alpha How fast the radius should decrease, ]0 1[. A value of 1 will keep the radius the same.
     * @return void
     */
    void advanceToNextIteration(double alpha = 0.5f);
    
    /**
     * \brief Returns the radius of photons relative to the size of the scene.
     * Used to get a measure that is independent on the scale of the world.
     * A value of one means that getRadius will equal the radius of the scene,
     * i.e. half the diagonal of the volume.
     *
     * @return float
     */
    double getRadiusRelativeToSceneSize() const { return getRadius() / sceneRadius_; }
    double getRadius() const { return worldSpaceRadius_; }
    /**
     * \brief Set radius of photons in world space.
     */
    void setRadius(double radius) { worldSpaceRadius_ = radius; }
    
    /**
     * \brief Compute progressive photon mapping radius for next iteration.
     *
     * See: http ://www.cs.jhu.edu/~misha/ReadingSeminar/Papers/Knaus11.pdf
     * eq. 20: r_(i+1) = r_i*((i+alpha)/(i+1) )^(1/3)
     * (Sphere)
     *
     * @param float radius The radius of the current iteration
     * @param int iteration Current iteration, first starts at 0
     * @param float alpha How fast the radius should decrease, [0 1[. A value of 1 will keep the radius the same.
     * @return float The radius of the next iteration
     */
    static double progressiveSphereRadius(double radius, int iteration, double alpha);
    
    double getSceneRadius() const { return sceneRadius_; }
    
    void resetIteration() { iteration_ = 0; }
    bool isReset() const { return iteration_ <= 1; }
    
    int iteration() const { return iteration_; }
    void setIteration(int val) { iteration_ = val; }
    
    static double sphereVolume(double radius);
    double getRelativeIrradianceScale() const;
    
    Buffer<vec4> photons_;
    
    static const float defaultRadiusRelativeToSceneRadius;
    static const float defaultSceneRadius;
    static const double scaleToMakeLightPowerOfOneVisibleForDirectionalLightSource;
    static const int defaultNumberOfPhotons{ 256 * 256 };
    
    PhotonData::InvalidationReason getInvalidationReason() const { return invalidationFlag_; }
    void setInvalidationReason(PhotonData::InvalidationReason val);
protected:
    int maxPhotonInteractions_ = 1;
    double sceneRadius_ = 1.0;
    double worldSpaceRadius_ = 0.01;
    int iteration_ = 0; ///< Progressive refinement iteration
    InvalidationReason invalidationFlag_ = InvalidationReason::All;
    
};
inline PhotonData::InvalidationReason operator|(PhotonData::InvalidationReason a, PhotonData::InvalidationReason b)
{
    return static_cast<PhotonData::InvalidationReason>(static_cast<int>(a) | static_cast<int>(b));
}
inline PhotonData::InvalidationReason& operator |=(PhotonData::InvalidationReason& a, PhotonData::InvalidationReason b)
{
    return a = a | b;
}



template<>
struct DataTraits<PhotonData> {
    static std::string classIdentifier() { return "org.inviwo.photondata"; }
    static std::string dataName() { return "PhotonData"; }
    static uvec3 colorCode() { return uvec3(239, 204, 0); } // Yellow (Munsell)
    static std::string info(const PhotonData& data) {
        using H = utildoc::TableBuilder::Header;
        using P = Document::PathComponent;
        Document doc;
        doc.append("b", "PhotonData", { { "style", "color:white;" } });
        utildoc::TableBuilder tb(doc.handle(), P::end());
        tb(H("Size"), data.getNumberOfPhotons());
        tb(H("Max iterations"), data.getMaxPhotonInteractions());
        tb(H("Radius"), data.getRadius());
        return doc;
    }
};

template<>
struct DataTraits<RecomputedPhotonIndices> {
    static std::string classIdentifier() { return "org.inviwo.recomputedphotonindices"; }
    static std::string dataName() { return "RecomputedPhotonIndices"; }
    static uvec3 colorCode() { return uvec3(200, 180, 0); }
    static std::string info(const RecomputedPhotonIndices& data) {
        using H = utildoc::TableBuilder::Header;
        Document doc;
        doc.append("b", "RecomputedPhotonIndices", { { "style", "color:white;" } });
        return doc;
    }
};

} // namespace

#endif // IVW_PHOTON_DATA_H
