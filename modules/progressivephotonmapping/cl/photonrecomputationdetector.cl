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

#include "intersection/rayboxintersection.cl" 
#include "light/light.cl" 
#include "datastructures/lightsample.cl"
#include "samplers.cl" 
#include "photon.cl"
#include "transformations.cl" 

#include "uniformgrid/uniformgrid.cl"  

/*
* Find first grid point hit point calculated in index coordinates [0 dim-1]
* using maximum value within each grid cell.
*
* @param x1 Start point in index coordinates
* @param x2 End point in index coordinates
* @param cellDim Dimension of a grid cell
* @param volume Data containing maximum data value within each grid cell
* @param volumeMaxSize Size of volume
* @param dataMinMaxValueThreshold All grids with value below min and above max value will be culled
* @param tHit Output the distance to the hitpoint along the direction from x1
* @return True if a hit is found, false otherwise
*/
float uniformGridImportance(float3 x1, float3 x2
    , float3 cellDim 
    , __global const float* uniformGrid3D
    , int4 uniformGridDimensions, float* tHit) {
    int3 cellCoord, cellCoordEnd, di;
    float3 dt, deltatx;
    setupUniformGridTraversal(x1, x2, cellDim, uniformGridDimensions.xyz
        , &cellCoord, &cellCoordEnd, &di, &dt, &deltatx);
    *tHit = 0.f;

    // Visit all cells
    bool continueTraversal = true;
    //if(get_global_id(0) == 128 && get_global_id(1) == 128) {
    //    printf("CellcoordEnd == %v3i\n", cellCoordEnd);
    //    printf("x1 == %v3f\n", x1);
    //    printf("x2 == %v3f\n", x2);

    //}
    float importance = 0.f;
    int iteration = 0;
    float dt1 = 0.f;
    while (continueTraversal) {
        // Visit cell     
        //if(get_global_id(0) == 128 && get_global_id(1) == 128) {
        //    printf("%v3i\n", cellCoord);
        //}

        float val = uniformGrid3D[cellCoord.x + cellCoord.y*uniformGridDimensions.x + cellCoord.z * uniformGridDimensions.x*uniformGridDimensions.y];
     
        float dt0 = dt1; 
        continueTraversal = stepToNextCellNextHit(deltatx, di, cellCoordEnd, &dt, &cellCoord, &dt1);
        importance += val*(min(1.f, dt1) - dt0);
    }  
    float len = length((x2 - x1));
    return importance*len;
}
  
__kernel void photonRecomputationDetectorKernel(
    // uniform grid parameters
      __global const float* uniformGrid3D
    , int4 uniformGridDimensions
    , float3 cellSize
    , float16 textureToIndexMat
    , float16 indexToTextureMat
    , __global PHOTON_DATA_TYPE* photonDataInput
    , int photonOffset
    , __global StoredLightSample const* __restrict lightSamples
    , __global StoredIntersectionPoint const* __restrict intersectionPoints // tStart, tEnd for each light sample  
    , int nLightSamples
    , uint maxInteractions
    , int totalPhotons
    , __global unsigned int* recomputationImportances
    )
{
    int threadId = get_global_id(0);
    if (threadId >= nLightSamples) {
        return;
    }       
    int interaction = 0;
    float recomputationImportance = 0;
    LightSample lightSample = readLightSample(lightSamples, threadId);
    float2 intersectionPoint = readIntersectionPoint(intersectionPoints, threadId);
    float tStart = intersectionPoint.x; float tEnd = intersectionPoint.y;
    bool scatterEvent = tStart < tEnd;
    if (scatterEvent) {
        float3 entry = lightSample.origin + tStart*lightSample.direction;
        for (; interaction < maxInteractions; ++interaction) {
            uint photonId = photonOffset + interaction*totalPhotons + threadId;

            float8 photon = readPhoton(photonDataInput, photonId);
            float3 exit = photon.xyz;
            if (any(photon.xyz == (float3)(FLT_MAX))) {
                if (interaction == 0) {
                    exit = tEnd*lightSample.direction;
                } else if (any(entry == (float3)(FLT_MAX))) {
                    // No previous interaction
                    break;
                } else {
                    BBox volumeBBox; volumeBBox.pMin = (float3)(0.f); volumeBBox.pMax = (float3)(1.f);
                    tStart = 0.f; tEnd = FLT_MAX;
                    float3 photonDirection = decodeDirection(photon.s67);
                    if (photon.w != FLT_MAX && rayBoxIntersection(volumeBBox, entry, photonDirection, &tStart, &tEnd)) {
                        exit += photonDirection*tEnd;
                    } else {
                        break;
                    }
                    
                }
            }
            float3 x1 = transformPoint(textureToIndexMat, entry.xyz) + 0.5f;
            float3 x2 = transformPoint(textureToIndexMat, exit.xyz) + 0.5f;

            float t0;
            recomputationImportance += uniformGridImportance(x1, x2, cellSize, uniformGrid3D, uniformGridDimensions, &t0);
            entry = photon.xyz;
        }
    } 
    // Recomputation importance must have been initialized to 2147483647u 
    // The importance must be reset when the photon has been updated.
    // Multiply by 100 round to positive (rtp) to make sure that we take small
    // rays of 1 voxel into account.
    recomputationImportances[photonOffset + threadId] -= clamp(0u, 2147483647u, convert_uint_sat_rtp(100*recomputationImportance));
}

// Update a certain percentage of photons with equal importance
__kernel void photonRecomputationDetectorEqualImportanceKernel(
    // uniform grid parameters
    __global const float* uniformGrid3D
    , int4 uniformGridDimensions
    , float3 cellSize
    , float16 textureToIndexMat
    , float16 indexToTextureMat
    , __global PHOTON_DATA_TYPE* photonDataInput
    , int photonOffset
    , __global StoredLightSample const* __restrict lightSamples
    , __global StoredIntersectionPoint const* __restrict intersectionPoints // tStart, tEnd for each light sample  
    , int nLightSamples
    , uint maxInteractions
    , int totalPhotons
    , __global unsigned int* recomputationImportances
    , int percentage
    , int iteration
    )
{
    int threadId = get_global_id(0);
    if (threadId >= nLightSamples) {
        return;
    }
    int interaction = 0;
    float recomputationImportance = 0;
    int photonId = photonOffset + threadId;
    if ((photonId + iteration) % (100 / percentage) == 0) {
        recomputationImportance = 1.f;
    }
    // Recomputation importance must have been initialized to 2147483647u 
    // The importance must be reset when the photon has been updated.
    // Multiply by 100 round to positive (rtp) to make sure that we take small
    // rays of 1 voxel into account.
    recomputationImportances[photonOffset + threadId] -= clamp(0u, 2147483647u, convert_uint_sat_rtp(100 * recomputationImportance));
}