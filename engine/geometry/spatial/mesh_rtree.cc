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

#include "ink/engine/geometry/spatial/mesh_rtree.h"

#include <iterator>
#include <utility>
#include <vector>

#include "third_party/absl/memory/memory.h"
#include "ink/engine/geometry/algorithms/convex_hull.h"
#include "ink/engine/geometry/algorithms/envelope.h"
#include "ink/engine/geometry/algorithms/intersect.h"
#include "ink/engine/geometry/algorithms/simplify.h"
#include "ink/engine/geometry/algorithms/transform.h"
#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/geometry/primitives/triangle.h"
#include "ink/engine/geometry/spatial/rtree_utils.h"
#include "ink/engine/util/funcs/utils.h"

namespace ink {
namespace spatial {

using glm::vec2;

using geometry::Transform;
using geometry::Triangle;

MeshRTree::MeshRTree(const OptimizedMesh& mesh) : MeshRTree(mesh.ToMesh()) {}

MeshRTree::MeshRTree(const Mesh& unpacked_mesh) {
  rtree_ = MakeRTreeFromMeshTriangles(unpacked_mesh);

  std::vector<vec2> vertices;
  vertices.reserve(unpacked_mesh.verts.size());
  for (const auto& v : unpacked_mesh.verts) vertices.push_back(v.position);
  convex_hull_ = geometry::ConvexHull(vertices);
  Rect mbr = geometry::Envelope(convex_hull_);
  mbr_offset_dist_ = .01 * std::max(mbr.Width(), mbr.Height());
  std::vector<glm::vec2> simplified;
  simplified.reserve(convex_hull_.size());
  geometry::Simplify(convex_hull_.begin(), convex_hull_.end(), mbr_offset_dist_,
                     std::back_inserter(simplified));
  if (convex_hull_.size() != simplified.size()) {
    convex_hull_ = simplified;
  } else {
    mbr_offset_dist_ = 0;
  }
}

Rect MeshRTree::Mbr(const glm::mat4& object_to_world) const {
  if (cached_mbr_ && cached_mbr_->first == object_to_world) {
    return cached_mbr_->second;
  }
  vec2 origin = Transform(vec2(0, 0), object_to_world);
  vec2 x_axis = Transform(vec2(mbr_offset_dist_, 0.0f), object_to_world);
  vec2 y_axis = Transform(vec2(0.0f, mbr_offset_dist_), object_to_world);
  vec2 offset = glm::max(glm::abs(x_axis - origin), glm::abs(y_axis - origin));
  std::vector<glm::vec2> transformed_convex_hull;
  transformed_convex_hull.reserve(convex_hull_.size());
  Transform(convex_hull_.begin(), convex_hull_.end(), object_to_world,
            std::back_inserter(transformed_convex_hull));
  Rect mbr = geometry::Envelope(transformed_convex_hull).Inset(-offset);

  cached_mbr_ =
      absl::make_unique<std::pair<glm::mat4, Rect>>(object_to_world, mbr);
  return mbr;
}

bool MeshRTree::Intersects(const Rect& region,
                           const glm::mat4& region_to_object) const {
  const Rect& local_region_mbr = Transform(region, region_to_object);
  if (!geometry::Intersects(rtree_->Bounds(), local_region_mbr)) {
    return false;
  }
  vec2 point0 = Transform(region.Leftbottom(), region_to_object);
  vec2 point1 = Transform(region.Rightbottom(), region_to_object);
  vec2 point2 = Transform(region.Righttop(), region_to_object);
  vec2 point3 = Transform(region.Lefttop(), region_to_object);
  Triangle lower_tri(point0, point1, point3);
  Triangle upper_tri(point1, point2, point3);
  return rtree_
      ->FindAny(local_region_mbr,
                [&lower_tri, &upper_tri](const Triangle& t) {
                  return geometry::Intersects(lower_tri, t) ||
                         geometry::Intersects(upper_tri, t);
                })
      .has_value();
}

absl::optional<Rect> MeshRTree::Intersection(
    const Rect& region, const glm::mat4& region_to_object) const {
  // The code below looks similar but is subtly different from the
  // the code in Intersects(). Once we have the tris-to-test set up,
  // we want to always return false from the FindAny predicate, to
  // ensure we see all tris that are in the rect. Intersects(...)
  // returns early.
  const Rect& local_region_mbr = Transform(region, region_to_object);
  if (!geometry::Intersects(rtree_->Bounds(), local_region_mbr)) {
    return absl::nullopt;
  }
  vec2 point0 = Transform(region.Leftbottom(), region_to_object);
  vec2 point1 = Transform(region.Rightbottom(), region_to_object);
  vec2 point2 = Transform(region.Righttop(), region_to_object);
  vec2 point3 = Transform(region.Lefttop(), region_to_object);
  Triangle lower_tri(point0, point1, point3);
  Triangle upper_tri(point1, point2, point3);
  absl::optional<Rect> intersected_tri_mbr;
  rtree_->FindAny(local_region_mbr, [&lower_tri, &upper_tri,
                                     &intersected_tri_mbr](const Triangle& t) {
    if (geometry::Intersects(lower_tri, t) ||
        geometry::Intersects(upper_tri, t)) {
      if (!intersected_tri_mbr) {
        intersected_tri_mbr = Rect::CreateAtPoint(t[0],
                                                  /* width= */ 0,
                                                  /* height=0*/ 0);
      }
      for (int i = 0; i < 3; ++i) {
        intersected_tri_mbr->InplaceJoin(t[i]);
      }
    }
    return false;
  });
  if (!intersected_tri_mbr) {
    return absl::nullopt;
  }
  Rect intersection;
  if (!geometry::Intersection(local_region_mbr, *intersected_tri_mbr,
                              &intersection)) {
    return absl::nullopt;
  }
  return intersection;
}

Mesh MeshRTree::DebugMesh() const {
  Mesh mesh;
  mesh.verts.reserve(3 * rtree_->Size());
  std::vector<Triangle> triangles;
  triangles.reserve(rtree_->Size());
  rtree_->FindAll(rtree_->Bounds(), std::back_inserter(triangles));
  for (const auto& t : triangles) {
    mesh.verts.emplace_back(t[0], glm::vec4(1, 0, 0, .5));
    mesh.verts.emplace_back(t[1], glm::vec4(1, 0, 0, .5));
    mesh.verts.emplace_back(t[2], glm::vec4(1, 0, 0, .5));
  }
  mesh.GenIndex();
  return mesh;
}

}  // namespace spatial
}  // namespace ink
