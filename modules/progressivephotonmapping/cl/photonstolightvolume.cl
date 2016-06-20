

#include "samplers.cl" 
#include "transformations.cl"
#include "densityestimationkernel.cl"
#include "photon.cl"
#include "shading/shading.cl" 
#include "image3d_write.cl" 
//#define VOLUME_OUTPUT_HALF_TYPE

#ifdef SUPPORTS_VOLUME_WRITE
#pragma OPENCL_EXTENSION cl_khr_3d_image_writes : enable
#endif

void atomic_add_float_global(volatile global float *source, const float operand) {
    union {
        unsigned int intVal;
        float floatVal;
    } newVal;
    union {
        unsigned int intVal;
        float floatVal;
    } prevVal;

    do {
        prevVal.floatVal = *source;
        newVal.floatVal = prevVal.floatVal + operand;
    } while (atomic_cmpxchg((volatile global unsigned int *)source, prevVal.intVal, newVal.intVal) != prevVal.intVal);
}

void splatPhoton(
#ifdef VOLUME_OUTPUT_SINGLE_CHANNEL
    __global float* volumeOut
#else
    __global float4* volumeOut
#endif
    , __constant VolumeParameters* volumeOutParams
    , int4 outDim
    , float8 photonData
    , float photonRadius
    ) {
    if (any(photonData.xyz == (float3)(FLT_MAX))) {
        return;
    }
    float3 radiusInTextureSpace = (float3)(photonRadius);
    int3 startCoord = max((int3)(0), convert_int3(transformPoint(volumeOutParams[0].textureToIndex, photonData.xyz - radiusInTextureSpace)));
    int3 endCoord = min(convert_int3(transformPoint(volumeOutParams[0].textureToIndex, photonData.xyz + radiusInTextureSpace) + 1.f), outDim.xyz);

    //float diag = length(transformPoint(volumeOutParams[0].indexToTexture, convert_float3(endCoord - startCoord)));
    //int3 endCoord = min(startCoord + 1, outDim.xyz);
    volatile global float* outVolData = (volatile global float *)volumeOut;
    for (int z = startCoord.z; z < endCoord.z; ++z) {
        for (int y = startCoord.y; y < endCoord.y; ++y) {
            for (int x = startCoord.x; x < endCoord.x; ++x) {
                int voxelIndex = x + y*outDim.x + z*outDim.x*outDim.y;

                float3 volTexCoord = transformPoint(volumeOutParams->indexToTexture, (float3)(x, y, z));

                float distanceToPhoton = distance(volTexCoord, photonData.xyz);
                float weight = densityEstimationKernel(distanceToPhoton / photonRadius);

                float3 filteredIrradiance = photonData.s345 * weight;
#ifdef VOLUME_OUTPUT_SINGLE_CHANNEL
                if (filteredIrradiance.x != 0.f) {
                    atomic_add_float_global(&outVolData[voxelIndex], filteredIrradiance.x);
                }

#else 
                if (filteredIrradiance.x != 0.f)
                    atomic_add_float_global(&outVolData[voxelIndex * 4], filteredIrradiance.x);
                if (filteredIrradiance.y != 0.f)
                    atomic_add_float_global(&outVolData[voxelIndex * 4 + 1], filteredIrradiance.y);
                if (filteredIrradiance.z != 0.f)
                    atomic_add_float_global(&outVolData[voxelIndex * 4 + 2], filteredIrradiance.z);
#endif
            }
        }
    }
}

__kernel void photonsToLightVolumeKernel(read_only image3d_t volumeIn, __constant VolumeParameters* volumeParams
#if VOLUME_OUTPUT_HALF_TYPE && VOLUME_OUTPUT_SINGLE_CHANNEL
    , image_3d_write_float16_t volumeOut
#elif VOLUME_OUTPUT_SINGLE_CHANNEL
    , image_3d_write_float32_t volumeOut
#elif VOLUME_OUTPUT_HALF_TYPE
    , image_3d_write_vec4_float16_t volumeOut
#else
    , image_3d_write_vec4_float32_t volumeOut
#endif 
    , __constant VolumeParameters* volumeOutParams
    , int4 outDim
    , __global PHOTON_DATA_TYPE* photonDataArray
    , int totalPhotons
    , float photonRadius
    , float relativeIrradianceScale
    ) 
{
    int3 globalId = (int3)(get_global_id(0), get_global_id(1), get_global_id(2));  

    
    if (any(globalId>=outDim.xyz)) {
        return;
    }
    float4 irradiance = (float4)(0.0f, 0.f, 0.f, 1.f);
    int4 startCoord = (int4)(globalId, 0);
    int4 endCoord = min(startCoord, get_image_dim(volumeIn));
    float3 volTexCoord = transformPoint(volumeOutParams[0].textureToWorld, transformPoint(volumeOutParams[0].indexToTexture, convert_float3(globalId)));
    int batchOffset = 0;
    for (uint i = 0; i < totalPhotons; ++i) {

        uint photonId = batchOffset + i;
        float8 photonData = readPhoton(photonDataArray, photonId);
        
        float3 entry = transformPoint(volumeParams[0].textureToWorld, photonData.xyz);
        
        if (distance(entry, volTexCoord) < photonRadius) {
            irradiance.xyz += photonData.s345;
        }
    }
    // In case we do not have a photon shader in between
#ifndef PER_PHOTON_SHADING
    irradiance *= isotropicPhaseFunction()*relativeIrradianceScale;
#endif
#if VOLUME_OUTPUT_HALF_TYPE && VOLUME_OUTPUT_SINGLE_CHANNEL
    writeImageFloat16f(volumeOut, (int4)(globalId, 0), outDim, max(irradiance.x, max(irradiance.y, irradiance.z)));
#elif VOLUME_OUTPUT_SINGLE_CHANNEL
    writeImageFloat32f(volumeOut, (int4)(globalId, 0), outDim, max(irradiance.x, max(irradiance.y, irradiance.z)));
#elif VOLUME_OUTPUT_HALF_TYPE
    writeImageVec4Float16f(volumeOut, (int4)(globalId, 0), outDim, irradiance);
#else
    writeImageVec4Float32f(volumeOut, (int4)(globalId, 0), outDim, irradiance);
#endif 
}




