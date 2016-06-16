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

#include "lightsamplemeshintersectioncl.h"
#include <modules/opencl/buffer/buffercl.h>
#include <modules/opencl/buffer/bufferclgl.h>
#include <modules/opencl/buffer/elementbuffercl.h>
#include <modules/opencl/buffer/elementbufferclgl.h>
#include <modules/opencl/syncclgl.h>


namespace inviwo {


LightSampleMeshIntersectionCL::LightSampleMeshIntersectionCL(size_t workGroupSize /*= 128*/, bool useGLSharing /*= true*/)
: workGroupSize_(workGroupSize), useGLSharing_(useGLSharing) {
    intersectionKernel_ = addKernel("intersection/lightsamplemeshintersection.cl", "lightSampleMeshIntersectionKernel");
}

LightSampleMeshIntersectionCL::~LightSampleMeshIntersectionCL()  {
    
}

void LightSampleMeshIntersectionCL::meshSampleIntersection(const Mesh* mesh, LightSamples* samples) {
    const BufferRAMPrecision<vec3>* vertices = dynamic_cast<const BufferRAMPrecision<vec3>*>(mesh->getBuffer(0)->getRepresentation<BufferRAM>());
    if (vertices == nullptr) {
        return;
    }
    //IVW_OPENCL_PROFILING(intersectionEvent, "Intersection computation")
    cl::Event* intersectionEvent = nullptr;
    try {
        if (useGLSharing_) {
            SyncCLGL glSync;
            auto lightSamplesCL = samples->getLightSamples()->getEditableRepresentation<BufferCLGL>();
            auto verticesCL = mesh->getBuffer(0)->getRepresentation<BufferCLGL>();
            auto indicesCL = mesh->getIndicies(0)->getRepresentation<ElementBufferCLGL>();
            auto intersectionPointsCL = samples->getIntersectionPoints()->getEditableRepresentation<BufferCLGL>();
            // Acquire shared representations before using them in OpenGL
            // The SyncCLGL object will take care of synchronization between OpenGL and OpenCL
            glSync.addToAquireGLObjectList(lightSamplesCL);
            glSync.addToAquireGLObjectList(verticesCL);
            glSync.addToAquireGLObjectList(indicesCL);
            glSync.addToAquireGLObjectList(intersectionPointsCL);
            glSync.aquireAllObjects();

            meshSampleIntersection(verticesCL, indicesCL, indicesCL->getSize(), samples->getSize(), lightSamplesCL, intersectionPointsCL, nullptr, intersectionEvent);
        } else {
            auto lightSamplesCL = samples->getLightSamples()->getEditableRepresentation<BufferCL>();
            auto verticesCL = mesh->getBuffer(0)->getRepresentation<BufferCL>();
            auto indicesCL = mesh->getIndicies(0)->getRepresentation<ElementBufferCL>();
            auto intersectionPointsCL = samples->getIntersectionPoints()->getEditableRepresentation<BufferCL>();
            meshSampleIntersection(verticesCL, indicesCL, indicesCL->getSize(), samples->getSize(), lightSamplesCL, intersectionPointsCL, nullptr, intersectionEvent);
        }

    } catch (cl::Error& err) {
        LogError(getCLErrorString(err));
    };
}

void LightSampleMeshIntersectionCL::meshSampleIntersection(const BufferCLBase* verticesCL, const BufferCLBase* indicesCL, size_t nIndices, size_t nSamples, const BufferCLBase* lightSamplesCL, BufferCLBase* intersectionPointsCL, const VECTOR_CLASS<cl::Event>* waitForEvents /*= nullptr*/, cl::Event* event /*= nullptr*/) {
    int argIndex = 0;
    intersectionKernel_->setArg(argIndex++, *verticesCL);
    intersectionKernel_->setArg(argIndex++, *indicesCL);
    intersectionKernel_->setArg(argIndex++, static_cast<int>(nIndices));
    intersectionKernel_->setArg(argIndex++, *lightSamplesCL);
    intersectionKernel_->setArg(argIndex++, static_cast<int>(nSamples));
    intersectionKernel_->setArg(argIndex++, *intersectionPointsCL);

    size_t globalWorkSizeX = getGlobalWorkGroupSize(nSamples, workGroupSize_);

    OpenCL::getPtr()->getQueue().enqueueNDRangeKernel(*intersectionKernel_, cl::NullRange, globalWorkSizeX, workGroupSize_, NULL, event);
}

} // namespace

