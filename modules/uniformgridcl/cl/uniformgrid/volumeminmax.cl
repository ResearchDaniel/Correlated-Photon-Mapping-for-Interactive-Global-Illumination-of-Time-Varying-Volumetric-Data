/*********************************************************************************
 *
 * Inviwo - Interactive Visualization Workshop
 *
 * Copyright (c) 2016 Daniel Jönsson
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
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
#include "image3d_write.cl" 

__kernel void volumeMinMaxKernel(read_only image3d_t volumeIn, __constant VolumeParameters* volumeParams
    , image_3d_write_vec2_uint16_t volumeOut
    , int4 outDim
    , int4 region
    )
{
    // output coordinates 
    int3 globalId = (int3)(get_global_id(0), get_global_id(1), get_global_id(2));  

    
    if (any(globalId>=outDim.xyz)) {
        return;
    }
    float2 minMaxVal = (float2)(FLT_MAX, 0);
    int4 startCoord = (int4)(globalId*region.xyz, 0);
    int4 endCoord = min(startCoord+region, get_image_dim(volumeIn));
    for (int z = startCoord.z; z < endCoord.z; ++z) {
        for (int y = startCoord.y; y < endCoord.y; ++y) {
            for (int x = startCoord.x; x < endCoord.x; ++x) {
                float value = getNormalizedVoxelUnorm(volumeIn, volumeParams, (int4)(x, y, z, 0)).x;
                minMaxVal.x = min(minMaxVal.x, value);
                minMaxVal.y = max(minMaxVal.y, value);
            }
        }
    }

    writeImageVec2UInt16f(volumeOut, as_int4(globalId), outDim, minMaxVal);

}