/*********************************************************************************
 *
 * Inviwo - Interactive Visualization Workshop
 *
 * Copyright (c) 2013-2016 Inviwo Foundation
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

// http://www.doc.ic.ac.uk/~dt10/research/rngs-gpu-mwc64x.html
/*
    Part of MWC64X by David Thomas, dt10@imperial.ac.uk
    This is provided under BSD, full license is with the main package.
    Edits by Daniel Jönsson (daniel.jonsson@liu.se)
*/
#include "random.cl"

// No stream splitting, seed is not shared between threads
__kernel void MWC64X_GenerateRandomState(__global uint* seeds, int size) {
    if (get_global_id(0) >= size) {
        return;
    }
    random_state randState;
    MWC64X_SeedStreams(&randState, seeds[2*get_global_id(0)], 1099511627776UL);
    seeds[2*get_global_id(0)] = randState.x;
    seeds[2*get_global_id(0)+1] = randState.c;
}

// Seeds are shared among threads and each thread can take maxSamplesPerStream random numbers 
// from the state
// To use it you need to choose the maximum possible samples you will take from a single stream
__kernel void MWC64X_GeneratePerStreamRandomState(__global uint* seeds, ulong maxSamplesPerStream, int size) {
    if (get_global_id(0) >= size) {
        return;
    }
    random_state randState;
    MWC64X_SeedStreams(&randState, seeds[2*get_global_id(0)], maxSamplesPerStream);
    seeds[2*get_global_id(0)] = randState.x;
    seeds[2*get_global_id(0)+1] = randState.c;
}