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
#include "random.cl"
#include "shading/shading.cl" 
#include "light/light.cl" 
#include "datastructures/lightsample.cl"

//#include "gradients.cl"
#include "samplers.cl" 
#include "photon.cl"
#include "transformations.cl" 
#include "transmittance.cl"  

//__constant float SAMPLING_BASE_INTERVAL_RCP = 200.0; 
__constant float RUSSIAN_ROULETTE_P = 0.9f;  

//#define NO_SINGLE_SCATTERING   
    
bool nextInteraction(read_only image3d_t volumeTex, __constant VolumeParameters* volumeParams, float volumeSample, BBox volumeBBox, float4 material,
                 float3 sample, 
                 float3* direction, float* __restrict t0, float* __restrict t1, random_state* randstate, const ShadingType shadingType) {
    float2 rnd = (float2)(random_01(randstate), random_01(randstate));     
    sampleShadingFunction(volumeTex, volumeParams, volumeSample, material, sample, direction, rnd, shadingType);
    
    return rayBoxIntersection(volumeBBox, sample, *direction, t0, t1);

}  
bool nextInteractionPdf(read_only image3d_t volumeTex, __constant VolumeParameters* volumeParams, float volumeSample, BBox volumeBBox, float4 material,
                 float3 sample, 
                 float3* direction, float* __restrict t0, float* __restrict t1, float *pdf, random_state* randstate, const ShadingType shadingType) {
    float2 rnd = (float2)(random_01(randstate), random_01(randstate));     
    sampleShadingFunctionPdf(volumeTex, volumeParams, volumeSample, material, sample, direction, pdf, rnd, shadingType);
    
    return rayBoxIntersection(volumeBBox, sample, *direction, t0, t1);

} 
  
