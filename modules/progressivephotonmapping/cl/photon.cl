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
#ifndef PHOTON_CL
#define PHOTON_CL

#define PHOTON_DATA_TYPE float8 
//#define PHOTON_DATA_TYPE half

//struct Photon {
//    // (float8)(photonPos.x, photonPos.y, photonPos.z, photonPower.x, photonPower.y, photonPower.z, dirAngles.x, dirAngles.y);
//    vec3 pos;
//    vec3 power; // RGB
//    vec2 encodedDirection; // Storing the direction encoded to make struct 8 float * 4 byte = 32 byte => aligned reads
//
//    void setDirection(vec3 dir);
//    vec3 getDirection() const;
//
//};

float8 readPhoton(__global const PHOTON_DATA_TYPE* photonData, int photonId) {
#ifdef PHOTON_DATA_TYPE_HALF
    return vloada_half8(photonId, photonData);
#else
    return photonData[photonId];
#endif
}

void writePhoton(float8 photon, __global PHOTON_DATA_TYPE* photonData, int photonId) {
#ifdef PHOTON_DATA_TYPE_HALF
    vstore_half8(photon, photonId, photonData);
#else
    photonData[photonId] = photon;
#endif      
}

  
#endif 