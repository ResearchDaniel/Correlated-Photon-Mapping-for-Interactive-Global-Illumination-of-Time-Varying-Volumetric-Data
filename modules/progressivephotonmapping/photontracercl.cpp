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

#include "photontracercl.h"
#include <inviwo/core/datastructures/image/layer.h>
#include <modules/opencl/syncclgl.h>
#include <modules/opencl/image/imagecl.h>
#include <modules/opencl/image/layercl.h>
#include <modules/opencl/image/layerclgl.h>
#include <modules/opencl/volume/volumeclgl.h>
#include <modules/opencl/volume/volumecl.h>
#include <modules/opencl/buffer/bufferclgl.h>

#include <modules/rndgenmwc64x/mwc64xseedgenerator.h>

namespace inviwo {

uvec2 getSamplesPerLight(uvec2 nSamples, int nLightSources) {
    uvec2 samplesPerLight;
    // samplesPerLight.y = nPhotons.y / nLightSources;
    // samplesPerLight.x = (int)sqrt((float)nPhotons.x*samplesPerLight.y);
    unsigned int nPhotons = nSamples.x * nSamples.y;
    samplesPerLight.y = nPhotons / nLightSources;
    samplesPerLight.x = (unsigned int)sqrt((float)samplesPerLight.y);
    samplesPerLight.y = samplesPerLight.x * samplesPerLight.x;
    return samplesPerLight;
}





PhotonTracerCL::PhotonTracerCL(size2_t workGroupSize /*= size2_t(8, 8)*/, bool useGLSharing /*= false*/)
: KernelOwner(), workGroupSize_(workGroupSize), useGLSharing_(useGLSharing) {
    compileKernels();
}

void PhotonTracerCL::tracePhotons(const Volume* volume, const TransferFunction& transferFunction, const BufferCL* axisAlignedBoundingBoxCL, const AdvancedMaterialProperty& material, const Camera* camera, float stepSize, const LightSamples* lightSamples, const Buffer<unsigned int>* photonsToRecomputeIndices, int nInvalidPhotons, int photonOffset, int batch, int maxInteractions, PhotonData* photonOutData, const VECTOR_CLASS<cl::Event> *waitForEvents, cl::Event *event /*= nullptr*/) {
    if (!photonTracerKernel_) {
        return;
    }
    if (randomState_.getSize() != photonOutData->getNumberOfPhotons()) {
        setRandomSeedSize(photonOutData->getNumberOfPhotons());
    }
    auto volumeDim = volume->getDimensions();
    // Texture space spacing
    const mat4 volumeTextureToWorld = volume->getCoordinateTransformer().getTextureToWorldMatrix();
    const mat4 textureToIndexMatrix = volume->getCoordinateTransformer().getTextureToIndexMatrix();
    vec3 voxelSpacing(1.f / glm::length(textureToIndexMatrix[0]), 1.f / glm::length(textureToIndexMatrix[1]), 1.f / glm::length(textureToIndexMatrix[2]));
    try {
        if (useGLSharing_) {
            
            SyncCLGL glSync;
            auto volumeCL = volume->getRepresentation<VolumeCLGL>();
            const BufferCLGL* lightSamplesCL = lightSamples->getLightSamples()->getRepresentation<BufferCLGL>();
            const BufferCLGL* intersectionPointsCL = lightSamples->getIntersectionPoints()->getRepresentation<BufferCLGL>();
            BufferCLGL* photonCL = photonOutData->photons_.getEditableRepresentation<BufferCLGL>();
            
            const LayerCLGL* transferFunctionCL = transferFunction.getData()->getRepresentation<LayerCLGL>();
            const BufferCLGL* photonsToRecomputeIndicesCL = nullptr;
            
            // Acquire shared representations before using them in OpenGL
            // The SyncCLGL object will take care of synchronization between OpenGL and OpenCL
            glSync.addToAquireGLObjectList(volumeCL);
            glSync.addToAquireGLObjectList(lightSamplesCL);
            glSync.addToAquireGLObjectList(intersectionPointsCL);
            glSync.addToAquireGLObjectList(photonCL);
            glSync.addToAquireGLObjectList(transferFunctionCL);
            //{IVW_CPU_PROFILING("aquireAllObjects")
            if (photonsToRecomputeIndices) {
                photonsToRecomputeIndicesCL = photonsToRecomputeIndices->getRepresentation<BufferCLGL>();
                glSync.addToAquireGLObjectList(photonsToRecomputeIndicesCL);
            }
            
            
            glSync.aquireAllObjects();
            //}
            //{IVW_CPU_PROFILING("tracePhotons")
            tracePhotons(photonOutData, volumeCL, volumeCL->getVolumeStruct(volume), axisAlignedBoundingBoxCL
                         , transferFunctionCL, material, stepSize, lightSamplesCL, intersectionPointsCL, lightSamples->getSize(), photonsToRecomputeIndicesCL, nInvalidPhotons
                         , photonCL, photonOffset, batch, maxInteractions
                         , waitForEvents, event);
            //}
        } else {
            const VolumeCL* volumeCL = volume->getRepresentation<VolumeCL>();
            const BufferCL* lightSamplesCL = lightSamples->getLightSamples()->getRepresentation<BufferCL>();
            const BufferCL* intersectionPointsCL = lightSamples->getIntersectionPoints()->getRepresentation<BufferCL>();
            BufferCL* photonCL = photonOutData->photons_.getEditableRepresentation<BufferCL>();
            const LayerCL* transferFunctionCL = transferFunction.getData()->getRepresentation<LayerCL>();
            const BufferCL* photonsToRecomputeIndicesCL = nullptr;
            if (photonsToRecomputeIndices) {
                photonsToRecomputeIndicesCL = photonsToRecomputeIndices->getRepresentation<BufferCL>();
            }
            tracePhotons(photonOutData, volumeCL, volumeCL->getVolumeStruct(volume), axisAlignedBoundingBoxCL
                         , transferFunctionCL, material, stepSize, lightSamplesCL, intersectionPointsCL, lightSamples->getSize(), photonsToRecomputeIndicesCL, nInvalidPhotons
                         , photonCL, photonOffset, batch, maxInteractions
                         , waitForEvents, event);
        }
    } catch (cl::Error& err) {
        LogError(getCLErrorString(err));
    }
    
}


void PhotonTracerCL::tracePhotons(PhotonData* photonData, const VolumeCLBase* volumeCL, const Buffer<glm::u8>& volumeStruct, const BufferCL* axisAlignedBoundingBoxCL, const LayerCLBase* transferFunctionCL, const AdvancedMaterialProperty& material, float stepSize, const BufferCLBase* lightSamplesCL, const BufferCLBase* intersectionPointsCL, size_t nLightSamples, const BufferCLBase* photonsToRecomputeIndicesCL, int nInvalidPhotons, BufferCLBase* photonsCL, int photonOffset, int batch, int maxInteractions, const VECTOR_CLASS<cl::Event> *waitForEvents, cl::Event *event /*= nullptr*/) {
    
    cl::Kernel* kernel;
    
    cl_uint tracerArg = 0;
    if (photonsToRecomputeIndicesCL) {
        kernel = recomputePhotonTracerKernel_;
        kernel->setArg(tracerArg++, *photonsToRecomputeIndicesCL);
        kernel->setArg(tracerArg++, nInvalidPhotons);
    } else {
        kernel = photonTracerKernel_;
    }
    kernel->setArg(tracerArg++, *volumeCL);
    kernel->setArg(tracerArg++, volumeStruct);
    kernel->setArg(tracerArg++, *axisAlignedBoundingBoxCL);
    kernel->setArg(tracerArg++, *transferFunctionCL);
    kernel->setArg(tracerArg++, *transferFunctionCL); // TODO: Replace with scattering or remove
    kernel->setArg(tracerArg++, material.getCombinedMaterialParameters());
    kernel->setArg(tracerArg++, *(randomState_.getEditableRepresentation<BufferCL>()));
    kernel->setArg(tracerArg++, stepSize);
    kernel->setArg(tracerArg++, *photonsCL);
    kernel->setArg(tracerArg++, photonData->iteration());
    kernel->setArg(tracerArg++, photonOffset);
    kernel->setArg(tracerArg++, batch);
    kernel->setArg(tracerArg++, *lightSamplesCL);
    kernel->setArg(tracerArg++, *intersectionPointsCL);
    kernel->setArg(tracerArg++, static_cast<int>(nLightSamples));
    kernel->setArg(tracerArg++, maxInteractions);
    kernel->setArg(tracerArg++, material.getPhaseFunctionEnum());
    //kernel->setArg(tracerArg++, 1); // Random light sampling
    kernel->setArg(tracerArg++, photonData->iteration() > 1 ? 1 : 0); // Random light sampling
    kernel->setArg(tracerArg++, static_cast<int>(photonData->getNumberOfPhotons()));
    auto globalWorkSize = getGlobalWorkGroupSize(nLightSamples, workGroupSize_.x*workGroupSize_.y);
    if (photonsToRecomputeIndicesCL) {
        globalWorkSize = getGlobalWorkGroupSize(nInvalidPhotons, workGroupSize_.x*workGroupSize_.y);
    }
    //size_t globalWorkSizeY = getGlobalWorkGroupSize(nPhotons.y, workGroupSize_.y);
    OpenCL::getPtr()->getQueue().enqueueNDRangeKernel(*kernel, cl::NullRange, globalWorkSize,
                                                      workGroupSize_.x*workGroupSize_.y, waitForEvents, event);
}

void PhotonTracerCL::setRandomSeedSize(size_t nPhotons) {
    if (nPhotons > 0) {
        randomState_.setSize(nPhotons);
        MWC64XSeedGenerator seedGenerator;
        seedGenerator.generateRandomSeeds(&randomState_, 0, false);
    }
}

void PhotonTracerCL::setNoSingleScattering(bool onlyMultipleScattering) {
    onlyMultipleScattering_ = onlyMultipleScattering;
    compileKernels();
    
}

void PhotonTracerCL::setProgressive(bool val) {
    if (val != progressive_) {
        progressive_ = val;
        compileKernels();
    }
    
}

void PhotonTracerCL::compileKernels() {
    removeKernel(photonTracerKernel_);
    removeKernel(recomputePhotonTracerKernel_);
    std::string defines = "";
    if (onlyMultipleScattering_) {
        defines = " -D NO_SINGLE_SCATTERING";
    }
    if (isProgressive()) {
        defines = " -D PROGRESSIVE_PHOTON_MAPPING";
    }
    photonTracerKernel_ = addKernel("photontracer.cl", "photonTracerKernel", "", defines);
    recomputePhotonTracerKernel_ = addKernel("photontracer.cl", "photonTracerKernel", "", defines + " -D PHOTON_RECOMPUTATION");
}

} // namespace

