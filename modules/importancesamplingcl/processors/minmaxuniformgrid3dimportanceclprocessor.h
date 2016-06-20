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
#ifndef IVW_MINMAXUNIFORMGRID3DIMPORTANCECLPROCESSOR_H
#define IVW_MINMAXUNIFORMGRID3DIMPORTANCECLPROCESSOR_H

#include <modules/importancesamplingcl/importancesamplingclmoduledefine.h>
#include <inviwo/core/common/inviwo.h>
#include <inviwo/core/processors/processor.h>
#include <inviwo/core/properties/boolproperty.h>
#include <inviwo/core/properties/ordinalproperty.h>
#include <inviwo/core/properties/transferfunctionproperty.h>


#include <modules/opencl/inviwoopencl.h>
#include <modules/opencl/buffer/bufferclbase.h>
#include <modules/opencl/kernelowner.h>


#include <modules/uniformgridcl/minmaxuniformgrid3d.h>
#include <modules/importancesamplingcl/importanceuniformgrid3d.h>
#include <modules/uniformgridcl/processors/dynamicvolumedifferenceanalysis.h>

namespace inviwo {

/** \docpage{<classIdentifier>, MinMaxUniformGrid3DImportanceCLProcessor}
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
 * \class MinMaxUniformGrid3DImportanceCLProcessor
 *
 * \brief Compute the importance of each grid point based on Transfer function content
 *
 * <Detailed description from a developer prespective>
 */
class IVW_MODULE_IMPORTANCESAMPLINGCL_API
    MinMaxUniformGrid3DImportanceCLProcessor : public Processor,
                                               public ProcessorKernelOwner {
public:
  enum class InvalidationReason {
    TransferFunction = 1 << 0,
    Volume = 1 << 1,
    All = TransferFunction | Volume
  };
  virtual const ProcessorInfo getProcessorInfo() const override;
  static const ProcessorInfo processorInfo_;
  MinMaxUniformGrid3DImportanceCLProcessor();
  virtual ~MinMaxUniformGrid3DImportanceCLProcessor() = default;

  virtual void process() override;

  void computeImportance(const BufferCLBase *minMaxUniformGridCL,
                         size_t nElements,
                         BufferCLBase *importanceUniformGridCL,
                         const size_t &globalWorkGroupSize,
                         const size_t &localWorkgroupSize, cl::Event *event);
  void computeImportance(const BufferCLBase *minMaxUniformGridCL, const BufferCLBase *prevMinMaxUniformGridCL,
                         const BufferCLBase *volumeDifferenceInfoUniformGridCL,
                         size_t nElements,
                         BufferCLBase *importanceUniformGridCL,
                         const size_t &globalWorkGroupSize,
                         const size_t &localWorkgroupSize, cl::Event *event);

  float getLabColorNormalizationFactor() const;

  void updateTransferFunctionData();

  void updateTransferFunctionDifferenceData();

  vec4 tfPointColorDiff(const vec4 &p1, const vec4 &p2);

protected:
  void setInvalidationReason(InvalidationReason invalidationFlag);
  TransferFunctionDataPoint mix(const TransferFunctionDataPoint &a,
                                const TransferFunctionDataPoint &b,
                                const TransferFunctionDataPoint &t);
  TransferFunctionDataPoint mix(const TransferFunctionDataPoint &a,
                                const TransferFunctionDataPoint &b, float t);
  UniformGrid3DInport minMaxUniformGrid3DInport_;  // Uniform grid with minimum
                                                   // and maximum volume data
                                                   // values
  UniformGrid3DInport volumeDifferenceInfoInport_; // Optional data on
                                                   // difference between
                                                   // previous and current
                                                   // volume.
  UniformGrid3DOutport importanceUniformGrid3DOutport_;

  FloatProperty opacityWeight_;
  FloatProperty opacityDiffWeight_;
  FloatProperty colorWeight_;
  FloatProperty colorDiffWeight_;
  BoolProperty useAssociatedColor_;

  FloatProperty TFPointEpsilon_; // Threshold for considering two TF points different

  TransferFunctionProperty transferFunction_;
  TransferFunction prevTransferFunction_;
  IntProperty workGroupSize_;
  BoolProperty useGLSharing_;

  Buffer<float> tfPointPositions_;
  Buffer<vec4> tfPointColors_;
  int tfPointImportanceSize_ = 0;
  InvalidationReason invalidationFlag_ = InvalidationReason::All;
  bool tfChanged_ = true;
  cl::Kernel *kernel_;
  cl::Kernel *timeVaryingKernel_;

  std::shared_ptr<const MinMaxUniformGrid3D>
      prevMinMaxUniformGrid3D_; ///< Previous time-step

  std::shared_ptr<ImportanceUniformGrid3D> importanceUniformGrid3D_;
};

inline MinMaxUniformGrid3DImportanceCLProcessor::InvalidationReason
operator|(MinMaxUniformGrid3DImportanceCLProcessor::InvalidationReason a,
          MinMaxUniformGrid3DImportanceCLProcessor::InvalidationReason b) {
  return static_cast<
      MinMaxUniformGrid3DImportanceCLProcessor::InvalidationReason>(
      static_cast<int>(a) | static_cast<int>(b));
}
inline MinMaxUniformGrid3DImportanceCLProcessor::InvalidationReason &
operator|=(MinMaxUniformGrid3DImportanceCLProcessor::InvalidationReason &a,
           MinMaxUniformGrid3DImportanceCLProcessor::InvalidationReason b) {
  return a = a | b;
}

} // namespace

#endif // IVW_MINMAXUNIFORMGRID3DIMPORTANCECLPROCESSOR_H

