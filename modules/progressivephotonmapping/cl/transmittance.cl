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

#ifndef TRANSMITTANCE_CL
#define TRANSMITTANCE_CL

#include "random.cl"
#include "samplers.cl" 

 
__constant float SAMPLING_BASE_INTERVAL_RCP = 150.f;

// Compute transmittance through volume
float transmittance(read_only image3d_t volumeTex, __constant VolumeParameters* volumeParams,
                 read_only image2d_t tfData,
                 const float3 origin, 
                 const float3 direction, const float tStart, const float tEnd, const float stepSize, random_state* randstate) {
    
    float t = tStart+random_01(randstate)*stepSize;
    float accumulatedOpacity = 0;
    float opticalThickness = 0;
    float extinction = 0.f;

    while(t <= tEnd) {
        float3 pos = origin+t*direction;
        float volumeSample = getNormalizedVoxel(volumeTex, volumeParams, as_float4(pos)).x;
        extinction = read_imagef(tfData, smpNormClampEdgeLinear, (float2)(volumeSample,0.5f)).w;
        opticalThickness += extinction;   
        t += stepSize;
    }
    // Remove the exessive extinction: (tEnd-t)*extinction
    return native_exp(-(opticalThickness*stepSize-(tEnd-t)*extinction)*SAMPLING_BASE_INTERVAL_RCP);
}
//float transmittance(float4 meanFreePath, float t) {
//    float4 survived = step(t, meanFreePath);
//    return (survived.x+survived.y+survived.z+survived.w)/4.f;
//}

// Finds location where attenuation is equal to random number
// Stores location in t
float findAttenuation(read_only image3d_t volumeTex, __constant VolumeParameters* volumeParams,
                 read_only image2d_t tfData,
                 const float3 origin, 
                 const float3 direction, const float tStart, const float tEnd, const float stepSize, random_state* randstate) {
    
    float t = tStart+random_01(randstate)*stepSize;
    float accumulatedOpacity = 0;
    float opacity = 0;
    // Dividing by step size and sample base interval before means that extinction does not need to
    // be multiplied with it during the loop
    float nextInteractionTransittance = -native_log(random_01(randstate))/(stepSize*SAMPLING_BASE_INTERVAL_RCP);
    //for(t < tEnd; accumulatedOpacity < nextInteractionTransittance; t+=stepSize) {
    while(t <= tEnd && accumulatedOpacity < nextInteractionTransittance) {
        float3 pos = origin+t*direction;
        float volumeSample = getNormalizedVoxel(volumeTex, volumeParams, as_float4(pos)).x;
        
        opacity = read_imagef(tfData, smpNormClampEdgeLinear, (float2)(volumeSample,0.5f)).w;    
        accumulatedOpacity += opacity;

        
        t += stepSize;
    }
    // Move back correcsponding to the amount we moved forward too much
    //if(opacity>0.f) {
        t -= ((accumulatedOpacity-nextInteractionTransittance)/opacity)*stepSize;
    //}
    return t;
}
float findAttenuation2(read_only image3d_t volumeTex, 
                 read_only image2d_t tfData,
                 const float3 origin, 
                 const float3 direction, const float tStart, const float tEnd, const float stepSize, float nextInteractionTransittance, float *accumulatedOpacity) {
    
    float t = tStart;
    float opacity = 0;
    //for(t < tEnd; accumulatedOpacity < nextInteractionTransittance; t+=stepSize) {
    while(t < tEnd && *accumulatedOpacity < nextInteractionTransittance) {
        float3 pos = origin+t*direction;
        float volumeSample = getVoxel(volumeTex, as_float4(pos));
        
        opacity = read_imagef(tfData, smpNormClampEdgeLinear, (float2)(volumeSample,0.5f)).w;    
        *accumulatedOpacity += opacity;

        
        t += stepSize;
    }
    // Move back correcsponding to the amount we moved forward to much
    if( t < tEnd ) {
        float adjust = ((*accumulatedOpacity-nextInteractionTransittance)/opacity);
        t -= adjust*stepSize;
        *accumulatedOpacity -=adjust;
    }
    return t;
}

