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

#ifndef IVW_UNIFORM_SAMPLE_GENERATOR_2D_CL_H
#define IVW_UNIFORM_SAMPLE_GENERATOR_2D_CL_H

#include <inviwo/core/common/inviwo.h>
#include <inviwo/core/datastructures/buffer/buffer.h>
#include <modules/opencl/image/layerclbase.h>
#include <modules/opencl/buffer/bufferclbase.h>
#include <modules/opencl/image/layerclbase.h>
#include <modules/opencl/inviwoopencl.h>
#include <modules/opencl/kernelowner.h>

#include <modules/importancesamplingcl/importancesamplingclmoduledefine.h>
#include <modules/lightcl/sample.h>
#include <modules/lightcl/samplegenerator2dcl.h>

namespace inviwo {



/**
 * \class UniformSampleGenerator2DCL
 *
 * \brief Generate samples uniformly spread in 2D.
 * 
 * Sample will be: (x \in [0 1],y \in [0 1],z = 0, pdf=1) 
 *
 * coord /in [0 nSamples-1]
 *
 * xy = (0.5 + coord)/nSamples;
 *
 */
class IVW_MODULE_IMPORTANCESAMPLINGCL_API UniformSampleGenerator2DCL : public SampleGenerator2DCL, public KernelOwner {

public:
    
    UniformSampleGenerator2DCL(bool useGLSharing = true);
    virtual ~UniformSampleGenerator2DCL();

    virtual void reset();

    virtual void generateNextSamples(SampleBuffer& positionSamplesOut, const VECTOR_CLASS<cl::Event>* waitForEvents = nullptr, cl::Event* event = nullptr);

    virtual void generateNextSamples(SampleBuffer& positionSamplesOut, SampleBuffer& directionSamplesOut, const VECTOR_CLASS<cl::Event>* waitForEvents = nullptr, cl::Event* event = nullptr);
    
private:
    void generateSamples(const size2_t& nSamples, size_t nElements, const BufferCLBase* samplesCL, const VECTOR_CLASS<cl::Event>* waitForEvents = nullptr, cl::Event* event = nullptr);
	cl::Kernel* kernel_;
};

}

#endif // IVW_UNIFORM_SAMPLE_GENERATOR_2D_CL_H


