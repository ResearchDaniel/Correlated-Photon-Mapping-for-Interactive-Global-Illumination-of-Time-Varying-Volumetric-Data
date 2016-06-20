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
#include "transformations.cl" 
#include "uniformgrid/uniformgrid.cl"  
#include "colorconversion.cl" 

/*
 * Step to next grid cell using parameters calculated in setupUniformGridTraversal.
 * @return True if succesfully stepped to the next cell, otherwise false (reached final cell coordinate).
 */
bool stepToNextCell2(float3 deltatx, int3 di, int3 cellCoordEnd, float3 *dt, int3* cellCoord, float* tHit) {
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
    return true;
}


/*
 * Find first grid point hit point calculated in index coordinates [0 dim-1]
 * using maximum value within each grid cell.
 *
 * @param x1 Start point in index coordinates
 * @param x2 End point in index coordinates
 * @param direction Normalized direction
 * @param cellDim Dimension of a grid cell
 * @param volume Data containing maximum data value within each grid cell
 * @param volumeMaxSize Size of volume
 * @param dataMinMaxValueThreshold All grids with value below min and above max value will be culled
 * @param tHit Output the distance to the hitpoint along the direction from x1
 * @return True if a hit is found, false otherwise
 */
float uniformGridImportance(float3 x1, float3 x2
    , float3 cellDim 
    , __global const ushort2* uniformGrid3D
    , int4 volumeMaxSize
    , float2 dataMinMaxValueThreshold, float3 *tHit, float16 indexToTextureMat) {
    int3 cellCoord, cellCoordEnd, di;
    float3 dt, deltatx; 
            // Deal with numerical issues when entry/exit are aligned with grid
    float translate = 0.0001f; 
    float3 dir = (x2-x1);

    setupUniformGridTraversal(x1, x2, cellDim, volumeMaxSize.xyz,
                              &cellCoord, &cellCoordEnd, &di, &dt, &deltatx);
       
    // Visit all cells
    bool continueTraversal = true;
    int iterations = 0;
    float importance = 0.f;
    // Length in index coordinates space
    //float len = length(x2-x1);
     float len = length(transformPoint(indexToTextureMat, x2)-transformPoint(indexToTextureMat, x1));

     float dt0 = (0.f);  
     float dt1 = (0.f);    

     float3 dtt = dt; 
     int iter = 0;
     float t = 0.f;
    //float3 direction = normalize(x2-x1);
    float3 direction = (x2-x1);
    float accum = 0;
    while(continueTraversal) {  
        // Visit cell      
        float2 gridMinMaxVal = (1.f / 65535.f)*convert_float2(uniformGrid3D[cellCoord.x + cellCoord.y*volumeMaxSize.x + cellCoord.z * volumeMaxSize.x*volumeMaxSize.y]);
        
        dt0 = dt1; 
        continueTraversal = stepToNextCell2(deltatx, di, cellCoordEnd, &dt, &cellCoord, &dt1); 
        if (!(gridMinMaxVal.y < dataMinMaxValueThreshold.x || gridMinMaxVal.x > dataMinMaxValueThreshold.y)) {
            importance += (min(1.f, dt1)-dt0); 
        }
        
    }   
    // dt goes between 0 and one so we need to take the length of the ray into account
    return importance*len; 
} 

float classifyMinMaxImportance(float2 minMax, float2 dataMinMaxValueThreshold) {
    if ((minMax.y < dataMinMaxValueThreshold.x || minMax.x > dataMinMaxValueThreshold.y)) {
        return 0.f;
    } else {
        return 1.f;
    }
}

float importanceForRange(float2 dataRange, __global float2 const* __restrict dataRangeImportance, int dataRangeImportanceSize) {
    int i = 0;
    float importance = 0.f;
    // Skip until we reach first range
    while (i < dataRangeImportanceSize - 1 && dataRange.x > dataRangeImportance[i + 1].x) {
        ++i;
    }
    // Compute maximum importance in data range
    do {
        importance = max(importance, dataRangeImportance[i].y);
        ++i;
    } while (i < dataRangeImportanceSize && dataRange.y > dataRangeImportance[i].x);
    return importance;
}

typedef struct ImportanceWeights {
    float colorWeight;
    float colorDiffWeight;
    float opacityDiffWeight;
    float opacityWeight;
}ImportanceWeights;

//#define INCREMENTAL_TF_IMPORTANCE

#ifdef INCREMENTAL_TF_IMPORTANCE
float tfPointsImportance(float4 color, float4 nextColor, ImportanceWeights weights) {
    float importance = 0.f;
    float4 colorDifference = nextColor;
    importance = colorDifference.x + colorDifference.y + colorDifference.z + colorDifference.w;
    return importance;
}
#else
float tfPointsImportance(float4 color, float4 nextColor, ImportanceWeights weights) {
    float importance = 0.f;
    if (color.w > 0.f || nextColor.w > 0.f) {
        float3 nextLab = rgb2lab(nextColor.xyz);
        float3 lab = rgb2lab(color.xyz);

        float3 colorDiff = nextLab - lab;
        float opacityDiff = nextColor.w - color.w;
        importance = weights.colorWeight*max(length(nextLab), length(lab)) + weights.colorDiffWeight*length(colorDiff) + weights.opacityDiffWeight * fabs(opacityDiff) + weights.opacityWeight*max(color.w, nextColor.w);
    }

    return importance;
}
#endif

