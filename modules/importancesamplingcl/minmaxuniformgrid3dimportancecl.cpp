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

#include "minmaxuniformgrid3dimportancecl.h"
#include <modules/opencl/image/imagecl.h>
#include <modules/opencl/image/imageclgl.h>
#include <modules/opencl/syncclgl.h>
#include <modules/opencl/volume/volumecl.h>
#include <modules/opencl/volume/volumeclgl.h>

namespace inviwo {


MinMaxUniformGrid3DImportanceCL::MinMaxUniformGrid3DImportanceCL()
    : KernelOwner()
    , tracerKernel_(NULL)
    , importance_(128 * 128) {
    tracerKernel_ = addKernel("minmaxuniformgrid3dimportance.cl", "uniformGridImportanceKernel");
}

bool MinMaxUniformGrid3DImportanceCL::computeImportance(const Volume* origVolume, const MinMaxUniformGrid3D* uniformGrid3D, const TransferFunction* transferFunction, const Image* entryPoints, const Image* exitPoints, const uvec2& workGroupSize, bool useGLSharing) {
    if (tracerKernel_ == NULL) {
        return false;
    }
    uvec2 dim = entryPoints->getColorLayer()->getDimensions();
    if (importance_.getSize() != dim.x*dim.y) {
        importance_.setSize(dim.x*dim.y);
    }


    size2_t localWorkGroupSize(workGroupSize);
    size2_t globalWorkGroupSize(getGlobalWorkGroupSize(dim.x, localWorkGroupSize.x),
        getGlobalWorkGroupSize(dim.y, localWorkGroupSize.y));
    IVW_OPENCL_PROFILING(profilingEvent, "")
        bool success;
    if (useGLSharing) {
        SyncCLGL glSync;
        //const BufferCLGL* volumeMax = volumeMaxBuffer->getRepresentation<BufferCLGL>();
        const ImageCLGL* entry = entryPoints->getRepresentation<ImageCLGL>();
        const ImageCLGL* exit = exitPoints->getRepresentation<ImageCLGL>();
        const BufferCLGL* uniformGrid3DCL = uniformGrid3D->data.getRepresentation<BufferCLGL>();
        BufferCLGL* importanceBuf = importance_.getEditableRepresentation<BufferCLGL>();
        //glSync.addToAquireGLObjectList(volumeMax);
        glSync.addToAquireGLObjectList(uniformGrid3DCL);
        glSync.addToAquireGLObjectList(entry);
        glSync.addToAquireGLObjectList(exit);
        glSync.addToAquireGLObjectList(importanceBuf);
        glSync.aquireAllObjects();

        success = computeImportance(origVolume, uniformGrid3D, uniformGrid3DCL, transferFunction, entry->getLayerCL()->get(),
            exit->getLayerCL()->get(), importanceBuf, globalWorkGroupSize,
            localWorkGroupSize,
            profilingEvent);
    } else {
        //const BufferCL* volumeMax = volumeMaxBuffer->getRepresentation<BufferCL>();
        const BufferCL* uniformGrid3DCL = uniformGrid3D->data.getRepresentation<BufferCL>();
        const ImageCL* entry = entryPoints->getRepresentation<ImageCL>();
        const ImageCL* exit = exitPoints->getRepresentation<ImageCL>();
        BufferCL* importanceBuf = importance_.getEditableRepresentation<BufferCL>();
        success = computeImportance(origVolume, uniformGrid3D, uniformGrid3DCL, transferFunction, entry->getLayerCL()->get(),
            exit->getLayerCL()->get(), importanceBuf, globalWorkGroupSize,
            localWorkGroupSize,
            profilingEvent);
    }

    return success;
}

bool MinMaxUniformGrid3DImportanceCL::computeImportance(const Volume* origVolume, const MinMaxUniformGrid3D* uniformGrid3D, const BufferCLBase* uniformGridCL, const TransferFunction* transferFunction, const cl::Image& entryPoints, const cl::Image& exitPoints, BufferCLBase* importanceBuf, const size2_t& globalWorkGroupSize, const size2_t& localWorkgroupSize, cl::Event* event) {
    // Transfer function parameters 
    if (transferFunction->getNumPoints() < 1) {
        return false;
    }
    const TransferFunctionDataPoint* lastPoint = transferFunction->getPoint(static_cast<int>(transferFunction->getNumPoints() - 1));
    uvec3 cellSize(uniformGrid3D->getCellDimension());
    vec2 transferFunctionMaxMin(
        transferFunction->getPoint(0)->getPos().y > 0.f ? 0.f : transferFunction->getPoint(0)->getPos().x,
        lastPoint->getPos().y > 0.f ? 1.f : lastPoint->getPos().x);

    try {
        int argIndex = 0;
        tracerKernel_->setArg(argIndex++, *uniformGridCL);
        //tracerKernel_->setArg(argIndex++, *volumeMax);
        tracerKernel_->setArg(argIndex++, ivec3(uniformGrid3D->getDimensions()));
        tracerKernel_->setArg(argIndex++, entryPoints);
        tracerKernel_->setArg(argIndex++, exitPoints);
        tracerKernel_->setArg(argIndex++, transferFunctionMaxMin);
        // Deal with numerical issues
        //mat4 textureToIndex = glm::translate(glm::scale(origVolume->getCoordinateTransformer().getTextureToIndexMatrix(), vec3(0.99f)), vec3(0.0001f));
        //tracerKernel_->setArg(argIndex++, textureToIndex);
        tracerKernel_->setArg(argIndex++, origVolume->getCoordinateTransformer().getTextureToIndexMatrix());
        tracerKernel_->setArg(argIndex++, origVolume->getCoordinateTransformer().getIndexToTextureMatrix());
        tracerKernel_->setArg(argIndex++, vec3(cellSize));
        tracerKernel_->setArg(argIndex++, *importanceBuf);
        OpenCL::getPtr()->getQueue().enqueueNDRangeKernel(*tracerKernel_, cl::NullRange, globalWorkGroupSize, localWorkgroupSize, NULL,
            event);
    } catch (cl::Error& err) {
        LogError(getCLErrorString(err));
        return false;
    }
    return true;
}

} // namespace

