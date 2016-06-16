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

#include "orientedboundingbox2d.h"
#include "pointplaneprojection.h"
#include "convexhull2d.h"

namespace inviwo {
namespace geometry {

OrientedBoundingBox2D mimumBoundingRectangle(const std::vector<vec2> &convexHull) {
    auto minArea = FLT_MAX;
    vec2 origin, u, v;
    auto nPoints = convexHull.size();
    // Compute box with minimum area
    for (size_t i = 0, j = convexHull.size() - 1; i < nPoints; j = i, ++i) {
        // Edge between two points on the hull
        vec2 e0 = glm::normalize(convexHull[i] - convexHull[j]);
        if (glm::any(glm::isnan(e0))) {
            continue;
        }
        // Axis orthogonal to e0
        vec2 e1 = vec2(-e0.y, e0.x);
        // Compute maximum extent 
        float min0 = 0.f, min1 = 0.f, max0 = 0.f, max1 = 0.f;
        for (auto k = 0; k < nPoints; ++k) {
            // Project points onto the two edges
            // and expand extent to cover points
            vec2 d = convexHull[k] - convexHull[j];
            float dot = glm::dot(d, e0);
            min0 = std::min(min0, dot);
            max0 = std::max(max0, dot);
            dot = glm::dot(d, e1);
            min1 = std::min(min1, dot);
            max1 = std::max(max1, dot);
        }
        auto area = (max0 - min0) * (max1 - min1);
        // Remember smallest box
        if (area < minArea) {
            minArea = area;
            // Move origin to lower-left corner
            origin = convexHull[j] + std::min(min0, 0.f)*e0 + std::min(min1, 0.f)*e1;
            // Store the two sides of the bounding box
            u = e0*(max0 - min0); v = e1*(max1 - min1);
        }
    }
    return OrientedBoundingBox2D(origin, u, v);
    //return std::make_tuple(origin, u, v);
}

std::tuple<vec3, vec3, vec3> fitPlaneAlignedOrientedBoundingBox2D(const std::vector<vec3>& points, const Plane& plane) {
    vec3 u, v;
    if (fabs(plane.getNormal().x) > fabs(plane.getNormal().y)) {
        u = glm::normalize(plane.projectPoint(vec3(1.f, 0.f, 0)) - plane.getPoint());
    } else {
        u = glm::normalize(plane.projectPoint(vec3(0.f, 1.f, 0)) - plane.getPoint());
    }
    v = glm::normalize(glm::cross(plane.getNormal(), u));
    std::vector<vec2> projectedPoints;
    projectPointsOnPlane(points, plane, u, v, projectedPoints);


    auto convexHull = geometry::convexHull2D(projectedPoints);
    OrientedBoundingBox2D boundingBox = geometry::mimumBoundingRectangle(convexHull);

    vec3 boundingBoxOrigin = plane.getPoint() + boundingBox.origin.x*u + boundingBox.origin.y*v;

    return std::make_tuple(boundingBoxOrigin,
        (boundingBox.u.x*u + boundingBox.u.y*v),
        (boundingBox.v.x*u + boundingBox.v.y*v));
}

} // namespace geometry
} // namespace

