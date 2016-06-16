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

#include "convexhull2d.h"

namespace inviwo {
namespace geometry {

std::vector<vec2> convexHull2D(std::vector<vec2> points) {
    // Sort points on increasing x-coordinate, then on y if x coordinates are equal.
    std::sort(std::begin(points), std::end(points),
        [](vec2 v1, vec2 v2) { return v1.x != v2.x ? v1.x < v2.x : v1.y < v2.y; });
    // Three points constitute a convex hull
    if (points.size() < 4) {
        return points;
    }
    // Helper function
    /**
    * \brief Determine where the point is in relation to the infinite line formed by p0 to p1.
    *
    * @param vec2 p0 First point on line
    * @param vec2 p1 Second point on line
    * @param vec2 point Point to test
    * @return float point is left of line if positive (>0), on line if ==0, right if negative
    */
    auto isPointLeftOfLine = [](vec2 p0, vec2 p1, vec2 point) {
        return (p1.x - p0.x)*(point.y - p0.y) - (point.x - p0.x)*(p1.y - p0.y);
    };


    // Get the coordinates with the minimum x and y coordinates
    int minXMinYId = 0; // Index to minimum x and y coordinate
    int minXMaxYId = 1; // Index to minimum x but largest y for equal x
    for (; minXMaxYId < points.size(); ++minXMaxYId) {
        if (points[0].x != points[minXMaxYId].x) {
            break;
        }
    }
    --minXMaxYId;
    // Special case with all x are equal 
    if (minXMaxYId == points.size() - 1) {
        std::vector<vec2> hull;
        hull.push_back(points[minXMinYId]);
        if (points[minXMaxYId].y != points[minXMinYId].y) {
            hull.push_back(points[minXMaxYId]);
        }
        hull.push_back(points[minXMinYId]);
        return hull;
    }

    int maxXMinYId = static_cast<int>(points.size()) - 1; // Index to maximum x and y coordinate
    int maxXMaxYId = static_cast<int>(points.size()) - 2; // Index to maximum x but smallest y for equal x
    for (; maxXMaxYId >= 0; --maxXMaxYId) {
        if (points[points.size() - 1].x > points[maxXMaxYId].x) {
            break;
        }
    }
    ++maxXMaxYId;

    // Compute lower hull
    std::vector<vec2> convexHull{ points[minXMinYId] };
    for (int i = minXMaxYId + 1; i <= maxXMinYId; ++i) {
        // Check if point is on or left of line from minXMinY to maxXMinY
        if (isPointLeftOfLine(points[minXMinYId], points[maxXMinYId], points[i]) >= 0 && i < maxXMinYId) {
            continue;
        }
        while (convexHull.size() >= 2) {
            // Check if point is left of the line formed by the last two elements in the hull 
            if (isPointLeftOfLine(convexHull[convexHull.size() - 2], convexHull[convexHull.size() - 1], points[i]) > 0) {
                break;
            }
            convexHull.pop_back();
        }
        convexHull.push_back(points[i]);

    }
    // Compute upper hull
    if (maxXMaxYId != maxXMinYId) {
        convexHull.push_back(points[maxXMaxYId]);
    }
    size_t bottomHull = convexHull.size() - 1;
    for (int i = maxXMaxYId; i > minXMaxYId; --i) {
        // Check if point is on or left of line from maxXMaxY to minXMaxY
        if (isPointLeftOfLine(points[maxXMaxYId], points[minXMaxYId], points[i]) >= 0 && i > minXMaxYId) {
            continue;
        }
        while (convexHull.size() - bottomHull >= 2) {
            // Check if point is left of the line formed by the last two elements in the hull 
            if (isPointLeftOfLine(convexHull[convexHull.size() - 2], convexHull[convexHull.size() - 1], points[i]) > 0) {
                break;
            }
            convexHull.pop_back();
        }
        convexHull.push_back(points[i]);

    }
    if (minXMaxYId != minXMinYId)
        convexHull.push_back(points[maxXMinYId]);

    return convexHull;
}

} // namespace geometry
} // namespace

