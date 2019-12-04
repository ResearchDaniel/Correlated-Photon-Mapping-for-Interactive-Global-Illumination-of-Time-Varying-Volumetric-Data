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

#ifndef IVW_LIGHTSAMPLE_H
#define IVW_LIGHTSAMPLE_H

#include <modules/lightcl/lightclmoduledefine.h>
#include <inviwo/core/common/inviwo.h>
#include <inviwo/core/datastructures/buffer/buffer.h>
#include <inviwo/core/ports/datainport.h>
#include <inviwo/core/ports/dataoutport.h>

namespace inviwo {

/**
 * \class LightSample
 *
 * \brief Origin, power and encoded direction. 
 * Must match struct in lightsample.cl
 *
 * @see Photon
 */
class IVW_MODULE_LIGHTCL_API LightSample { 
public:
    LightSample();
    virtual ~LightSample();


    vec3 getOrigin() const { return origin; }
    void setOrigin(inviwo::vec3 val) { origin = val; }
    vec3 getPower() const { return power; }
    void setPower(inviwo::vec3 val) { power = val; }
    /** 
     * \brief Get normalized direction.
     * 
     * @return inviwo::vec3 
     */
    vec3 getDirection() const;
    void setDirection(vec3 normalizedDirection);
protected:
    vec3 origin;
    vec3 power;
    vec2 encodedDirection;

};


/**
* \class LightSamples
*
* \brief Container for multiple light samples and their intersection point along the direction.
*
* Iteration indicates how many times a light source has been sampled since it changed.
* Iteration should be reset when the light source changes and advanced after 
* the light source has been sampled.
*
* @see LightSample
*/
class IVW_MODULE_LIGHTCL_API LightSamples {
public:
    LightSamples(size_t nSamples = 0);
    virtual ~LightSamples();

    Buffer<unsigned char>* getLightSamples() { return &lightSamples_; }
    const Buffer<unsigned char>* getLightSamples() const { return &lightSamples_; }
    const Buffer<vec2>* getIntersectionPoints() const { return &intersectionPoints_; }
    Buffer<vec2>* getIntersectionPoints() { return &intersectionPoints_; }

    void setSize(size_t nSamples);
    size_t getSize() const;

    void resetIteration() { iteration_ = 0; }
    void advanceIteration() { ++iteration_; }
    /** 
     * \brief Did the light source change since last iteration.
     * 
     * @return bool True if changed, false otherwise
     */
    bool isReset() const { return iteration_ <= 1; }
    size_t getIteration() const { return iteration_; }
    void setIteration(size_t val) { iteration_ = val; }
private:
    Buffer<unsigned char> lightSamples_;
    Buffer<vec2> intersectionPoints_; // tStart, tEnd for each light sample
    size_t iteration_ = 0; // Resets when light source changes
};

template<>
struct DataTraits<LightSamples> {
    static std::string classIdentifier() { return "org.inviwo.lightsamples"; }
    static std::string dataName() { return "LightSamples"; }
    static uvec3 colorCode() { return uvec3(209, 174, 0); } // Darker than photon data
    static Document info(const LightSamples& data) {
        using H = utildoc::TableBuilder::Header;
        using P = Document::PathComponent;
        Document doc;
        doc.append("b", "LightSamples", { { "style", "color:white;" } });
        utildoc::TableBuilder tb(doc.handle(), P::end());
        tb(H("Size"), data.getSize());
        tb(H("Iteration"), data.getIteration());
        return doc;
    }
};
using LightSamplesInport = DataInport<LightSamples>;
using LightSamplesOutport = DataOutport<LightSamples>;

} // namespace

#endif // IVW_LIGHTSAMPLE_H

