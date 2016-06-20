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

#ifndef IVW_PROGRESSIVE_PHOTON_TRACER_CL_H
#define IVW_PROGRESSIVE_PHOTON_TRACER_CL_H

#include <modules/progressivephotonmapping/progressivephotonmappingmoduledefine.h>
#include <inviwo/core/common/inviwo.h>

#include <inviwo/core/datastructures/light/baselightsource.h>
#include <inviwo/core/datastructures/buffer/buffer.h>
#include <inviwo/core/datastructures/light/directionallight.h>
#include <inviwo/core/ports/imageport.h>
#include <inviwo/core/ports/volumeport.h>
#include <inviwo/core/processors/processor.h>
#include <inviwo/core/properties/advancedmaterialproperty.h>
#include <inviwo/core/properties/cameraproperty.h>
#include <inviwo/core/properties/minmaxproperty.h>
#include <inviwo/core/properties/stringproperty.h>
#include <inviwo/core/properties/transferfunctionproperty.h>
#include <modules/opengl/shader/shader.h>
#include <modules/opengl/image/imagegl.h>

#include <modules/opencl/inviwoopencl.h>
#include <modules/opencl/kernelowner.h>
#include <modules/opencl/buffer/buffercl.h>
#include <inviwo/core/ports/bufferport.h>

#include <modules/lightcl/lightsample.h>

#include <modules/progressivephotonmapping/photondata.h>
#include <modules/progressivephotonmapping/photontracercl.h>
#include <modules/progressivephotonmapping/photonrecomputationdetector.h>

#include <modules/importancesamplingcl/importanceuniformgrid3d.h>

#include <clogs/clogs.h>

namespace inviwo {

/** \docpage{<classIdentifier>, ProgressivePhotonTracerCL}
 * Photon tracer processor to be connected with a photon gathering processor (PhotonToLightVolumeProcessorCL).
 *
 * ### Inports
 *   * __volume__                   Volume data.
 *   * __recomputationImportance__  Optional importance grid.
 *   * __LightSamples__             Light source samples.
 * ### Outports
 *   * __photons__ Traced photons.
 *   * __recomputedIndices__ List of indices to recomputed photons if importance grid is used.
 * 
 * ### Properties
 *   * __<Prop1>__ <description>.
 *   * __<Prop2>__ <description>
 */


/**
 * \class ProgressivePhotonTracerCL
 *
 * \brief <brief description> 
 *
 * <Detailed description from a developer prespective>
 */
class IVW_MODULE_PROGRESSIVEPHOTONMAPPING_API ProgressivePhotonTracerCL : public Processor, public KernelObserver, public KernelOwner {
public:
    ProgressivePhotonTracerCL();
    ~ProgressivePhotonTracerCL() = default;

    virtual const ProcessorInfo getProcessorInfo() const override;
    static const ProcessorInfo processorInfo_;

    virtual void process() override;

    

    virtual void onKernelCompiled(const cl::Kernel* kernel) override;

protected:

    void kernelArgChanged();
    void onTimerEvent();

    float getSceneRadius() const;

    void invalidateProgressiveRendering(PhotonData::InvalidationReason invalidationFlag);
    // Invalidate without resetting iterations
    void evaluateProgressiveRefinement();
    void phaseFunctionChanged();

    void onClipChange();

    void progressiveRefinementChanged();
    void noSingleScatteringChanged();

    void sortIndicesByImportance(const BufferBase* keys, BufferCLBase* keysCL, const BufferBase* data, const BufferCLBase* dataCL, const VECTOR_CLASS<cl::Event> *waitForEvents, cl::Event *event = nullptr);
    void sortIndices(const BufferBase* keys, BufferCLBase* keysCL, const BufferBase* values, BufferCLBase* valuesCL, size_t nElements, const VECTOR_CLASS<cl::Event> *waitForEvents, cl::Event *event = nullptr);
    int reduceInts(const BufferCLBase* dataCL, size_t nElements, bool blocking, const VECTOR_CLASS<cl::Event> *waitForEvents, cl::Event *readBackEvent = nullptr, cl::Event *reduceEvent = nullptr);
    void resetPhotonImportance(size_t offset, size_t nPhotons, const VECTOR_CLASS<cl::Event> *waitForEvents = nullptr, cl::Event* event = nullptr);
    // Sorting algorithm
    std::unique_ptr<clogs::Radixsort> recomputationImportanceSorter_;
    size_t sortKeysTempBufferSize_ = 0;
    size_t sortDataTempBufferSize_ = 0;
    std::unique_ptr<clogs::Radixsort> recomputationIndexSorter_;
    size_t sortIndicesTempBufferSize_ = 0;
    std::unique_ptr<clogs::Reduce> reduce_;
private:
    VolumeInport volumePort_;
    UniformGrid3DInport recomputationImportanceGrid_;
    MultiDataInport<LightSamples> lightSamples_;

    DataOutport<PhotonData> outport_;
    DataOutport<RecomputedPhotonIndices> recomputedIndicesPort_;

    FloatProperty samplingRate_;
    FloatProperty radius_;
    FloatProperty sceneRadianceScaling_;
    FloatProperty alphaProp_;

    IntProperty maxScatteringEvents_;
    BoolProperty noSingleScattering_;
    // Material properties
    TransferFunctionProperty transferFunction_;
    AdvancedMaterialProperty advancedMaterial_;

    IntVec2Property workGroupSize_;
    BoolProperty useGLSharing_;

    CameraProperty camera_;

    FloatProperty maxIncrementalPhotonsToUpdate_; // Percentage of photons to update when TF/time-volume changes
    BoolProperty equalIncrementalImportance_; // All photons to update receive equal importance.
    int remainingPhotonsOffset_ = 0;
    int remainingPhotonsToUpdate_ = -1;
    BoolProperty spatialSorting_;
    // Enables step by step invalidation and 
    // other processors to react on an invalidation
    // from this processor
    ButtonProperty invalidateRendering_;
    BoolProperty enableProgressiveRefinement_;
    BoolProperty enableProgressivePhotonRecomputation_;

    IntMinMaxProperty clipX_;
    IntMinMaxProperty clipY_;
    IntMinMaxProperty clipZ_;

    std::shared_ptr<PhotonData> photonData_;
    PhotonData::InvalidationReason invalidationFlag_ = PhotonData::InvalidationReason::All;

    BufferCL axisAlignedBoundingBoxCL_;

    PhotonTracerCL photonTracer_;

    PhotonRecomputationDetector photonRecomputationDetector_;
    Buffer<unsigned int> photonRecomputationImportance_; // Must be unsigned integer type for sorting to work (radix sort)
    Buffer<unsigned int> photonRecomputationHashed_; // Must be unsigned integer type for sorting to work
    std::shared_ptr<RecomputedPhotonIndices> recomputedPhotonIndices_; // Spatially sorted indices
    Buffer<unsigned int> thresholdPhotonRecomputation_;
    cl::Kernel* indexToBuffer_;
    cl::Kernel* thresholdKernel_;
    cl::Kernel* lightSampleHashKernel_;
    
    // Timer
    Timer progressiveTimer_;
};

} // namespace

#endif // IVW_PROGRESSIVE_PHOTON_TRACER_CL_H