__kernel void splatPhotonsToLightVolumeKernel(read_only image3d_t volumeIn, __constant VolumeParameters* volumeParams
#ifdef VOLUME_OUTPUT_SINGLE_CHANNEL
    , __global float* volumeOut
#else
    , __global float4* volumeOut
#endif
    
    , __constant VolumeParameters* volumeOutParams
    , int4 outDim
    , __global PHOTON_DATA_TYPE* photonDataArray
    , int totalPhotons
    , float photonRadius
    , float relativeIrradianceScale // Scale power to be similar independent of number of photons and photon radius.
    )
{
    int photonId = get_global_id(0);

    if (any(photonId >= totalPhotons)) {
        return;
    }
    int batchOffset = 0;
    float8 photonData = readPhoton(photonDataArray, photonId);
    // In case we do not have a photon shader in between
#ifndef PER_PHOTON_SHADING
    photonData.s345 *= isotropicPhaseFunction()*relativeIrradianceScale;
#endif
    splatPhoton(volumeOut, volumeOutParams, outDim, photonData, photonRadius);
}

__kernel void splatSelectedPhotonsToLightVolumeKernel(
#ifdef VOLUME_OUTPUT_SINGLE_CHANNEL
      __global float* volumeOut
#else
      __global float4* volumeOut
#endif
    , __constant VolumeParameters* volumeOutParams
    , int4 outDim
    //, __global uint* nPhotonInteractions
    , __global PHOTON_DATA_TYPE* photonDataArray
    , __global int* photonIndices // Indices to photons
    , int nIndices
    , float photonRadius
    , float relativeIrradianceScale // Scale power to be similar independent of number of photons and photon radius.
    , float photonRadianceMultiplier // +1 if contribution should be added, -1 otherwise
    , int nPhotons // Number of photons per scattering event
    , int nInteractions // Maximum number of scattering events
    )
{

    if (get_global_id(0) >= nIndices) {
        return;
    }
    int photonId = photonIndices[get_global_id(0)];
    for (int interaction = 0; interaction < nInteractions; ++interaction) {
        int photonOffset = interaction * nPhotons;
        float8 photonData = readPhoton(photonDataArray, photonOffset + photonId);
        // In case we do not have a photon shader in between
    #ifndef PER_PHOTON_SHADING
        photonData.s345 *= isotropicPhaseFunction()*relativeIrradianceScale;
    #endif
        photonData.s345 *= photonRadianceMultiplier;
        splatPhoton(volumeOut, volumeOutParams, outDim, photonData, photonRadius);
    }
}

__kernel void clearFloat4Kernel(__global float4* memory, int size) {
    if (get_global_id(0) < size) {
        memory[get_global_id(0)] = (float4)(0, 0, 0, 1.f);
    }
    
}

__kernel void clearFloatKernel(__global float* memory, int size) {
    if (get_global_id(0) < size) {
        memory[get_global_id(0)] = 0;
    }
}

__kernel void photonDensityNormalizationKernel(__global float4* memory, int size) {
    if (get_global_id(0) < size) {
        float4 photonIrradiance = memory[get_global_id(0)];
        if (photonIrradiance.w > 0)
            memory[get_global_id(0)] = photonIrradiance / (photonIrradiance.w);
    }
}

__kernel void copyIndexPhotonsKernel(
      __global PHOTON_DATA_TYPE* photonDataArray
    , __global int* photonIndices // Indices to photons
    , int nIndices
    , float photonRadianceMultiplier // +1 if contribution should be added, -1 otherwise
    , int nPhotons // Number of photons per scattering event
    , int nInteractions // Maximum number of scattering events
    , __global PHOTON_DATA_TYPE* alignedPhotons
    , int outOffset
    )
{

    if (get_global_id(0) >= nIndices) {
        return;
    }
    int photonId = photonIndices[get_global_id(0)];
    //int photonId = get_global_id(0);
    for (int interaction = 0; interaction < nInteractions; ++interaction) {
        int photonOffset = interaction * nPhotons;
        float8 photonData = readPhoton(photonDataArray, photonOffset + photonId);
        photonData.s345 *= photonRadianceMultiplier;
        writePhoton(photonData, alignedPhotons, outOffset + get_global_id(0) + interaction*nIndices);
    }
}