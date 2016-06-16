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

#ifndef IVW_LIGHT_SAMPLE_CL
#define IVW_LIGHT_SAMPLE_CL

#include "datastructures/ray.cl"
#include "transformations.cl" 

// Note that largest variables should be placed first 
// in order to ensure struct size
typedef struct LightSample {
    //Ray ray;
    float3 origin;
    float3 direction; // Normalized direction
    float3 power;
} LightSample;

// Storage requirements: float8
//typedef struct StoredLightSample {
//    //Ray ray;
//    float origin[3];
//    float power[3];
//    float2 encodedDirection; // Normalized direction, see transformations.cl
//
//} StoredLightSample;

//#define STORE_INTERSECTION_POINT_AS_HALF
#ifdef STORE_INTERSECTION_POINT_AS_HALF
#define StoredIntersectionPoint half
#else
#define StoredIntersectionPoint float2
#endif



/*
 * Define the data type that will be stored and read.
 * Storing half data type is slower than float (Nvidia 670).
 */

//#define STORE_LIGHT_SAMPLE_AS_HALF
#ifdef STORE_LIGHT_SAMPLE_AS_HALF
#define StoredLightSample half
#else
#define StoredLightSample float8 
#endif

// Write light sample into buffer
inline void writeLightSample(LightSample lightSample, __global StoredLightSample* data, int index) {
    
#ifdef STORE_LIGHT_SAMPLE_AS_HALF
    // Slower... Need to get down to 8 float values to increase performance
    vstore_half8((float8)(lightSample.origin.xyz, lightSample.power.xyz, encodeDirection(lightSample.direction)), index, data);
#else 
    data[index] = (float8)(lightSample.origin.xyz, lightSample.power.xyz, encodeDirection(lightSample.direction)); 
    //data[index] = lightSample;
#endif
}

// Read light sample from buffer
inline LightSample readLightSample(__global StoredLightSample const * __restrict data, int index) {
#ifdef STORE_LIGHT_SAMPLE_AS_HALF
    float8 storedData = vload_half8(index, data);
#else 
    float8 storedData = data[index];
#endif
    LightSample lightSample; 
    lightSample.origin = storedData.s012;
    lightSample.power = storedData.s345;
    lightSample.direction = decodeDirection(storedData.s67);
    return lightSample;
}

// Read light sample from buffer
inline float3 readLightSamplePos(__global StoredLightSample const * __restrict data, int index) {
#ifdef STORE_LIGHT_SAMPLE_AS_HALF
    return vload_half4(index*2, data).xyz;
#else 
    return data[index].xyz;
#endif
}

inline float2 readIntersectionPoint(__global StoredIntersectionPoint const * __restrict data, int index) {
#ifdef STORE_INTERSECTION_POINT_AS_HALF
    return vload_half2(index, data);
#else 
    return data[index];
#endif
}

inline void writeIntersectionPoint(float2 tStartAndEnd, __global StoredIntersectionPoint* data, int index) {
#ifdef STORE_INTERSECTION_POINT_AS_HALF
    vstore_half2(tStartAndEnd, index, data);
#else 
    data[index] = tStartAndEnd;
#endif
}


#endif // IVW_LIGHT_SAMPLE_CL