// Apply Woodcock tracking to find the sample distance along the ray
float woodcockTracking(read_only image3d_t volumeTex, __constant VolumeParameters* volumeParams,
                 read_only image2d_t tfData, const float3 origin, 
                 const float3 direction, const float tStart, const float tEnd, float tauMax, random_state* __restrict randstate) {
    
    float invTauMaxSampleBaseInterval = 1.f/(tauMax*SAMPLING_BASE_INTERVAL_RCP);
    float invTauMax = 1.f/(tauMax);
    float t = tStart;
    float opacity;
    do {
        t += -native_log(random_01(randstate))*invTauMaxSampleBaseInterval;
        float3 pos = origin+t*direction;
        float volumeSample = getNormalizedVoxel(volumeTex, volumeParams, as_float4(pos)).x;
        //volumeSample = toDataSpace1d2(1.f/convert_float(get_image_width(tfData)), volumeSample);
        opacity = read_imagef(tfData, smpNormClampEdgeLinear, (float2)(volumeSample,0.5f)).w; 
    } while( random_01(randstate) >= opacity*invTauMax && t <= tEnd);
    return t;
    //return min(t, tEnd);

}
float woodcockTrackingPhoton(read_only image3d_t volumeTex, __constant VolumeParameters* volumeParams,
                 read_only image2d_t tfData, const float3 origin, 
                 const float3 direction, const float tStart, const float tEnd, float tauMax, random_state* __restrict randstate, float* rnd) {
    
    float invTauMaxSampleBaseInterval = 1.f/(tauMax*SAMPLING_BASE_INTERVAL_RCP);
    float invTauMax = 1.f/(tauMax);
    float t = tStart;
    float opacity;
    do {
        t += -native_log(random_01(randstate))*invTauMaxSampleBaseInterval;
        float3 pos = origin+t*direction;
        float volumeSample = getNormalizedVoxel(volumeTex, volumeParams, as_float4(pos)).x;
        opacity = read_imagef(tfData, smpNormClampEdgeLinear, (float2)(volumeSample,0.5f)).w; 
        *rnd = random_01(randstate);
    } while( *rnd >= (opacity)*invTauMax && t <= tEnd);

    return t;

}
float woodcockTrackingN(read_only image3d_t volumeTex, __constant VolumeParameters* volumeParams, read_only image2d_t tfData, const float3 origin,
                 const float3 direction, const float tStart, const float tEnd, float tauMax, random_state* __restrict randstate, float opacity2) {
    
    float invTauMaxSampleBaseInterval = 1.f/(tauMax*SAMPLING_BASE_INTERVAL_RCP);
    float invTauMax = 1.f/(tauMax);
    float t = tStart;
    float opacity;
    do {
        t += -native_log(random_01(randstate))*invTauMaxSampleBaseInterval;
        float3 pos = origin+t*direction;
        float volumeSample = getNormalizedVoxel(volumeTex, volumeParams, as_float4(pos)).x;
        float tfOpacity = read_imagef(tfData, smpNormClampEdgeLinear, (float2)(volumeSample,0.5f)).w;
        opacity = tfOpacity == 0 ? tfOpacity : opacity2;
    } while( random_01(randstate) >= opacity*invTauMax && t <= tEnd);

    return t;

}
// Apply Woodcock tracking to find the sample distance along the ray
// Check for first non-zero location
float woodcockTrackingCheckNonZero(read_only image3d_t volumeTex, __constant VolumeParameters* volumeParams,
                 read_only image2d_t tfData, const float3 origin, 
                 const float3 direction, const float tStart, const float tEnd, float *tFirstNonZero, float tauMax, random_state* randstate) {
    
    float invTauMaxSampleBaseInterval = 1.f/(tauMax*SAMPLING_BASE_INTERVAL_RCP);
    float invTauMax = 1.f/(tauMax);
    float t = tStart;
    bool checkNonZero = true;
    float opacity;
    do {
        t += -native_log(random_01(randstate))*invTauMaxSampleBaseInterval;
        float3 pos = origin+t*direction;
        float volumeSample = getNormalizedVoxel(volumeTex, volumeParams, as_float4(pos)).x;
        opacity = read_imagef(tfData, smpNormClampEdgeLinear, (float2)(volumeSample,0.5f)).w; 
        if (t < *tFirstNonZero && opacity == 0.f) {
            *tFirstNonZero = t;
        }
        //if (checkNonZero == true && opacity == 0.f) {
        //    //if (t < *tFirstNonZero)
        //        *tFirstNonZero = t;
        //} else {
        //    checkNonZero = false;
        //}

    } while( random_01(randstate) >= opacity*invTauMax && t <= tEnd);

    return t;

}
// Apply Woodcock tracking to find the sample distance along the ray
float4 woodcockTracking4(read_only image3d_t volumeTex, __constant VolumeParameters* volumeParams,
                 read_only image2d_t tfData, const float3 origin, 
                 const float3 direction, const float tStart, const float tEnd, float tauMax, random_state* randstate) {
    
    float invTauMaxSampleBaseInterval = 1.f/(tauMax*SAMPLING_BASE_INTERVAL_RCP);
    float invTauMax = 1.f/(tauMax);
    float4 t = tStart;
    float4 opacity;
    float4 done = 1.f;
    do {
        float4 rnd = (float4)(random_01(randstate),random_01(randstate),random_01(randstate),random_01(randstate));
        t += -done*native_log(rnd)*invTauMaxSampleBaseInterval;
        float3 p1 = origin+t.x*direction;
        float3 p2 = origin+t.y*direction;
        float3 p3 = origin+t.z*direction;
        float3 p4 = origin+t.w*direction;
        float4 volumeSample;
        volumeSample.x = getNormalizedVoxel(volumeTex, volumeParams, as_float4(p1)).x;
        volumeSample.y = getNormalizedVoxel(volumeTex, volumeParams, as_float4(p2)).x;
        volumeSample.z = getNormalizedVoxel(volumeTex, volumeParams, as_float4(p3)).x;
        volumeSample.w = getNormalizedVoxel(volumeTex, volumeParams, as_float4(p4)).x;

        opacity.x = read_imagef(tfData, smpNormClampEdgeLinear, (float2)(volumeSample.x,0.5f)).w; 
        opacity.y = read_imagef(tfData, smpNormClampEdgeLinear, (float2)(volumeSample.y,0.5f)).w; 
        opacity.z = read_imagef(tfData, smpNormClampEdgeLinear, (float2)(volumeSample.z,0.5f)).w; 
        opacity.w = read_imagef(tfData, smpNormClampEdgeLinear, (float2)(volumeSample.w,0.5f)).w; 
        float4 rnd2 = (float4)(random_01(randstate),random_01(randstate),random_01(randstate),random_01(randstate));
        float4 done2 = step(opacity*invTauMax, rnd2);
        float4 done3 = step(t, tEnd);
        done = min(done, min(done3, done2));
    } while( (done.x+done.y+done.z+done.w) > 0.f);

    return t;

}




float homogeneousAbsorption(float absorption, float dist) {
    return native_exp(-absorption*dist);
}

float homogeneousTransmittance(float extinction, float dist) {
    return native_exp(-extinction*dist);
}
// Find distance for which transmittance = random value
float homogeneousFindDistance(float extinction, random_state* randstate) {
    float rnd = random_01(randstate);
    return -native_log(random_01(randstate))/extinction;
}


float skipEmptySpace(read_only image3d_t volumeTex, 
                 read_only image2d_t tfData, const float3 origin, 
                 const float3 direction, const float tStart, const float tEnd, float stepSize) {
    float t = tStart;
    while(t < tEnd) {
        float3 pos = origin+t*direction;
        float volumeSample = getVoxel(volumeTex, as_float4(pos));
        float opacity = read_imagef(tfData, smpNormClampEdgeLinear, (float2)(volumeSample,0.5f)).w; 
        if(opacity>0.f) {
            break;
        }
        t += stepSize;
    }

    return t;

}


#endif