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
#ifndef LIGHT_SAMPLING_CL
#define LIGHT_SAMPLING_CL

#include "intersection/rayboxintersection.cl" 
#include "intersection/raydiscintersection.cl"
#include "intersection/raysphereintersection.cl" 
#include "intersection/rayplaneintersection.cl" 
#include "random.cl"
#include "shading/shading.cl" 
#include "light/light.cl"    

#include "transformations.cl" 
#include "transmittance.cl" 
#include "gradients.cl"

// Direction in light space
float coneFalloff(float cosWiWo, float cosFOV) {
    float cosFalloffEnd = cosFOV+0.01f*cosFOV;
    if(cosWiWo < cosFalloffEnd) {
        return 0.f;
    } else if(cosWiWo > cosFOV) { return 1.f; }

    float delta = (cosWiWo-cosFOV) / (cosFOV-cosFalloffEnd);
    return delta*delta*delta*delta; 
}

// wi will contain the outgoing direction of the light source towards pos
void sampleLightSource(__global LightSource const * __restrict lightSources, const int nLightSources, const float3 pos, float3* __restrict wi, float3* __restrict power, float* __restrict pdf, random_state* __restrict randstate) {
 
    uint globalId = get_global_id(1)*get_global_size(0)+get_global_id(0);

    int lightSourceId = min(convert_int(random_01(randstate)*convert_float(nLightSources)), (nLightSources-1));   

    LightSource lightSource = lightSources[lightSourceId];
    float3 origin;

    if( lightSource.type == LIGHT_POINT ) {
        float3 localOrigin = (float3)(0.f);
        origin = transformPoint(lightSource.tm, localOrigin);
        *wi = pos-origin;
        *pdf = 1.f;
        *power = lightSource.radiance / (fast_length(*wi)*fast_length(*wi));
        *wi = fast_normalize(*wi);
    } 
    else if (lightSource.type == LIGHT_AREA ) {
        // p' Position on light source
        // p  Position on path
        // G(p, p') = dot (w_l, w_i) / length(p'-p)^2
        // L = 1/(p*p') * Le * G(p, p')
        // For uniform PDF p*p' = 1/A
        // => L = A * Le * G(p,p')



        float2 uv = (float2)(random_01(randstate), random_01(randstate)); 
        float3 localOrigin = (float3)(lightSource.size*(-0.5f+uv), 0.f);
        //float3 localOrigin = (float3)((float2)(0.03f, 0.5f)*(-0.5f+uv), 0.f);

        origin = transformPoint(lightSource.tm, localOrigin);

        float3 lightNormal = fast_normalize(transformPoint(lightSource.tm, (float3)(localOrigin.xy, 1.f))-origin);
        // Area light
        *wi = pos-origin;

        float nDotL = dot(lightNormal, fast_normalize(*wi));
        //*pdf = (fast_length(*wi)*fast_length(*wi))*(lightSource.area); 
        //*pdf = (fast_length(*wi)*fast_length(*wi))*(fabs(dot(lightNormal, normalize(*wi)))*lightSource.area); 
        // Uniform pdf
        if (nDotL > 0.f) {
            float dist = fast_length(*wi);
            *pdf = dist*dist/(nDotL*lightSource.area); 
            *power = lightSource.radiance;
        } else {
            *pdf = 0.f;
            *power = 0.f;
        } 
        *wi = fast_normalize(*wi);
        
    }  
    else if (lightSource.type == LIGHT_DIRECTIONAL ) {

            float3 localOrigin = (float3)(0.f, 0.f, 0.f);
            origin = transformPoint(lightSource.tm, localOrigin);
            // Directional light
            //float3 pointOnPlane = transformPoint(lightSource.tm, (float3)(localOrigin.xy, 10000001.f));
            // Get the direction using the rotational component
            // assuming that the z component was used to create the rotation matrix
            *wi = fast_normalize(transformVector(lightSource.tm, (float3)(0.f, 0.f, 1.f)));
            // Check if plane is pointing away from position 
            float nDotL = dot(pos-origin, *wi);
            if (nDotL > 0.f) {
                *pdf = 1.f/(M_PI);  
                *power = lightSource.radiance;
            } else {
                *pdf = 0.f;
                *power = 0.f;
            } 
            //*pdf = 1.f/(M_PI); 
            //*power = lightSource.radiance;

    } else { // if (lightSource.type == LIGHT_CONE ) 
        float3 localOrigin = (float3)(0.f);
        origin = transformPoint(lightSource.tm, localOrigin);
        *wi = fast_normalize(pos-origin);
        *pdf = 1.f;
        *power = lightSource.radiance*coneFalloff( dot(*wi, fast_normalize(origin-transformPoint(lightSource.tm,(float3)(0.f, 0.f, 1.f))) ), lightSource.cosFOV ) / (fast_distance(origin, pos)*fast_distance(origin, pos));     

    }  

    *power *= convert_float(nLightSources);
}

