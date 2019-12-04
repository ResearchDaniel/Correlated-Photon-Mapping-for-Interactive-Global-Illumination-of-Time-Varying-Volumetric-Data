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

#ifndef IVW_ORIENTEDBOUNDINGBOX2D_H
#define IVW_ORIENTEDBOUNDINGBOX2D_H

#include <modules/lightcl/lightclmoduledefine.h>
#include <inviwo/core/common/inviwo.h>
#include <inviwo/core/datastructures/geometry/plane.h>

namespace inviwo {
namespace geometry {
/**
* \class OrientedBoundingBox2D
*
* \brief Bounding box oriented according to vectors u and v.
*      v
*     /
*    o
*     \
*      u
*/
struct IVW_MODULE_LIGHTCL_API OrientedBoundingBox2D {
    OrientedBoundingBox2D(vec2 o, vec2 right, vec2 up) : origin(o), u(right), v(up) {}
    vec2 origin; // Origin
    vec2 u;
    vec2 v;
};

/**
* \brief Compute optimal bounding box around a convex hull.
*
* Computes the origin and two sides of rectangle covering the points.
* Sides are not normalized.
*        v ^
*          | x   x
*          x  x
*          o--x-x-> u
*  origin
*
* See page 111 in Real-Time Collision Detection
* @param std::vector<vec2> & convexHull Points to bound.
* @return std::tuple<vec2, vec2, vec2> origin and two sides given by two vectors u, v.
*/
IVW_MODULE_LIGHTCL_API OrientedBoundingBox2D mimumBoundingRectangle(const std::vector<vec2> &convexHull);

/** 
 * \brief Fit oriented bounding box of points projected to the plane.
 *
 * Projects all the points onto the plane and computes the optimal
 * bounding box in 2D. 
 * Returns the origin and the u,v vectors describing the bounding box.
 * 
 * @param const std::vector<vec3> & points Points to project and fit bounding box.
 * @param const Plane & plane              Plane to project points onto.
 * @return IVW_MODULE_LIGHTCL_API std::tuple<vec3, vec3, vec3> Origin and sides of bounding box: u (right), v (up).
 */
IVW_MODULE_LIGHTCL_API std::tuple<vec3, vec3, vec3> fitPlaneAlignedOrientedBoundingBox2D(const std::vector<vec3>& points, const Plane& plane);
}
} // namespace

#endif // IVW_ORIENTEDBOUNDINGBOX2D_H

