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

#ifndef IVW_PHOTONRECOMPUTATIONDETECTOR_H
#define IVW_PHOTONRECOMPUTATIONDETECTOR_H

#include <modules/progressivephotonmapping/progressivephotonmappingmoduledefine.h>
#include <inviwo/core/common/inviwo.h>

#include <modules/opencl/inviwoopencl.h>
#include <modules/opencl/buffer/buffercl.h>
#include <modules/opencl/kernelowner.h>
#include <modules/opencl/syncclgl.h>

#include <modules/lightcl/lightsample.h>
#include <modules/importancesamplingcl/importanceuniformgrid3d.h>
#include <modules/progressivephotonmapping/photondata.h>

namespace inviwo {

/**
 * \class PhotonRecomputationDetector
 * \brief Detect photons that need to be recomputed using a uniform grid containing recomputation importance values.
 *
 */
class IVW_MODULE_PROGRESSIVEPHOTONMAPPING_API PhotonRecomputationDetector : public KernelOwner {
public:
    PhotonRecomputationDetector(size_t workGroupSize = 64, bool useGLSharing = true);
    virtual ~PhotonRecomputationDetector();

    size_t workGroupSize() const { return workGroupSize_; }
    void workGroupSize(size_t val) { workGroupSize_ = val; }
    bool useGLSharing() const { return useGLSharing_; }
    void useGLSharing(bool val) { useGLSharing_ = val; }

    bool isValid() const { return kernel_ != nullptr; }


    void photonRecomputationImportance(const PhotonData* photonData, int photonOffset, const Volume* origVolume, const ImportanceUniformGrid3D* uniformGridVolume, const LightSamples& lightSamples, Buffer<unsigned int>& recomputationImportance, const VECTOR_CLASS<cl::Event> *waitForEvents, cl::Event *event = nullptr, SyncCLGL* glSync = nullptr);

    void photonRecomputationImportance(const PhotonData* photonData, int photonOffset, const BufferCLBase* photonDataCL, const Volume* origVolume, const ImportanceUniformGrid3D* uniformGridVolume, const BufferCLBase* uniformGridVolumeCL, const LightSamples& lightSamples, const BufferCLBase* lightSamplesCL, const BufferCLBase* intersectionPointsCL, BufferCLBase* recomputationImportance, const VECTOR_CLASS<cl::Event> *waitForEvents, cl::Event *event = nullptr);

    bool getEqualImportance() const { return equalImportance_; }
    void setEqualImportance(bool val) { equalImportance_ = val; }
    int getPercentage() const { return percentage_; }
    void setPercentage(int val) { percentage_ = val; }
    int getIteration() const { return iteration_; }
    void setIteration(int val) { iteration_ = val; }
private:
    int percentage_ = 100;
    int iteration_ = 0;
    bool equalImportance_ = false;
    size_t workGroupSize_;
    bool useGLSharing_;

    cl::Kernel* kernel_;
    cl::Kernel* equalImportanceKernel_;
};

} // namespace

#endif // IVW_PHOTONRECOMPUTATIONDETECTOR_H

