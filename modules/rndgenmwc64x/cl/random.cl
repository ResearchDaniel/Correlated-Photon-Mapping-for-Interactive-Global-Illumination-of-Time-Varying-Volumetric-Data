/*********************************************************************************
 *
 * Inviwo - Interactive Visualization Workshop
 *
 * Copyright (c) 2013-2015 Inviwo Foundation
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
#ifndef RANDOM_CL
#define RANDOM_CL

/*
    Part of MWC64X by David Thomas, dt10@imperial.ac.uk
    This is provided under BSD, full license is with the main package.
    Edits by Daniel Jönsson (daniel.jonsson@liu.se)
*/
#define RANDOM_SEED_TYPE uint

#include "skip_mwc.cl"

//! Represents the state of a particular generator
typedef struct{ uint x; uint c; } random_state;

enum{ MWC64X_A = 4294883355U };
enum{ MWC64X_M = 18446383549859758079UL };

void saveRandState(global uint* stateBuffer, int index, random_state *state) {
    stateBuffer[2*index] = state->x;
    stateBuffer[2*index+1] = state->c;
}
void loadRandState(global uint* stateBuffer, int index, random_state *state) {
    state->x = stateBuffer[2*index];
    state->c = stateBuffer[2*index+1];
}

void MWC64X_Step(random_state *s)
{
    uint X=s->x, C=s->c;
    
    uint Xn=MWC64X_A*X+C;
    uint carry=(uint)(Xn<C);                // The (Xn<C) will be zero or one for scalar
    uint Cn=mad_hi(MWC64X_A,X,carry);  
    
    s->x=Xn;
    s->c=Cn;
}

void MWC64X_Skip(random_state *s, ulong distance)
{
    uint2 tmp=MWC_SkipImpl_Mod64((uint2)(s->x,s->c), MWC64X_A, MWC64X_M, distance);
    s->x=tmp.x;
    s->c=tmp.y;
}

void MWC64X_SeedStreams(random_state *s, ulong baseOffset, ulong perStreamOffset)
{
    uint2 tmp=MWC_SeedImpl_Mod64(MWC64X_A, MWC64X_M, 1, 0, baseOffset, perStreamOffset);
    s->x=tmp.x;
    s->c=tmp.y;
}

//! Return a 32-bit integer in the range [0..2^32)
uint MWC64X_NextUint(random_state *s)
{
    uint res=s->x ^ s->c;
    MWC64X_Step(s);
    return res;
}

float random_01(random_state *r)
{
    return MWC64X_NextUint(r) / 4294967295.0f;
}



#endif // RANDOM_CL