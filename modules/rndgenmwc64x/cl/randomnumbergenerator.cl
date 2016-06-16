/*********************************************************************************
 *
 * Inviwo - Interactive Visualization Workshop
 *
 * Copyright (c) 2014-2016 Inviwo Foundation
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

#include "random.cl"

#define N_NUMBERS_PER_THREAD 1

__kernel void randomNumberGeneratorKernel(
     __global RANDOM_SEED_TYPE* randomSeeds
    , int size
    , __global float* generatedNumbers
    ) 
{ 
    if (get_global_id(0) >= size) {
        return;
    }
    random_state randstate; 
    loadRandState(randomSeeds, get_global_id(0), &randstate);
    for (int i = 0; i < N_NUMBERS_PER_THREAD; ++i) {
        generatedNumbers[get_global_id(0)*N_NUMBERS_PER_THREAD+i] = random_01(&randstate);
    }
    saveRandState(randomSeeds, get_global_id(0), &randstate);
}

__kernel void randomNumberGenerator2DKernel(
     __global RANDOM_SEED_TYPE* randomSeeds
    , int2 size
    , write_only image2d_t generatedNumbers
    ) 
{ 
    if (get_global_id(0) >= size.x*size.y) {
        return;
    }
    random_state randstate; 
    loadRandState(randomSeeds, get_global_id(0), &randstate);
    for (int i = 0; i < N_NUMBERS_PER_THREAD; ++i) {
        //generatedNumbers[get_global_id(0)*N_NUMBERS_PER_THREAD+i] = random_01(&randstate);
        int idx = get_global_id(0)*N_NUMBERS_PER_THREAD+i;
        int2 coord = (int2)(idx % size.x, idx / size.x);
        write_imagef(generatedNumbers, coord, random_01(&randstate));
    }
    saveRandState(randomSeeds, get_global_id(0), &randstate);
    

}