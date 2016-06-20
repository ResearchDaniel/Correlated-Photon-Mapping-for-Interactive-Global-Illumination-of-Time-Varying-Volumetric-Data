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

#include <modules/importancesamplingcl/uniformsamplegenerator2dcl.h>
#include <inviwo/core/datastructures/buffer/bufferram.h>
#include <modules/opencl/inviwoopencl.h>
#include <modules/opencl/buffer/buffercl.h>
#include <modules/opencl/buffer/bufferclgl.h>
#include <modules/opencl/image/imagecl.h>
#include <modules/opencl/syncclgl.h>


namespace inviwo {

UniformSampleGenerator2DCL::UniformSampleGenerator2DCL(bool useGLSharing)
    : SampleGenerator2DCL(useGLSharing), KernelOwner()
    , kernel_(NULL)
{
    kernel_ = addKernel("uniformsamplegenerator2d.cl", "uniformSampleGenerator2DKernel");
}


UniformSampleGenerator2DCL::~UniformSampleGenerator2DCL() {
}

void UniformSampleGenerator2DCL::reset() {

}

void UniformSampleGenerator2DCL::generateNextSamples(SampleBuffer& positionSamplesOut, const VECTOR_CLASS<cl::Event>* waitForEvents /*= nullptr*/, cl::Event* event /*= nullptr*/) {
    if (kernel_ == NULL) {
        return throw Exception("Invalid kernel: Kernel not found or failed to compile");
    }
    size2_t nSamples{static_cast<size_t>(std::sqrt(static_cast<double>(positionSamplesOut.getSize())))};
    if (getUseGLSharing()) {
        SyncCLGL glSync;
        BufferCLGL* samples = positionSamplesOut.getEditableRepresentation<BufferCLGL>();

        // Acquire shared representations before using them in OpenGL
        // The SyncCLGL object will take care of synchronization between OpenGL and OpenCL
        glSync.addToAquireGLObjectList(samples);
        glSync.aquireAllObjects();
        generateSamples(nSamples, positionSamplesOut.getSize(), samples, waitForEvents, event);
    } else {
        BufferCL* samples = positionSamplesOut.getEditableRepresentation<BufferCL>();
        generateSamples(nSamples, positionSamplesOut.getSize(), samples, waitForEvents, event);
    }
}

void UniformSampleGenerator2DCL::generateNextSamples(SampleBuffer& positionSamplesOut, SampleBuffer& directionSamplesOut, const VECTOR_CLASS<cl::Event>* waitForEvents /*= nullptr*/, cl::Event* event /*= nullptr*/) {
    cl::Event positionSampleEvent;
    generateNextSamples(positionSamplesOut, waitForEvents, &positionSampleEvent);
    std::vector<cl::Event> waitFor{ 1, positionSampleEvent };
    if (getUseGLSharing()) {
        SyncCLGL glSync;
        auto src = positionSamplesOut.getRepresentation<BufferCLGL>();
        auto dst = directionSamplesOut.getEditableRepresentation<BufferCLGL>();
        // Acquire shared representations before using them in OpenGL
        // The SyncCLGL object will take care of synchronization between OpenGL and OpenCL
        glSync.addToAquireGLObjectList(src);
        glSync.addToAquireGLObjectList(dst);
        glSync.aquireAllObjects();
        OpenCL::getPtr()->getQueue().enqueueCopyBuffer(src->get(), dst->get(), 0, 0,
            src->getSize() * src->getSizeOfElement(), &waitFor, event);
    } else {
        auto src = positionSamplesOut.getRepresentation<BufferCL>();
        auto dst = directionSamplesOut.getEditableRepresentation<BufferCL>();
        OpenCL::getPtr()->getQueue().enqueueCopyBuffer(src->get(), dst->get(), 0, 0,
            src->getSize() * src->getSizeOfElement(), &waitFor, event);
    }

}


void UniformSampleGenerator2DCL::generateSamples(const size2_t& nSamples, size_t nElements, const BufferCLBase* samplesCL, const VECTOR_CLASS<cl::Event>* waitForEvents, cl::Event* event) {
    
    auto globalWorkSize = getGlobalWorkGroupSize(nElements, getWorkGroupSize());

    int argIndex = 0;
    kernel_->setArg(argIndex++, vec2(nSamples));
    kernel_->setArg(argIndex++, static_cast<int>(nElements));
    kernel_->setArg(argIndex++, *samplesCL);
    OpenCL::getPtr()->getQueue().enqueueNDRangeKernel(*kernel_, cl::NullRange, globalWorkSize, getWorkGroupSize(), waitForEvents, event);
}





} // inviwo namespace
