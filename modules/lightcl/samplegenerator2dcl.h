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

#ifndef IVW_SAMPLEGENERATOR2DCL_H
#define IVW_SAMPLEGENERATOR2DCL_H

#include <modules/lightcl/lightclmoduledefine.h>
#include <inviwo/core/common/inviwo.h>
#include <modules/opencl/inviwoopencl.h>

#include <inviwo/core/ports/datainport.h>
#include <inviwo/core/ports/dataoutport.h>

#include <modules/lightcl/sample.h>

namespace inviwo {

/**
 * \class SampleGenerator2DCL
 *
 * \brief Interface for generating 2D samples
 *
 */
class IVW_MODULE_LIGHTCL_API SampleGenerator2DCL { 
public:
    SampleGenerator2DCL(bool useGLSharing = true);
    virtual ~SampleGenerator2DCL() = default;

    /** 
     * \brief Reset sample generation.
     * 
     * @return void 
     */
    virtual void reset() = 0;

    virtual void generateNextSamples(SampleBuffer& positionSamplesOut, const VECTOR_CLASS<cl::Event>* waitForEvents = nullptr, cl::Event* event = nullptr) = 0;

    /** 
     * \brief Generate position and direction samples at the same time.
     * 
     * @param SampleBuffer & positionSamplesOut 
     * @param SampleBuffer & directionSamplesOut 
     * @param const VECTOR_CLASS<cl::Event> * waitForEvents 
     * @param cl::Event * event 
     */
    virtual void generateNextSamples(SampleBuffer& positionSamplesOut, SampleBuffer& directionSamplesOut, const VECTOR_CLASS<cl::Event>* waitForEvents = nullptr, cl::Event* event = nullptr) = 0;


    size_t getWorkGroupSize() const { return workGroupSize_; }
    void setWorkGroupSize(size_t val) { workGroupSize_ = val; }

    bool getUseGLSharing() const { return useGLSharing_; }
    void setUseGLSharing(bool val) { useGLSharing_ = val; }


private:
    size_t workGroupSize_ = 64;
    bool useGLSharing_;
};

template<>
struct port_traits<SampleGenerator2DCL> {
    static std::string class_identifier() { return "SampleGenerator2DCL"; }
    static uvec3 color_code() { return uvec3(0, 168, 119); } // Green (Munsell)
    static std::string data_info(const SampleGenerator2DCL* data) { return "SampleGenerator2DCL"; }
};
using SampleGenerator2DCLInport = DataInport<SampleGenerator2DCL>;
using SampleGenerator2DCLOutport = DataOutport<SampleGenerator2DCL>;

} // namespace

#endif // IVW_SAMPLEGENERATOR2DCL_H

