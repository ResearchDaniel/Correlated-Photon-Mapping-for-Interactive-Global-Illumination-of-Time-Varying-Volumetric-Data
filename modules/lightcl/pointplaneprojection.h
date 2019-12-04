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

#ifndef IVW_POINTPLANEPROJECTION_H
#define IVW_POINTPLANEPROJECTION_H

#include <modules/lightcl/lightclmoduledefine.h>
#include <inviwo/core/common/inviwo.h>
#include <inviwo/core/datastructures/geometry/plane.h>

namespace inviwo {
namespace geometry {

/**
* \brief Project points onto plane and express them in the coordinates system given by the plane origin and u and v.
*     Plane
* x   ->|
*    x->|
* Looking along the plane normal:
* v ^
*   |    x
*   | x
*   o-----> u

* @param const std::vector<vec3> & points    Points to project
* @param const Plane & plane                 Plane to project points on
* @param vec3 u                              Normalized plane tangent u-vector
* @param vec3 v                              Normalized plane tangent u-vector
* @param std::vector<vec2> & projectedPoints uv-coordinates of projected points
*
*/
IVW_MODULE_LIGHTCL_API void projectPointsOnPlane(const std::vector<vec3> &points, const Plane &plane, vec3 u, vec3 v, std::vector<vec2>& projectedPoints);


/** 
 * \brief Returns an iterator to the closest point on the positive side of the plane.
 *
 * 
 * @param ForwardIterator first Input iterator to the initial position of the sequence to compare.
 * @param ForwardIterator last  Input iterators to the final position of the sequence to compare. The range used is [first,last), which contains all the elements between first and last, including the element pointed by first but not the element pointed by last.
 * @param const Plane & plane   The plane to project points on.
 * @return ForwardIterator      An iterator to smallest value in the range, or last if the range is empty.
 */
template< typename ForwardIterator>
ForwardIterator findClosestPoint(ForwardIterator first, ForwardIterator last, const Plane &plane) {
    if (first == last) {
        return last;
    }
    vec3 n = plane.getNormal();
    float d = glm::dot(n, plane.getPoint());
    auto closestPoint = first;
    float closestPointDistance = FLT_MAX;
    while (++first != last) {
        float distanceFromPlane = glm::dot(n, *first) - d;
        if (distanceFromPlane >= 0 && distanceFromPlane < closestPointDistance) {
            closestPoint = first;
        }
    }
    return closestPoint;
}

} // namespace geometry
} // namespace

#endif // IVW_POINTPLANEPROJECTION_H

