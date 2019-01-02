// Copyright 2018 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "ink/engine/geometry/algorithms/convex_hull.h"

#include "ink/engine/geometry/primitives/triangle.h"
#include "ink/engine/geometry/primitives/vector_utils.h"
#include "ink/engine/util/dbg/errors.h"

namespace ink {
namespace geometry {
namespace {

// Prune interior points using the Aki-Toussaint heuristic
// (go/wiki/Convex_hull_algorithms#Akl%E2%80%93Toussaint_heuristic).
std::vector<glm::vec2> Prune(const std::vector<glm::vec2> &points) {
  glm::vec2 max_x_point = points.front();
  glm::vec2 max_y_point = points.front();
  glm::vec2 min_x_point = points.front();
  glm::vec2 min_y_point = points.front();
  for (int i = 1; i < points.size(); ++i) {
    if (points[i].x > max_x_point.x) {
      max_x_point = points[i];
    } else if (points[i].x < min_x_point.x) {
      min_x_point = points[i];
    }

    if (points[i].y > max_y_point.y) {
      max_y_point = points[i];
    } else if (points[i].y < min_y_point.y) {
      min_y_point = points[i];
    }
  }

  // Points that are inside of the quadrilateral formed by the extrema will not
  // be in the convex hull, and can be eliminated.
  Triangle upper_triangle(min_x_point, max_x_point, max_y_point);
  Triangle lower_triangle(min_x_point, min_y_point, max_x_point);
  std::vector<glm::vec2> pruned_points;
  pruned_points.reserve(points.size());
  for (const auto &p : points) {
    if (!upper_triangle.Contains(p) && !lower_triangle.Contains(p))
      pruned_points.push_back(p);
  }

  return pruned_points;
}
}  // namespace

std::vector<glm::vec2> ConvexHull(const std::vector<glm::vec2> &points) {
  if (points.size() < 2) return points;

  auto pruned_points = points.size() > 500 ? Prune(points) : points;

  // Find the point with the lowest y-coordinate, selecting the lowest
  // x-coordinate in the case of ties.
  glm::vec2 start_point = *std::min_element(
      points.begin(), points.end(), [](glm::vec2 lhs, glm::vec2 rhs) {
        return lhs.y < rhs.y || (lhs.y == rhs.y && lhs.x < rhs.x);
      });

  // Sort the remaining points by their angle from the start point, placing the
  // closest first in the case of ties.
  std::vector<glm::vec2> sorted_points;
  sorted_points.reserve(points.size());
  std::copy_if(pruned_points.begin(), pruned_points.end(),
               std::back_inserter(sorted_points),
               [start_point](glm::vec2 v) { return v != start_point; });
  std::sort(sorted_points.begin(), sorted_points.end(),
            [start_point](glm::vec2 lhs, glm::vec2 rhs) {
              glm::vec2 lhs_vec = lhs - start_point;
              glm::vec2 rhs_vec = rhs - start_point;
              float det = Determinant(lhs_vec, rhs_vec);
              return det > 0 ||
                     (det == 0 && std::abs(lhs_vec.x) < std::abs(rhs_vec.x));
            });

  // Add the sorted points to the hull, removing any that form concavities.
  std::vector<glm::vec2> hull;
  hull.reserve(sorted_points.size());
  hull.push_back(start_point);
  for (const auto &point : sorted_points) {
    while (hull.size() >= 2 && Determinant(hull.back() - hull[hull.size() - 2],
                                           point - hull.back()) <= 0)
      hull.pop_back();
    hull.push_back(point);
  }

  return hull;
}

}  // namespace geometry
}  // namespace ink
