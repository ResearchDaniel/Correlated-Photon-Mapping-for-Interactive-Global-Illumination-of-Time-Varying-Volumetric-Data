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

#ifndef HASHLIGHTSAMPLE_CL
#define HASHLIGHTSAMPLE_CL
#include "datastructures/lightsample.cl"


__kernel void hashLightSampleKernel(
    __global StoredLightSample const* __restrict lightSamples
    , __global StoredIntersectionPoint const* __restrict intersectionPoints // tStart, tEnd for each light sample  
    , int nLightSourceSamples
    , __global unsigned int const* __restrict lightSampleId // tStart, tEnd for each light sample 
    , int nLightSampleIds
    , float3 cellSize, int3 nBlocks
    , __global uint* whichBucket
    , int outOffset) {

    if (get_global_id(0) >= nLightSampleIds) {
        return;
    }
    uint id = lightSampleId[get_global_id(0)];
    if (id < outOffset || id >= nLightSourceSamples) {
        return;
    }
    LightSample lightSample = readLightSample(lightSamples, id);
    float2 intersectionPoint = readIntersectionPoint(intersectionPoints, id);
    float tStart = intersectionPoint.x; float tEnd = intersectionPoint.y;
    bool scatterEvent = tStart < tEnd;
    float3 pos = lightSample.origin + tStart*lightSample.direction;
    uint3 hashGridPos = convert_uint3(pos*(cellSize));
    uint index = hashGridPos.z*nBlocks.x*nBlocks.y + hashGridPos.y*nBlocks.x + hashGridPos.x;
    //uint index = hashGridPos.z + hashGridPos.y + hashGridPos.x;
    //index = ((hashGridPos.x * 73856093) ^ (hashGridPos.y * 19349663) ^ (hashGridPos.z * 83492791)) % (nBlocks.x*nBlocks.y*nBlocks.z);
    whichBucket[outOffset + get_global_id(0)] = index;
    //whichBucket[outOffset + get_global_id(0)] = id;
}


 
#endif