float importanceForRangeTF(float2 dataRange, __global float const* __restrict positions, __global float4 const* __restrict colors, int nPoints, ImportanceWeights weights) {
    int i = 0;
    float importance = 0.f;
    // Skip until we reach first range
    while (i < nPoints - 1 && dataRange.x > positions[i + 1]) {
        ++i;
    }

    // Interpolate start color
    float4 color = mix(colors[i], colors[i + 1], (dataRange.x - positions[i]) / (positions[i + 1] - positions[i]));

    float4 minColor = color;
    float4 maxColor = minColor;
    if (dataRange.y <= positions[i + 1]) {
        float4 nextColor = mix(colors[i], colors[i + 1], (dataRange.y - positions[i]) / (positions[i + 1] - positions[i]));

        minColor = min(minColor, nextColor);
        maxColor = max(maxColor, nextColor);
        return tfPointsImportance(minColor, maxColor, weights);
    } else {

        float4 nextColor = colors[i + 1];
        minColor = min(minColor, nextColor);
        maxColor = max(maxColor, nextColor);
        ++i;
    }
    while (i < nPoints - 1 && dataRange.y > positions[i + 1]) {

        float4 nextColor = colors[i + 1];
        minColor = min(minColor, nextColor);
        maxColor = max(maxColor, nextColor);
        ++i;
    }
    if (i < nPoints - 1) {
        color = mix(colors[i], colors[i + 1], (dataRange.y - positions[i]) / (positions[i + 1] - positions[i]));

        float4 nextColor = color;
        minColor = min(minColor, nextColor);
        maxColor = max(maxColor, nextColor);
    }
    
    return tfPointsImportance(minColor, maxColor, weights);

    //// Interpolate start color
    //float4 color = mix(colors[i], colors[i + 1], (dataRange.x - positions[i]) / (positions[i + 1] - positions[i]));
    //if (dataRange.y <= positions[i + 1]) {
    //    float4 nextColor = mix(colors[i], colors[i + 1], (dataRange.y - positions[i]) / (positions[i + 1] - positions[i]));
    //    return tfPointsImportance(color, nextColor, weights);
    //} else {
    //    importance = tfPointsImportance(color, colors[i + 1], weights);
    //    ++i;
    //}
    //while (i < nPoints - 1 && dataRange.y > positions[i + 1]) {
    //    importance = max(importance, tfPointsImportance(colors[i], colors[i + 1], weights));
    //    ++i;
    //}
    //if (i < nPoints - 1) {
    //    color = mix(colors[i], colors[i + 1], (dataRange.y - positions[i]) / (positions[i + 1] - positions[i]));
    //    importance = max(importance, tfPointsImportance(colors[i], color, weights));
    //}


    return importance;
}

__kernel void minMaxUniformGrid3DImportanceKernel(
    __global const ushort2* minMaxUniformGrid3D
    , int nElements
    , __global float2 const* __restrict dataRangeImportance
    , int dataRangeImportanceSize
    , float2 dataMinMaxValueThreshold
    , __global float* importanceUniformGrid3D) {
    if (get_global_id(0) >= nElements) {
        return;
    }
    float2 gridMinMaxVal = (1.f / 65535.f)*convert_float2(minMaxUniformGrid3D[get_global_id(0)]);
    float importance = importanceForRange(gridMinMaxVal, dataRangeImportance, dataRangeImportanceSize);
    //float importance = classifyMinMaxImportance(gridMinMaxVal, dataMinMaxValueThreshold);
    //importanceUniformGrid3D[get_global_id(0)] = convert_uchar_sat_rte(255.f*importance);
    importanceUniformGrid3D[get_global_id(0)] = importance;
}


__kernel void classifyMinMaxUniformGrid3DImportanceKernel(
    __global const ushort2* minMaxUniformGrid3D
    , int nElements
    , __global float const* __restrict positions
    , __global float4 const* __restrict colors
    , int nPoints
    , float colorWeight, float colorDiffWeight, float opacityDiffWeight, float opacityWeight
    , __global float* importanceUniformGrid3D) {
    if (get_global_id(0) >= nElements) {
        return;
    }
    float2 gridMinMaxVal = (1.f / 65535.f)*convert_float2(minMaxUniformGrid3D[get_global_id(0)]);
    ImportanceWeights weights;
    weights.colorWeight = colorWeight;
    weights.colorDiffWeight = colorDiffWeight;
    weights.opacityDiffWeight = opacityDiffWeight;
    weights.opacityWeight = opacityWeight;
    float importance = importanceForRangeTF(gridMinMaxVal, positions, colors, nPoints, weights);
    //importanceUniformGrid3D[get_global_id(0)] = convert_uchar_sat_rte(255.f*importance);
    importanceUniformGrid3D[get_global_id(0)] = importance;
}

