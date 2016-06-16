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

#include "samplers.cl" 
#include "light/light.cl"
#include "datastructures/lightsample.cl"

 
__kernel void directionalLightSamplerKernel(
    __global float4 const * __restrict positionSamples
    , float4 radiance
    , float4 direction     // Light plane normal
    , float4 planeOrigin   // Light plane origin
    , float4 planeTangentU // Light plane tangent u-direction
    , float4 planeTangentV // Light plane tangent v-direction
    , float planeArea      // length(planeTangentU.xyz)*length(planeTangentV.xyz)
    , int nSamples
    , __global StoredLightSample* lightSamples
    ) 
{ 
 
    int threadId = get_global_id(0);
    if (threadId >= nSamples) {
        return;
    } 
    float4 positionSample = positionSamples[threadId]; // u,v,w, pdf
    LightSample lightSample; 
    lightSample.origin = planeOrigin.xyz + planeTangentU.xyz*positionSample.x + planeTangentV.xyz*positionSample.y;
    lightSample.direction = direction.xyz;
    float pdf = positionSample.w / (planeArea);
    lightSample.power = radiance.xyz / pdf; 

    writeLightSample(lightSample, lightSamples, threadId);  
}
