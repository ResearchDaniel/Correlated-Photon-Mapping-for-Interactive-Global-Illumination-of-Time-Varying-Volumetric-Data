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

#include "samplers.cl" 

// rotate/flip a quadrant appropriately
void rotateHilbertQuadrant(int n, int *x, int *y, int rx, int ry) {
    if (ry == 0) {
        if (rx == 1) {
            *x = n-1 - *x;
            *y = n-1 - *y;
        }
        //Swap x and y
        int t  = *x;
        *x = *y;
        *y = t;
    }
}

// Flatten (x,y) location to index d, http://en.wikipedia.org/wiki/Hilbert_curve
// n is power of two, which divides a square into nxn blocks.
int HilbertCurve2D (int n, int x, int y) {
    int rx, ry, s, d=0;
    for (s=n/2; s>0; s/=2) {
        rx = (x & s) > 0;
        ry = (y & s) > 0;
        d += s * s * ((3 * rx) ^ ry);
        rotateHilbertQuadrant(s, &x, &y, rx, ry);
    }
    return d;
}

// Compute a 1D grid index from samples in [0 1], stored in the xy component of a float4 
__kernel void sampleGridIndexKernel(
      __global float4* samples
    , float2 invBlockSize
    , int2 nBlocks
    , int nSamples
    , __global unsigned int* gridIndices
    ) 
{ 
    //output image pixel coordinates 
    int threadId = get_global_id(0);  
    if (threadId>=nSamples) {
        return;
    }
    float4 sample = samples[threadId];
    uint2 gridIndex = convert_uint2(sample.xy*invBlockSize); 
#ifdef USE_REGULAR_GRID
    gridIndices[threadId] = gridIndex.y*nBlocks.x + gridIndex.x;
#else 
    // Hilbert curve http://en.wikipedia.org/wiki/Hilbert_curve
    gridIndices[threadId] = HilbertCurve2D(nBlocks.x, gridIndex.x, gridIndex.y);
#endif

}