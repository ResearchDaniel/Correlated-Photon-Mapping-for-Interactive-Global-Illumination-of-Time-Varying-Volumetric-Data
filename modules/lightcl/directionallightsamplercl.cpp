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

#include "directionallightsamplercl.h"
#include <inviwo/core/datastructures/light/directionallight.h>

#include <modules/lightcl/convexhull2d.h>
#include <modules/lightcl/orientedboundingbox2d.h>
#include <modules/lightcl/lightsourcescl.h>

#include <modules/opencl/buffer/bufferclgl.h>
#include <modules/opencl/syncclgl.h>




namespace inviwo {
    
DirectionalLightSamplerCL::DirectionalLightSamplerCL(size_t workGroupSize /*= 128*/, bool useGLSharing /*= true*/)
: LightSourceSamplerCL(nullptr, nullptr), KernelOwner(), useGLSharing_(useGLSharing), workGroupSize_(workGroupSize) {
    kernel_ = addKernel("directionallightsampler.cl", "directionalLightSamplerKernel");
}

DirectionalLightSamplerCL::~DirectionalLightSamplerCL()  {
    
}

void DirectionalLightSamplerCL::sampleLightSource(const Mesh* mesh, const SampleBuffer* samples, const LightSource* light, LightSamples& lightSamplesOut) {
    const BufferRAMPrecision<vec3>* vertices = dynamic_cast<const BufferRAMPrecision<vec3>*>(mesh->getBuffer(0)->getRepresentation<BufferRAM>());
    if (vertices == nullptr) {
        return ;
    }
    if (samples->getSize() != lightSamplesOut.getSize()) {
        lightSamplesOut.setSize(samples->getSize());
    }
    
    //const DirectionalLight* light = lights_.getData().get();
    PackedLightSource lightBase = baseLightToPackedLight(light, 1.f, mesh->getCoordinateTransformer().getWorldToDataMatrix());
    
    vec3 lightDirection = glm::normalize(vec3(lightBase.tm * vec4(0.f, 0.f, 1.f, 0.f)));
    vec3 u, v;
    vec3 lightOrigin{ lightBase.tm*vec4(0.f, 0.f, 0.f, 1.f) };
    
    std::tie(lightOrigin, u, v) = geometry::fitPlaneAlignedOrientedBoundingBox2D(vertices->getDataContainer(), Plane(lightOrigin, lightDirection));
    
    float area = glm::length(u) * glm::length(v);
    //LogInfo("Bounding box center: " << lightOrigin + 0.5f*(u + v));
    //LogInfo("direction, o, lightU, lightV:" << lightDirection << o << u << v);
    bool useGLSharing = true;
    //IVW_OPENCL_PROFILING(profilingEvent, "Light sampling")
    cl::Event* profilingEvent = nullptr;
    //IVW_OPENCL_PROFILING(intersectionEvent, "Intersection computation")
    try {
        if (useGLSharing) {
            SyncCLGL glSync;
            auto samplesCL = samples->getRepresentation<BufferCLGL>();
            auto lightSamplesCL = lightSamplesOut.getLightSamples()->getEditableRepresentation<BufferCLGL>();
            auto verticesCL = mesh->getBuffer(0)->getRepresentation<BufferCLGL>();
            auto indicesCL = mesh->getIndexBuffers().front().second->getRepresentation<BufferCLGL>();
            auto intersectionPointsCL = lightSamplesOut.getIntersectionPoints()->getEditableRepresentation<BufferCLGL>();
            // Acquire shared representations before using them in OpenGL
            // The SyncCLGL object will take care of synchronization between OpenGL and OpenCL
            glSync.addToAquireGLObjectList(samplesCL);
            glSync.addToAquireGLObjectList(lightSamplesCL);
            glSync.addToAquireGLObjectList(verticesCL);
            glSync.addToAquireGLObjectList(indicesCL);
            glSync.addToAquireGLObjectList(intersectionPointsCL);
            glSync.aquireAllObjects();
            
            sampleLightSource(samplesCL, lightBase.radiance, lightDirection, lightOrigin, u, v, area, samples->getSize(), lightSamplesCL, nullptr, profilingEvent);
            
        } else {
            auto samplesCL = samples->getRepresentation<BufferCL>();
            auto lightSamplesCL = lightSamplesOut.getLightSamples()->getEditableRepresentation<BufferCL>();
            
            sampleLightSource(samplesCL, lightBase.radiance, lightDirection, lightOrigin, u, v, area, samples->getSize(), lightSamplesCL, nullptr, profilingEvent);
        }
        
    } catch (cl::Error& err) {
        LogError(getCLErrorString(err));
    };
    lightSamplesOut.advanceIteration();
}

void DirectionalLightSamplerCL::sampleLightSource(const BufferCLBase* samplesCL, vec3 radiance, vec3 lightDirection, vec3 lightOrigin, vec3 u, vec3 v, float area, size_t nSamples, BufferCLBase* lightSamplesCL, const VECTOR_CLASS<cl::Event>* waitForEvents /*= nullptr*/, cl::Event* event /*= nullptr*/) {
    int argIndex = 0;
    kernel_->setArg(argIndex++, *samplesCL);
    kernel_->setArg(argIndex++, radiance);
    kernel_->setArg(argIndex++, lightDirection);
    kernel_->setArg(argIndex++, lightOrigin);
    kernel_->setArg(argIndex++, u);
    kernel_->setArg(argIndex++, v);
    kernel_->setArg(argIndex++, area);
    kernel_->setArg(argIndex++, static_cast<int>(nSamples));
    kernel_->setArg(argIndex++, *lightSamplesCL);
    
    size_t globalWorkSizeX = getGlobalWorkGroupSize(nSamples, workGroupSize_);
    
    OpenCL::getPtr()->getQueue().enqueueNDRangeKernel(*kernel_, cl::NullRange, globalWorkSizeX, workGroupSize_, waitForEvents, event);
    
}

void DirectionalLightSamplerCL::sampleLightSource(const Mesh* mesh, LightSamples& lightSamplesOut, const VECTOR_CLASS<cl::Event>* waitForEvents /*= nullptr*/, cl::Event* event /*= nullptr*/) {
    const LightSource* light = lightSource_.get();
    const BufferRAMPrecision<vec3>* vertices = dynamic_cast<const BufferRAMPrecision<vec3>*>(mesh->getBuffer(0)->getRepresentation<BufferRAM>());
    if (vertices == nullptr || sampleGenerator_ == nullptr) {
        return ;
    }
    if (samples_.getSize() != lightSamplesOut.getSize()) {
        samples_.setSize(lightSamplesOut.getSize());
    }
    std::vector<cl::Event> sampleGenEvents(1);
    sampleGenerator_->setUseGLSharing(false);
    sampleGenerator_->generateNextSamples(samples_, waitForEvents, &sampleGenEvents[0]);
    
    //const DirectionalLight* light = lights_.getData().get();
    PackedLightSource lightBase = baseLightToPackedLight(light, 1.f, mesh->getCoordinateTransformer().getWorldToDataMatrix());
    
    vec3 lightDirection = glm::normalize(vec3(lightBase.tm * vec4(0.f, 0.f, 1.f, 0.f)));
    vec3 u, v;
    vec3 lightOrigin{ (lightBase.tm*vec4(0.f, 0.f, 0.f, 1.f)) };
    
    std::tie(lightOrigin, u, v) = geometry::fitPlaneAlignedOrientedBoundingBox2D(vertices->getDataContainer(), Plane(lightOrigin, lightDirection));
    
    float area = glm::length(u) * glm::length(v);
    //LogInfo("Bounding box center: " << lightOrigin + 0.5f*(u + v));
    //LogInfo("direction, o, lightU, lightV:" << lightDirection << o << u << v);
    bool useGLSharing = true;
    IVW_OPENCL_PROFILING(profilingEvent, "Light sampling")
    //IVW_OPENCL_PROFILING(intersectionEvent, "Intersection computation")
    try {
        auto samplesCL = samples_.getRepresentation<BufferCL>();
        if (useGLSharing) {
            SyncCLGL glSync;
            
            BufferCLGL* lightSamplesCL = lightSamplesOut.getLightSamples()->getEditableRepresentation<BufferCLGL>();
            // Acquire shared representations before using them in OpenGL
            // The SyncCLGL object will take care of synchronization between OpenGL and OpenCL
            glSync.addToAquireGLObjectList(lightSamplesCL);
            glSync.aquireAllObjects();
            
            sampleLightSource(samplesCL, lightBase.radiance, lightDirection, lightOrigin, u, v, area, samples_.getSize(), lightSamplesCL, &sampleGenEvents, profilingEvent);
            
        } else {
            BufferCL* lightSamplesCL = lightSamplesOut.getLightSamples()->getEditableRepresentation<BufferCL>();
            
            sampleLightSource(samplesCL, lightBase.radiance, lightDirection, lightOrigin, u, v, area, samples_.getSize(), lightSamplesCL, &sampleGenEvents, profilingEvent);
        }
        
    } catch (cl::Error& err) {
        LogError(getCLErrorString(err));
    };
    lightSamplesOut.advanceIteration();
    
}
    
} // namespace


