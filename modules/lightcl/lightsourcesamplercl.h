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

#ifndef IVW_LIGHTSOURCESAMPLERCL_H
#define IVW_LIGHTSOURCESAMPLERCL_H

#include <modules/lightcl/lightclmoduledefine.h>
#include <inviwo/core/common/inviwo.h>
#include <inviwo/core/datastructures/light/baselightsource.h>
#include <inviwo/core/datastructures/geometry/mesh.h>
#include <modules/lightcl/samplegenerator2dcl.h>
#include <modules/lightcl/lightsample.h>


namespace inviwo {

/**
 * \class LightSourceSamplerCL
 *
 * \brief Interface for light source samplers
 *
 * DESCRIBE_THE_CLASS
 */
class IVW_MODULE_LIGHTCL_API LightSourceSamplerCL { 
public:
    LightSourceSamplerCL(std::shared_ptr<const LightSource> lightSource, std::shared_ptr<SampleGenerator2DCL> sampleGenerator);
    virtual ~LightSourceSamplerCL();

    virtual void sampleLightSource(const Mesh* mesh, LightSamples& lightSamplesOut, const VECTOR_CLASS<cl::Event>* waitForEvents = nullptr, cl::Event* event = nullptr) = 0;
    
    size2_t getWorkGroupSize() const { return workGroupSize_; }
    void setWorkGroupSize(size2_t val) { workGroupSize_ = val; }

    bool getUseGLSharing() const { return useGLSharing_; }
    void setUseGLSharing(bool val) { useGLSharing_ = val; }

    std::shared_ptr<const LightSource> getLightSource() const { return lightSource_; }
    void setLightSource(std::shared_ptr<const LightSource> val) { lightSource_ = val; }
    std::shared_ptr<SampleGenerator2DCL> getSampleGenerator() const { return sampleGenerator_; }
    void setSampleGenerator(std::shared_ptr<SampleGenerator2DCL> val) { sampleGenerator_ = val; }
protected:
    std::shared_ptr<SampleGenerator2DCL> sampleGenerator_;
    std::shared_ptr<const LightSource> lightSource_;
private:
    size2_t workGroupSize_;
    bool useGLSharing_;


};

} // namespace

#endif // IVW_LIGHTSOURCESAMPLERCL_H

