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
#ifndef DENSITY_ESTIMATION_KERNEL_CL
#define DENSITY_ESTIMATION_KERNEL_CL

/**
* \brief Kernel for density estimate. Note that x in [0 1]. I.e abs(x), x in [-1 1]  is expected as input.
* The function integrates to 1 over the domain x in [-1 1].
* http://en.wikipedia.org/wiki/Uniform_kernel#Kernel_functions_in_common_use
*
* @param x Absolute value of parameter Value in range [0 1]
* @return Kernel weight, 0 if x > 1
*/
float densityEstimationKernel(float x) {
    //if(x <= 1.f) {
    //   return 0.5f;
    //}else {
    //   return 0.f;
    //}
    //// Triangle
    //if(x <= 1.f) {
    //   return 1.f-x;
    //}else {
    //   return 0.f;
    //}
    // Epanechnikov
    if (x <= 1.f) {
        return (0.75f)*(1.f - x*x);
    } else {
        return 0.f;
    }
    // Quartic
    //(biweight)
    //if(x <= 1.f) {
    //    return (15.f/16.f)*pown((1.f-x*x),2);
    //}else {
    //    return 0.f;
    //}
    // Triweight
    //if(x <= 1.f) {
    //    return (35.f/32.f)*pown((1.f-x*x),3);
    //}else {
    //    return 0.f;
    //}
    // Tricube
    //if(x <= 1.f) {
    //    return (70.f/81.f)*pown((1.f-fabs(x*x*x)),3);
    //}else {
    //    return 0.f;
    //}
    // Gaussian
    //if(x <= 1.f) {
    //   //return (1.f/(sqrt(M_PI*2.f)))*native_exp(-0.5f*x*x);
    //   // Optimized
    //   return 0.28209479177387814347403972578039f*native_exp(-0.5f*x*x);
    //}else return 0.f;
    // Chebyshev–Gauss
    //if (x <= 1.f) {
    //    //return (1.f/(sqrt(M_PI*2.f)))*native_sqrt(1-x*x);
    //    // Optimized
    //    return 0.28209479177387814347403972578039f*native_sqrt(1.f-x*x);
    //} else return 0.f;
    // Cosine
    //if(x <= 1.f) {
    //   return (M_PI/4.f)*cos((M_PI/2.f)*x);
    //}else return 0.f;
}

  
#endif 