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

#ifndef IVW_PHOTONTOLIGHTVOLUMEPROCESSORCL_H
#define IVW_PHOTONTOLIGHTVOLUMEPROCESSORCL_H

#include <modules/progressivephotonmapping/progressivephotonmappingmoduledefine.h>
#include <inviwo/core/common/inviwo.h>
#include <inviwo/core/processors/processor.h>
#include <inviwo/core/ports/volumeport.h>
#include <inviwo/core/datastructures/buffer/buffer.h>
#include <inviwo/core/properties/boolproperty.h>
#include <inviwo/core/properties/optionproperty.h>
#include <inviwo/core/properties/ordinalproperty.h>
#include <modules/base/properties/volumeinformationproperty.h>

#include <modules/opencl/inviwoopencl.h>
#include <modules/opencl/kernelowner.h>
#include <modules/opencl/buffer/buffercl.h>
#include <modules/opencl/image/layerclbase.h>
#include <modules/opencl/kernelowner.h>
#include <modules/opencl/volume/volumeclbase.h>

#include <modules/progressivephotonmapping/photondata.h>

namespace inviwo {

/** \docpage{<classIdentifier>, PhotonToLightVolumeProcessorCL}
 * Explanation of how to use the processor.
 *
 * ### Inports
 *   * __<Inport1>__ <description>.
 *
 * ### Outports
 *   * __<Outport1>__ <description>.
 *
 * ### Properties
 *   * __<Prop1>__ <description>.
 *   * __<Prop2>__ <description>
 */


/**
 * \class PhotonToLightVolumeProcessorCL
 *
 * \brief <brief description>
 *
 * <Detailed description from a developer prespective>
 */
class IVW_MODULE_PROGRESSIVEPHOTONMAPPING_API PhotonToLightVolumeProcessorCL : public Processor {
public:
    virtual const ProcessorInfo getProcessorInfo() const override;
    static const ProcessorInfo processorInfo_;
    PhotonToLightVolumeProcessorCL();
    virtual ~PhotonToLightVolumeProcessorCL() = default;
    
    virtual void process();
protected:
    void executeVolumeOperation(const Volume* volume, const VolumeCLBase* volumeCL, VolumeCLBase* volumeOutCL, const BufferCLBase* photonsCL, const Volume* volumeOut, const size3_t& outDim, const size_t& globalWorkGroupSize, const size_t& localWorkgroupSize, std::vector<cl::Event>* waitForEvents, std::vector<cl::Event>* splatEvent, cl::Event* copyEvent);
    
    void clearBuffer(BufferCL* tmpVolumeCL, size_t outDimFlattened, const size_t& localWorkgroupSize, std::vector<cl::Event>* waitForEvents, cl::Event * events);
    
    void photonsToLightVolume(VolumeCLBase* volumeOutCL, const BufferCLBase* photonsCL, const BufferCLBase* photonIndices, const PhotonData& photons, const RecomputedPhotonIndices& recomputedPhotons, float radianceMultiplier, const Volume* volumeOut, const size3_t& outDim, const size_t& globalWorkGroupSize, const size_t& localWorkgroupSize, std::vector<cl::Event>* waitForEvents, cl::Event* event);
    void volumeSizeOptionChanged();
    void buildKernel();
    private:
    VolumeInport volumeInport_;
    DataInport<PhotonData> photons_;
    DataInport<RecomputedPhotonIndices> recomputedPhotonIndicesPort_;
    VolumeOutport outport_;
    
    
    //CameraProperty camera_;
    OptionPropertyInt volumeSizeOption_;
    OptionPropertyString volumeDataTypeOption_;
    FloatProperty incrementalRecomputationThreshold_; // Threshold to decide if photons should be removed-added.
    VolumeInformationProperty information_;
    BoolProperty alignChangedPhotons_;
    IntProperty workGroupSize_;
    BoolProperty useGLSharing_;
    
    ProcessorKernelOwner kernelOwner_;
    cl::Kernel* kernel_;
    cl::Kernel* splatSelectedPhotonsKernel_;
    cl::Kernel* clearFloatsKernel_;
    cl::Kernel* photonDensityNormalizationKernel_;
    cl::Kernel* copyIndexPhotonsKernel_;
    std::vector<cl::Event> copyPrevPhotonsEvent_; // Can be done in parallel, wait for completion if
    std::shared_ptr<Volume> lightVolume_;
    Buffer<vec4> prevPhotons_; // Copy of photons from last computation only used when recomputedPhotonIndicesPort_ is connected
    Buffer<vec4> changedAlignedPhotons_; // Aligned copy of photons changed from previous and current distribution. Only used when recomputedPhotonIndicesPort_ is connected
    Buffer<unsigned char> tmpVolume_;   // Enables atomic operations to be used
};

} // namespace

#endif // IVW_PHOTONTOLIGHTVOLUMEPROCESSORCL_H

