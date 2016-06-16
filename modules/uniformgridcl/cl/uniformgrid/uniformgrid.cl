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
#ifndef UNIFORMGRID_CL
#define UNIFORMGRID_CL

#include "samplers.cl" 
#include "transformations.cl" 
#include "intersection/rayboxintersection.cl"

#define OPTIMIZE_STEP_FOR_SIMD

void setupUniformGridTraversal(float3 x1, float3 x2, float3 cellDim, int3 maxCells
                               , int3* cellCoord, int3* cellCoordEnd, int3* di, float3* dt, float3* deltatx) {
    // x in [0 dim]
    // cell = clamp( floor(x), 0, dim-1) 
    float3 cellCoordf =  clamp(floor((x1)/cellDim), (float3)(0.f), convert_float3(maxCells-1));    
    // Determine start grid cell coordinates
    *cellCoord = convert_int3(cellCoordf);
    // Determine the end grid cell coordinates
    *cellCoordEnd = clamp(convert_int3((x2)/cellDim), (int3)(0), maxCells-1);
    //*cellCoordEnd = convert_int3_rtz(x2/cellDim);
    
    // Determine primary step direction
    *di = ((x1 < x2) ? 1 : ((x1>x2) ? -1 : 0));

    // Determine dt, the values of t at wich the directed segment
    // x1-x2 crosses the first horizontal and vertical cell
    // boundaries, respectively. Min(dt) indicates how far 
    // we can travel along the segment and still remain in
    // the current cell
    float3 invAbsDir = 1.f/fabs(x2-x1);

    float3 minx = cellDim*cellCoordf;
    float3 maxx = minx + cellDim;
    *dt = ((x1>x2) ? (x1-minx) : (maxx-x1)) * invAbsDir;

    // Determine delta x, how far in units for t we must 
    // step along the directed line segment for the 
    // horizontal/vertical/depth movement to equal
    // the width/height/depth of a cell
    // t is parameterized as [0 1] along the ray.
    *deltatx = cellDim * invAbsDir;
}

/*
 * Step to next grid cell using parameters calculated in setupUniformGridTraversal.
 *        _____________
 *       |      |      |
 *       |      |      |
 *       |______|______|
 *       |      |      |
 * ray-> x      x      |
 *       |______|______|
 * @return True if succesfully stepped to the next cell, otherwise false (reached final cell coordinate).
 */
inline bool stepToNextCell(float3 deltatx, int3 di, int3 cellCoordEnd, float3 *dt, int3* cellCoord, float* tHit) {
#ifdef OPTIMIZE_STEP_FOR_SIMD
	// Find axis with minimum distance
	// Don't know why the following does not work for the z-component
	// ((*dt).x >  (*dt).z && (*dt).y >  (*dt).z) ? -1 : 0)
    int3 advance = (int3)(((*dt).x <= (*dt).y && (*dt).x <= (*dt).z) ? -1 : 0, 
						  ((*dt).x >  (*dt).y && (*dt).y <= (*dt).z) ? -1 : 0, 
						  0); 
						  // (((*dt).x <= (*dt).y && (*dt).x <= (*dt).z) || ((*dt).x >  (*dt).y && (*dt).y <= (*dt).z)) ? 0 : -1
						  // ((*dt).x >  (*dt).z && (*dt).y >  (*dt).z) ? -1 : 0);
	advance.y = (advance.x == -1) ? 0 : advance.y; 
	advance.z = (advance.x == -1 || advance.y == -1) ? 0 : -1; 
    if (any(advance && ((*cellCoord) == cellCoordEnd))) {
        return false;
    }
    (*tHit) = advance.x != 0 ? (*dt).x : (advance.y != 0 ? (*dt).y : (*dt).z);
    (*dt) += select((float3)(0), deltatx, advance);
    (*cellCoord) += select((int3)(0), di, advance);

#else 
    
    if ((*dt).x <= (*dt).y && (*dt).x <= (*dt).z) {
        if ((*cellCoord).x == cellCoordEnd.x) {
            return false;
        }
        (*tHit) = (*dt).x;
        (*dt).x += deltatx.x;
        (*cellCoord).x += di.x;

    } else if ((*dt).y <= (*dt).x && (*dt).y <= (*dt).z) {
        if ((*cellCoord).y == cellCoordEnd.y) {
            return false;
        }
        (*tHit) = (*dt).y;
        (*dt).y += deltatx.y;
        (*cellCoord).y += di.y;

    } else {
        if ((*cellCoord).z == cellCoordEnd.z) {
            return false;
        }
        (*tHit) = (*dt).z;
        (*dt).z += deltatx.z;
        (*cellCoord).z += di.z;
    }
#endif
    return true;
}

/*
 * Step to next grid cell using parameters calculated in setupUniformGridTraversal.
 * In difference to the function stepToNextCell this function returns the hit point  
 * of the next cell.
 *
 *        _____________
 *       |      |      |
 *       |      |      |
 *       |______|______|
 *       |      |      |
 * ray-> |      x      x
 *       |______|______|
 *
 * Calculates the next hit point along the ray
 * @return True if succesfully stepped to the next cell, otherwise false (reached final cell coordinate).
 */
bool stepToNextCellNextHit(float3 deltatx, int3 di, int3 cellCoordEnd, float3 *dt, int3* cellCoord, float* tHit) {
    
#ifdef OPTIMIZE_STEP_FOR_SIMD
	// Find axis with minimum distance
	// Don't know why the following does not work for the z-component
	// ((*dt).x >  (*dt).z && (*dt).y >  (*dt).z) ? -1 : 0)
    int3 advance = (int3)(((*dt).x <= (*dt).y && (*dt).x <= (*dt).z) ? -1 : 0, 
						  ((*dt).x >  (*dt).y && (*dt).y <= (*dt).z) ? -1 : 0, 
						  0); 
						  // (((*dt).x <= (*dt).y && (*dt).x <= (*dt).z) || ((*dt).x >  (*dt).y && (*dt).y <= (*dt).z)) ? 0 : -1
						  // ((*dt).x >  (*dt).z && (*dt).y >  (*dt).z) ? -1 : 0);
	advance.y = (advance.x == -1) ? 0 : advance.y; 
	advance.z = (advance.x == -1 || advance.y == -1) ? 0 : -1; 

    (*tHit) = advance.x != 0 ? (*dt).x : (advance.y != 0 ? (*dt).y : (*dt).z);
    if (any(advance && ((*cellCoord) == cellCoordEnd))) {
        return false;
    }
    
    (*dt) += select((float3)(0), deltatx, advance);
    (*cellCoord) += select((int3)(0), di, advance);
    
#else 
    if ((*dt).x <= (*dt).y && (*dt).x <= (*dt).z) {
        (*tHit) = (*dt).x;
        if ((*cellCoord).x == cellCoordEnd.x) {
            return false;
        }
        (*dt).x += deltatx.x;
        (*cellCoord).x += di.x;

    } else if ((*dt).y <= (*dt).x && (*dt).y <= (*dt).z) {
        (*tHit) = (*dt).y;
        if ((*cellCoord).y == cellCoordEnd.y) {
            return false;
        }
        (*dt).y += deltatx.y;
        (*cellCoord).y += di.y;

    } else {
        (*tHit) = (*dt).z;
        if ((*cellCoord).z == cellCoordEnd.z) {
            return false;
        } 
        (*dt).z += deltatx.z;
        (*cellCoord).z += di.z;
    }
#endif
    return true;

}



#endif

