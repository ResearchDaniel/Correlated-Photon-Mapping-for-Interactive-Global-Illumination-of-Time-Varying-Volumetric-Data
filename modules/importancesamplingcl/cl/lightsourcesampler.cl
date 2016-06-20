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
#include "datastructures/lightsample.cl"
#include "intersection/rayboxintersection.cl" 


LightSample sampleLight(LightSource lightSources, float4 uvwPdf) {
    LightSample sample;
    sample.t0 = 0.f; sample.t1 = FLT_MAX;
    if (lightSource.type == LIGHT_POINT) {
        float3 localOrigin = (float3)(0.f);
        sample.origin = transformPoint(lightSource.tm, localOrigin);
        float3 sphereSample = uniformSampleSphere(uvwPdf.xy);
        sample.direction = -sphereSample;

        sample.pdf = uniformSpherePdf();
        sample.power = lightSource.radiance / sample.pdf;
    } else if (lightSource.type == LIGHT_AREA) {
        float3 localOrigin = (float3)(lightSource.size*(-0.5f + uvwPdf.xy), 0.f);
        float3 lightCoordinateSystem
            sample.direction = uniformSampleHemisphere(uvwPdf.xy);
        float3 lighNormal = normalize(transformVector(lightSource.tm, (float3)(0.f, 0.f, 1.f)));
        float3 Nu = (float3)(0.f); float3 Nv = (float3)(0.f);
        createCoordinateSystem(lighNormal, &Nu, &Nv);
        sample.direction = shadingToWorld(Nu, Nv, lightNormal, sample.direction);

        sample.origin = transformPoint(lightSource.tm, localOrigin);
        // Area light
        //float3 pointOnPlane = bbox.pMin + (bbox.pMax - bbox.pMin)*(float3)(rndNum.x, rndNum.y, rndNum.z);
        //sample.direction = normalize(pointOnPlane - sample.origin);
        sample.pdf = lightSource.area;

        sample.power = lightSource.radiance / sample.pdf;
    } else if (lightSource.type == LIGHT_DIRECTIONAL) {
        float3 localOrigin = (float3)(lightSource.size*(-0.5f + uvwPdf.xy), 0.f);
        sample.origin = transformPoint(lightSource.tm, localOrigin);
        // Directional light
        sample.direction = normalize(transformVector(lightSource.tm, (float3)(0.f, 0.f, 1.f)));
        sample.pdf = 1.f / (M_PI_F);
        sample.power = lightSource.radiance / sample.pdf;

    } else { // if (lightSource.type == LIGHT_CONE ) 
        float3 localOrigin = (float3)(0.0f, 0.f, 0.f);
        sample.origin = transformPoint(lightSource.tm, localOrigin);

        float3 localDir = uniformSampleCone(uvwPdf.xy, lightSource.cosFOV);
        float3 pointOnPlane = transformPoint(lightSource.tm, localDir);
        sample.direction = normalize(pointOnPlane - sample.origin);
        sample.pdf = uniformConePdf(lightSource.cosFOV);

        sample.power = localDir.z*localDir.z*localDir.z*localDir.z*lightSource.radiance*localDir.z / (sample.pdf);
    }

    return sample;
}


__kernel void lightSourceSamplerKernel(
    __global float4 const * __restrict samples
    , __global LightSource const * __restrict lightSources
    , int nLights
    , int nSamples
    , __global float const * __restrict vertices
    , __global int const * __restrict indices
    , int nIndices
    , __global LightSample* lightSamples
    ) 
{ 
    //output image pixel coordinates 

 
    int threadId = get_global_id(0);
    if (threadId >= nSamples) {
        return;
    }

    LightSample lightSample = sampleLight(lightSources[0], samples[threadId]);

    bool iSect = rayMeshIntersection(vertices, indices, nIndices, lightSample.origin, lightSample.direction, &lightSample.t0, &lightSample.t1);

    lightSamples[threadId] = lightSample;
}