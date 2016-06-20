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

#ifndef IVW_PHOTONTRACERCL_H
#define IVW_PHOTONTRACERCL_H

#include <modules/progressivephotonmapping/progressivephotonmappingmoduledefine.h>
#include <inviwo/core/datastructures/transferfunction.h>
#include <inviwo/core/properties/advancedmaterialproperty.h>
#include <inviwo/core/common/inviwo.h>

#include <modules/lightcl/lightsample.h>

#include <modules/opencl/inviwoopencl.h>
#include <modules/opencl/kernelowner.h>
#include <modules/opencl/buffer/buffercl.h>
#include <modules/opencl/image/layerclbase.h>
#include <modules/opencl/light/packedlightsource.h>
#include <modules/opencl/volume/volumeclbase.h>
#include <modules/progressivephotonmapping/photondata.h>


namespace inviwo {



// Calculate how many samples to take from each light source.
// x component contains the amount of samples to take in x and y dimensions
// y component is the number of samples taken for each light source (x*x)
IVW_MODULE_PROGRESSIVEPHOTONMAPPING_API uvec2 getSamplesPerLight(uvec2 nSamples, int nLightSources);


/**
 * \class PhotonTracerCL
 *
 * \brief VERY_BRIEFLY_DESCRIBE_THE_CLASS
 *
 * DESCRIBE_THE_CLASS
 */
class IVW_MODULE_PROGRESSIVEPHOTONMAPPING_API PhotonTracerCL : public KernelOwner  {
public:
    PhotonTracerCL(size2_t workGroupSize = size2_t(8, 8), bool useGLSharing = false);
    virtual ~PhotonTracerCL(){}


    void tracePhotons(const Volume* volume, const TransferFunction& transferFunction, const BufferCL* axisAlignedBoundingBoxCL, const AdvancedMaterialProperty& material, const Camera* camera, float stepSize, const LightSamples* lightSamples, const Buffer<unsigned int>* photonsToRecomputeIndices, int nInvalidPhotons, int photonOffset, int batch, int maxInteractions, PhotonData* photonOutData, const VECTOR_CLASS<cl::Event> *waitForEvents, cl::Event *event = nullptr);

    void tracePhotons(PhotonData* photonData, const VolumeCLBase* volumeCL, const Buffer<glm::u8>& volumeStruct, const BufferCL* axisAlignedBoundingBoxCL, const LayerCLBase* transferFunctionCL, const AdvancedMaterialProperty& material, float stepSize, const BufferCLBase* lightSamplesCL, const BufferCLBase* intersectionPointsCL, size_t nLightSamples, const BufferCLBase* photonsToRecomputeIndicesCL, int nPhotonsToRecompute, BufferCLBase* photonsCL, int photonOffset, int batch, int maxInteractions, const VECTOR_CLASS<cl::Event> *waitForEvents, cl::Event *event = nullptr);

    size2_t workGroupSize() const { return workGroupSize_; }
    void workGroupSize(size2_t val) { workGroupSize_ = val; }
    bool useGLSharing() const { return useGLSharing_; }
    void useGLSharing(bool val) { useGLSharing_ = val; }

    void setNoSingleScattering(bool onlyMultipleScattering);

    

    bool isValid() const { return photonTracerKernel_ != nullptr; }
    bool isProgressive() const { return progressive_; }
    void setProgressive(bool val);
private:
    
    void setRandomSeedSize(size_t nPhotons);
    void compileKernels();
    size2_t workGroupSize_;
    bool useGLSharing_;
    bool progressive_ = true; // should use new random values each time called
    bool onlyMultipleScattering_ = false;

    Buffer<glm::uvec2> randomState_;

    cl::Kernel* photonTracerKernel_;
    cl::Kernel* recomputePhotonTracerKernel_;
};

} // namespace

#endif // IVW_PHOTONTRACERCL_H

