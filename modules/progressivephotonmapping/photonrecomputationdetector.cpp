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

#include "photonrecomputationdetector.h"
#include <modules/opencl/buffer/buffercl.h>
#include <modules/opencl/buffer/bufferclgl.h>

namespace inviwo {

PhotonRecomputationDetector::PhotonRecomputationDetector(size_t workGroupSize, bool useGLSharing /*= false*/)
: KernelOwner(), workGroupSize_(workGroupSize), useGLSharing_(useGLSharing) {
    kernel_ = addKernel("photonrecomputationdetector.cl", "photonRecomputationDetectorKernel");
    equalImportanceKernel_ = addKernel("photonrecomputationdetector.cl", "photonRecomputationDetectorEqualImportanceKernel");
}

PhotonRecomputationDetector::~PhotonRecomputationDetector()  {
    
}

void PhotonRecomputationDetector::photonRecomputationImportance(const PhotonData* photonData, int photonOffset, const Volume* origVolume, const ImportanceUniformGrid3D* uniformGridVolume, const LightSamples& lightSamples, Buffer<unsigned int>& recomputationImportance, const VECTOR_CLASS<cl::Event> *waitForEvents, cl::Event *event, SyncCLGL* glSync) {
    if (kernel_ == NULL) {
        return;
    }

    if (recomputationImportance.getSize() != photonData->getNumberOfPhotons()) {
        recomputationImportance.setSize(photonData->getNumberOfPhotons()*photonData->getMaxPhotonInteractions());
    }
    //IVW_CPU_PROFILING("useGLSharing")
    if (glSync) {
        //SyncCLGL glSync;
        //IVW_CPU_PROFILING("useGLSharing")
        auto lightSampleCL = lightSamples.getLightSamples()->getRepresentation<BufferCLGL>();
        auto intersectionPointCL = lightSamples.getIntersectionPoints()->getRepresentation<BufferCLGL>();
        auto uniformGrid3DCL = uniformGridVolume->data.getRepresentation<BufferCLGL>();
        auto photonCL = photonData->photons_.getRepresentation<BufferCLGL>();
        auto recomputationImportanceBuf = recomputationImportance.getEditableRepresentation<BufferCL>();

        glSync->addToAquireGLObjectList(lightSampleCL);
        glSync->addToAquireGLObjectList(intersectionPointCL);
        glSync->addToAquireGLObjectList(uniformGrid3DCL);
        glSync->addToAquireGLObjectList(photonCL);
        //glSync->addToAquireGLObjectList(recomputationImportanceBuf);
        glSync->aquireAllObjects();

        photonRecomputationImportance(photonData, photonOffset, photonCL, origVolume, uniformGridVolume, uniformGrid3DCL, lightSamples, lightSampleCL, intersectionPointCL,
            recomputationImportanceBuf, waitForEvents, event);
        if (event != nullptr) {
            std::vector<cl::Event> events(1, *event);
            glSync->releaseAllGLObjects(&events);
        } else {
            glSync->releaseAllGLObjects();
        }
    } else {
        auto lightSampleCL = lightSamples.getLightSamples()->getRepresentation<BufferCL>();
        auto intersectionPointCL = lightSamples.getIntersectionPoints()->getRepresentation<BufferCL>();
        auto uniformGrid3DCL = uniformGridVolume->data.getRepresentation<BufferCL>();
        auto photonCL = photonData->photons_.getRepresentation<BufferCL>();
        auto recomputationImportanceBuf = recomputationImportance.getEditableRepresentation<BufferCL>();
        photonRecomputationImportance(photonData, photonOffset, photonCL, origVolume, uniformGridVolume, uniformGrid3DCL, lightSamples, lightSampleCL, intersectionPointCL,
            recomputationImportanceBuf, waitForEvents, event);
    }
}

void PhotonRecomputationDetector::photonRecomputationImportance(const PhotonData* photonData, int photonOffset, const BufferCLBase* photonDataCL, const Volume* origVolume, const ImportanceUniformGrid3D* uniformGridVolume, const BufferCLBase* uniformGridVolumeCL, const LightSamples& lightSamples, const BufferCLBase* lightSamplesCL, const BufferCLBase* intersectionPointsCL, BufferCLBase* recomputationImportance, const VECTOR_CLASS<cl::Event> *waitForEvents, cl::Event *event /*= nullptr*/) {
    cl::Kernel* kernel = kernel_;
    if (getEqualImportance()) {
        kernel = equalImportanceKernel_;
    }
    //IVW_CPU_PROFILING("photonRecomputationImportance")
    cl_uint argId = 0;
    kernel->setArg(argId++, *uniformGridVolumeCL);
    kernel->setArg(argId++, ivec3(uniformGridVolume->getDimensions()));
    kernel->setArg(argId++, vec3(uniformGridVolume->getCellDimension()));
    kernel->setArg(argId++, origVolume->getCoordinateTransformer().getTextureToIndexMatrix());
    kernel->setArg(argId++, origVolume->getCoordinateTransformer().getIndexToTextureMatrix());
    kernel->setArg(argId++, *photonDataCL);
    kernel->setArg(argId++, photonOffset);
    kernel->setArg(argId++, *lightSamplesCL);
    kernel->setArg(argId++, *intersectionPointsCL);
    kernel->setArg(argId++, static_cast<int>(lightSamples.getSize()));
    kernel->setArg(argId++, photonData->getMaxPhotonInteractions());
    kernel->setArg(argId++, static_cast<int>(photonData->getNumberOfPhotons()));
    kernel->setArg(argId++, *recomputationImportance);
    if (getEqualImportance()) {
        kernel->setArg(argId++, getPercentage());
        kernel->setArg(argId++, getIteration());
    }

    size_t globalWorkGroupSize(getGlobalWorkGroupSize(lightSamples.getSize(), workGroupSize()));
    OpenCL::getPtr()->getQueue().enqueueNDRangeKernel(*kernel, cl::NullRange, globalWorkGroupSize,
        workGroupSize(), waitForEvents, event);
}

} // namespace