__kernel void photonTracerKernel(
#ifdef PHOTON_RECOMPUTATION
    __global const unsigned int* recomputationPhotonIndex,
    int nPhotonsToRecompute,
#endif
    read_only image3d_t volumeTex
    , __constant VolumeParameters* volumeParams
    , __global  const BBox* volumeBBox
    , read_only image2d_t tfData
    , read_only image2d_t tfScattering
    , float4 material
    , __global RANDOM_SEED_TYPE* randomSeeds
    //, __global const  float2* lightSamples
    , float stepSize
    , __global PHOTON_DATA_TYPE* photonDataArray
    , int iteration
    , int photonOffset
    , int batch
    , __global StoredLightSample const* __restrict lightSamples
    , __global StoredIntersectionPoint const* __restrict intersectionPoints // tStart, tEnd for each light sample  
    , int nLightSamples
    , uint maxInteractions
    , ShadingType shadingType
    , int randomLightSampling
    , int totalPhotons
    )
{
//#define DISPLAY_RECOMPUTED_PHOTONS
#ifdef PHOTON_RECOMPUTATION

    if (get_global_id(0) >= nPhotonsToRecompute) {
        return;
    }
    int threadId = recomputationPhotonIndex[get_global_id(0)] - photonOffset;
    //int threadId = get_global_id(0);
    if (threadId < 0 || threadId >= nLightSamples) {
        return;
    }
    //uchar photonRecomputationImportance = recomputationImportance[photonOffset+threadId];
//#ifndef DISPLAY_RECOMPUTED_PHOTONS
//    if (photonRecomputationImportance == 0) {
//        return;
//    }
//#endif
#else 
    int threadId = get_global_id(0);
    if (threadId >= nLightSamples) {
        return;
    }
#endif
    float3 photonPower; 
    random_state randstate;  
    // Use new random seed for each iteration. 
    //int rndIndex = (threadId+64*(iteration-1))%(get_global_size(0)*get_global_size(1));
    loadRandState(randomSeeds, photonOffset+threadId, &randstate);
    //loadRandState(randomSeeds, 0, &randstate);
    //loadRandState(randomSeeds, rndIndex, &randstate);
    uint nInteractions = 0;
     
    LightSample lightSample = readLightSample(lightSamples, threadId);
    lightSample.power /= convert_float(maxInteractions);
#ifdef PHOTON_RECOMPUTATION 
#ifdef DISPLAY_RECOMPUTED_PHOTONS
    lightSample.power.xyz = (float3)(100000.f);
#endif
#endif
    float2 intersectionPoint = readIntersectionPoint(intersectionPoints, threadId);
    float tStart = intersectionPoint.x; float tEnd = intersectionPoint.y;
    bool scatterEvent = tStart < tEnd;

    //float3 firstHitPoint;
    //if (scatterEvent)  
    //    tStart = skipEmptySpace(volumeTex, tfData, lightSample.origin, lightSample.direction, tStart, tEnd, stepSize);   
    //{float2 dirAngles = encodeDirection(lightSample.direction); 
    #ifdef NO_SINGLE_SCATTERING
        // Only perform multiple scattering
    float t = woodcockTracking(volumeTex, volumeParams, tfData, lightSample.origin, lightSample.direction, tStart, tEnd, 1.f, &randstate);  
        if(scatterEvent) {
            lightSample.origin += t*lightSample.direction;
            tStart = 0.f; tEnd = FLT_MAX; 
            float volumeSample = getVoxel(volumeTex, as_float4(lightSample.origin));
            float pdf;
            scatterEvent = nextInteractionPdf(volumeTex, volumeParams, volumeSample, volumeBBox[0], material, lightSample.origin,  
                &lightSample.direction, &tStart, &tEnd, &pdf, &randstate, shadingType);
            lightSample.power /= pdf;
            // Move a bit to avoid getting stuck in the same material
            tStart+=0.5f*stepSize;
        } 
    #endif
    while(scatterEvent) {  
        // Find next scattering event
        float t = woodcockTracking(volumeTex, volumeParams, tfData, lightSample.origin, lightSample.direction, tStart, tEnd, 1.f, &randstate);

        scatterEvent = t <= tEnd;   
        if(scatterEvent) { 
                    
            lightSample.origin += t*lightSample.direction; 
            uint photonId = photonOffset + nInteractions*totalPhotons + threadId;
            float2 dirAngles = encodeDirection(lightSample.direction);
 
            // Determine which kind of interaction we have
            float volumeSample = getNormalizedVoxel(volumeTex, volumeParams, as_float4(lightSample.origin)).x;
            float4 color = read_imagef(tfData, smpNormClampEdgeLinear, (float2)(volumeSample,0.5f));

            float4 scattering = read_imagef(tfScattering, smpNormClampEdgeLinear, (float2)(volumeSample,0.5f)); 
            float scatteringAlbedo = scattering.w/(scattering.w+color.w);
            // Monte-Carlo: Divide by the probability that the photon ended up here
            lightSample.power /= max(color.w, 0.01f);
            
            
            ++nInteractions;  
            if (nInteractions < maxInteractions && random_01(&randstate) < scatteringAlbedo) { // Photon is scattered 
                lightSample.power *= scatteringAlbedo;
                writePhoton((float8)(lightSample.origin.x, lightSample.origin.y, lightSample.origin.z, lightSample.power.x, lightSample.power.y, lightSample.power.z, dirAngles.x, dirAngles.y), photonDataArray, photonId);
                tStart= 0.f; tEnd = FLT_MAX; 
                scatterEvent = nextInteraction(volumeTex, volumeParams, volumeSample, volumeBBox[0], material, lightSample.origin,
                                            &lightSample.direction, &tStart, &tEnd, &randstate, shadingType);
                // Move a bit to avoid getting stuck in the same material
                tStart+=0.5*stepSize;
            } else {
                writePhoton((float8)(lightSample.origin.x, lightSample.origin.y, lightSample.origin.z, lightSample.power.x, lightSample.power.y, lightSample.power.z, dirAngles.x, dirAngles.y), photonDataArray, photonId);
                // Used in photonrecompuationdetector.cl
                lightSample.power = (float3)(FLT_MAX);
                scatterEvent = false;    
            }

        }  

    }

    float2 dirAngles = encodeDirection(lightSample.direction);
    float8 photon = (float8)(FLT_MAX, FLT_MAX, FLT_MAX, lightSample.power.x, FLT_MAX, FLT_MAX, dirAngles.x, dirAngles.y);
    for( uint i = nInteractions; i < maxInteractions; ++i) {
        uint photonId = photonOffset + (i)*totalPhotons + threadId;
        #ifdef PHOTON_DATA_TYPE_HALF
        vstore_half8(photon, photonId, photonDataArray);
        #else
        photonDataArray[photonId] = photon;
        #endif
         
    } 
    // Ensuring that the same random seed is used reduces noise
#ifdef PROGRESSIVE_PHOTON_MAPPING

    saveRandState(randomSeeds, photonOffset+threadId, &randstate);
    //saveRandState(randomSeeds, rndIndex, &randstate);
#endif
}
