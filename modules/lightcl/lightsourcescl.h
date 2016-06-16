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

#ifndef IVW_LIGHTSOURCESCL_H
#define IVW_LIGHTSOURCESCL_H

#include <modules/LightCL/LightCLmoduledefine.h>
#include <inviwo/core/common/inviwo.h>
#include <inviwo/core/ports/inportiterable.h>
#include <inviwo/core/datastructures/light/baselightsource.h>
#include <modules/opencl/buffer/buffercl.h>
#include <modules/opencl/light/packedlightsource.h>

namespace inviwo {



/** 
 * \brief Uploads PackedLightSources to supplied buffer.
 * Will resize lightSourcesCLOut to fit all light sources.
 *
 * @param const std::vector< std::shared_ptr<LightSource> > & lightSources Light sources to upload
 * @param const mat4 & transformation Transformation to apply to all light sources (i.e. worldToTexture)
 * @param float radianceScale Scaling of light source intensity
 * @param BufferCL * lightSourcesCLOut OpenCL buffer with light sources (size = sizeof(PackedLightSource)*lightSources.size()
 * @return the number of uploaded light sources 
 */
IVW_MODULE_LIGHTCL_API size_t uploadLightSources(InportIterable<LightSource, false>::const_iterator begin, InportIterable<LightSource, false>::const_iterator end, const mat4& transformation, float radianceScale, BufferCL* lightSourcesCLOut);


} // namespace

#endif // IVW_LIGHTSOURCESCL_H