__kernel void classifyTimeVaryingMinMaxUniformGrid3DImportanceKernel(
      __global const ushort2* minMaxUniformGrid3D
    , __global const ushort2* prevMinMaxUniformGrid3D
    , __global const float* volumeDiffInfoUniformGrid3D
    , int nElements
    , __global float const* __restrict positions
    , __global float4 const* __restrict colors
    , int nPoints
    , float colorWeight, float colorDiffWeight, float opacityDiffWeight, float opacityWeight
    , __global float* importanceUniformGrid3D) {
    if (get_global_id(0) >= nElements) {
        return;
    }
    ushort2 cellMinMaxVal = minMaxUniformGrid3D[get_global_id(0)];
    // Min-max value of previous time-step
    ushort2 prevCellMinMaxVal = prevMinMaxUniformGrid3D[get_global_id(0)];
    float2 gridMinMaxVal = (1.f / 65535.f)*convert_float2((ushort2)(min(cellMinMaxVal.x, prevCellMinMaxVal.x), max(cellMinMaxVal.y, prevCellMinMaxVal.y)));
    ImportanceWeights weights;
    weights.colorWeight = colorWeight;
    weights.colorDiffWeight = colorDiffWeight;
    weights.opacityDiffWeight = opacityDiffWeight;
    weights.opacityWeight = opacityWeight;
    float importance = 0.f;
    //if (distance(prevGridMinMaxVal, gridMinMaxVal) > 0.0f) {
    //if (volumeDiffInfoUniformGrid3D[get_global_id(0)] > 0) {
    //float prevImportance = importanceForRangeTF((1.f / 65535.f)*convert_float2(prevCellMinMaxVal), positions, colors, nPoints, weights);
    //float nextImportance = importanceForRangeTF((1.f / 65535.f)*convert_float2(cellMinMaxVal), positions, colors, nPoints, weights);
    //if (prevImportance == 0.f || nextImportance == 0.f) {
    //    importance = 2.f*volumeDiffInfoUniformGrid3D[get_global_id(0)] * (prevImportance + nextImportance);
    //} else {
    //    importance = volumeDiffInfoUniformGrid3D[get_global_id(0)] * (prevImportance + nextImportance);
    //}

    importance = volumeDiffInfoUniformGrid3D[get_global_id(0)] * importanceForRangeTF(gridMinMaxVal, positions, colors, nPoints, weights);
    //} else {
    //    importance = importanceForRangeTF(gridMinMaxVal, positions, colors, nPoints, weights);
    //}
    //importanceUniformGrid3D[get_global_id(0)] = convert_uchar_sat_rte(255.f*importance);
    importanceUniformGrid3D[get_global_id(0)] = importance;
}





__kernel void uniformGridImportanceKernel(__global const ushort2* uniformGrid3D
    , int4 volumeMaxSize
    , read_only image2d_t entryTexCol
    , read_only image2d_t exitTexCol
    , float2 dataMinMaxValueThreshold 
    , float16 textureToIndexMat
    , float16 indexToTextureMat
    , float3 cellSize
    , __global float* importance
    ) 
{
    //output image pixel coordinates 
    int2 globalId = (int2)(get_global_id(0), get_global_id(1));  
    
    int threadId = get_global_id(0) + get_global_id(1)*get_image_width(entryTexCol);
    if (any(globalId>=get_image_dim(entryTexCol))) {
        return;
    } 
    //int offset = get_image_width(entryTexCol)*get_image_height(entryTexCol);
    float4 entry = read_imagef(entryTexCol, smpUNormNoClampNearest, globalId);  
    //
    float4 result = (float4)(0.f);    
    float4 exit = read_imagef(exitTexCol, smpUNormNoClampNearest, globalId);   
    // x in [-0.5 dim-0.5]
    // add 0.5 to convert to [0 dim]
    float3 x1 =  transformPoint(textureToIndexMat, entry.xyz)+0.5f; 
    float3 x2 =  transformPoint(textureToIndexMat, exit.xyz)+0.5f;
    //// Deal with numerical issues when entry/exit are aligned with grid
    //float translate = 0.0001f; 
    //float3 dir = normalize(x2-x1);
    //x1 -= dir*translate;
    //x2 -= dir*translate;
    
    //x1 = x1*scale + translate;
    //x2 = x2*scale + translate;
    float3 t0;
    if (any(x1!=x2)) {
        importance[threadId] = uniformGridImportance(x1, x2, cellSize, uniformGrid3D, volumeMaxSize, dataMinMaxValueThreshold, &t0, indexToTextureMat);
    } else {
        importance[threadId] = 0.f;
    }

}


