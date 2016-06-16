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

#include "datastructures/lightsample.cl"
#include "intersection/raymeshintersection.cl"

// Compute intersection point along ray for light samples.
__kernel void lightSampleMeshIntersectionKernel(
    __global float const * __restrict vertices
    , __global int const * __restrict indices
    , int nIndices
    , __global StoredLightSample const * __restrict lightSamples
    , int nSamples
    , __global StoredIntersectionPoint* intersectionPoints
    ) 
{ 
    int threadId = get_global_id(0);
    if (threadId >= nSamples) {
        return;
    } 
    LightSample lightSample = readLightSample(lightSamples, threadId);
    float2 intersectionPoint;
    float t0 = 0; float t1 = FLT_MAX;
    bool hit = rayMeshIntersection(vertices, indices, nIndices, lightSample.origin, lightSample.direction, &t0, &t1);
    if (!hit) {
        t0 = 0.f;  t1 = -1.f;
    }

    writeIntersectionPoint((float2)(t0, t1), intersectionPoints, threadId);
}