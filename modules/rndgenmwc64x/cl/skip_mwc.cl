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

/*
    Part of MWC64X by David Thomas, dt10@imperial.ac.uk
    This is provided under BSD, full license is with the main package.
    See http://www.doc.ic.ac.uk/~dt10/research
*/
#ifndef dt10_mwc64x_skip_cl
#define dt10_mwc64x_skip_cl

// Pre: a<M, b<M
// Post: r=(a+b) mod M
ulong MWC_AddMod64(ulong a, ulong b, ulong M)
{
    ulong v=a+b;
    if( (v>=M) || (v<a) )
        v=v-M;
    return v;
}

// Pre: a<M,b<M
// Post: r=(a*b) mod M
// This could be done more efficently, but it is portable, and should
// be easy to understand. It can be replaced with any of the better
// modular multiplication algorithms (for example if you know you have
// double precision available or something).
ulong MWC_MulMod64(ulong a, ulong b, ulong M)
{    
    ulong r=0;
    while(a!=0){
        if(a&1)
            r=MWC_AddMod64(r,b,M);
        b=MWC_AddMod64(b,b,M);
        a=a>>1;
    }
    return r;
}


// Pre: a<M, e>=0
// Post: r=(a^b) mod M
// This takes at most ~64^2 modular additions, so probably about 2^15 or so instructions on
// most architectures
ulong MWC_PowMod64(ulong a, ulong e, ulong M)
{
    ulong sqr=a, acc=1;
    while(e!=0){
        if(e&1)
            acc=MWC_MulMod64(acc,sqr,M);
        sqr=MWC_MulMod64(sqr,sqr,M);
        e=e>>1;
    }
    return acc;
}

uint2 MWC_SkipImpl_Mod64(uint2 curr, ulong A, ulong M, ulong distance)
{
    ulong m=MWC_PowMod64(A, distance, M);
    ulong x=curr.x*(ulong)A+curr.y;
    x=MWC_MulMod64(x, m, M);
    return (uint2)((uint)(x/A), (uint)(x%A));
}

uint2 MWC_SeedImpl_Mod64(ulong A, ulong M, uint vecSize, uint vecOffset, ulong streamBase, ulong streamGap)
{
    // This is an arbitrary constant for starting LCG jumping from. I didn't
    // want to start from 1, as then you end up with the two or three first values
    // being a bit poor in ones - once you've decided that, one constant is as
    // good as any another. There is no deep mathematical reason for it, I just
    // generated a random number.
    enum{ MWC_BASEID = 4077358422479273989UL };
    
    ulong dist=streamBase + (get_global_id(0)*vecSize+vecOffset)*streamGap;
    ulong m=MWC_PowMod64(A, dist, M);
    
    ulong x=MWC_MulMod64(MWC_BASEID, m, M);
    return (uint2)((uint)(x/A), (uint)(x%A));
}

#endif