float lightSourcePdf(__global LightSource const * __restrict lightSources, const int lightSourceId, const float3 pos, const float3 toLightSourceDirection) {
 
    LightSource lightSource = lightSources[lightSourceId];
    float pdf;
    if( lightSource.type == LIGHT_POINT ) {
        pdf = 1.f;
    } 
    else if (lightSource.type == LIGHT_AREA ) {

        float3 lightNormal = fast_normalize( transformPoint( lightSource.tm, (float3)(0.f, 0.f, 1.f)-(float3)(0.f) ) );
        // Area light
        pdf = fast_length(toLightSourceDirection)*fast_length(toLightSourceDirection)/(fabs(dot(lightNormal, toLightSourceDirection))*lightSource.area);         
    }  
    else if (lightSource.type == LIGHT_DIRECTIONAL ) {
        pdf = 1.f; 
    } else { // if (lightSource.type == LIGHT_CONE ) 
        pdf = 1.f;
    }  

    return pdf;
}
  
bool rayLightIntersection(LightSource lightSource, const float3 o, const float3 d, float * __restrict t0, float* __restrict t1) {
    if( lightSource.type == LIGHT_POINT ) {
        float3 origin = transformPoint(lightSource.tm, (float3)(0.f)); 
        return raySphereIntersection(origin, 0.01f, o, d, t0, t1);
    }     
    else if (lightSource.type == LIGHT_AREA ) {
        float3 A = transformPoint(lightSource.tm, 0.5f*(float3)(-lightSource.size, 0.f));
        float3 lightNormal = fast_normalize(transformPoint(lightSource.tm, (float3)(-0.5f*lightSource.size, 1.f))-A);
        // No intersection if the direction is pointing away from the light source
        if( dot(lightNormal, d) > 0.f )
            return false;;

        float3 B = transformPoint(lightSource.tm, 0.5f*(float3)(lightSource.size.x, -lightSource.size.y, 0.f));
        float3 C = transformPoint(lightSource.tm, 0.5f*(float3)(lightSource.size.x, lightSource.size.y, 0.f));
        float3 D = transformPoint(lightSource.tm, 0.5f*(float3)(-lightSource.size.x, lightSource.size.y, 0.f));
            
        return rayQuadIntersection(A, B, C, D, o, d, t0, t1);
    } 
    else if (lightSource.type == LIGHT_DIRECTIONAL ) {
        float3 origin = transformPoint(lightSource.tm, (float3)(0.f, 0.f, 0.f));
        // Get the direction using the rotational component
        // assuming that the z component was used to create the rotation matrix
        float3 normal = fast_normalize(transformVector(lightSource.tm, (float3)(0.f, 0.f, 1.f)));
        // Check if plane is pointing away from position 
        //if (dot(o-origin, normal) <= 0.f) {
        // Check if the light is parallel and pointing toward the origin
        if (dot(d, normal) > -0.99f) {
            return false;
        }
        //float3 normal = fast_normalize(transformPoint4(lightSource.tm, (float4)(0.f, 0.f, 1.f, 0.f)).xyz);
        return rayPlaneIntersection(origin, normal, o, d, t0, t1);
    } else { // if (lightSource.type == LIGHT_CONE ) 
        float3 origin = transformPoint(lightSource.tm, (float3)(0.f));
        float3 pointOnPlane = transformPoint(lightSource.tm, (float3)(0.f, 0.f, 0.1f));
        float3 normal = fast_normalize(pointOnPlane-origin);
        return rayDiscIntersection(origin, normal, lightSource.size.x, o, d, t0, t1);
    }  
}

bool rayAnyLightIntersection(__global LightSource const* __restrict lightSources, int nLightSources, const float3 o, const float3 d, float * __restrict t0, float* __restrict t1, float3* Li, int* lightHitId) {

    for(int lightSourceId = 0; lightSourceId < nLightSources; ++lightSourceId) {
        LightSource lightSource = lightSources[lightSourceId];
        *lightHitId = lightSourceId;
        *Li = lightSource.radiance;
        float t0Tmp = *t0; float t1Tmp = *t1;
        bool hit = rayLightIntersection(lightSource, o, d, &t0Tmp, &t1Tmp);
        if(hit) {
            *t0 = t0Tmp;
            *t1 = t1Tmp;
            return true;
        }
    }
    return false;    
}


#endif