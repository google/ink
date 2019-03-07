/*
 * Copyright 2018 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef INK_ENGINE_GEOMETRY_SPATIAL_RTREE_UTILS_H_
#define INK_ENGINE_GEOMETRY_SPATIAL_RTREE_UTILS_H_

#include <memory>

#include "third_party/absl/memory/memory.h"
#include "ink/engine/geometry/algorithms/envelope.h"
#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/geometry/primitives/triangle.h"
#include "ink/engine/geometry/spatial/rtree.h"

namespace ink {
namespace spatial {

// Creates an R-Tree by constructing a DataType instance for each triangle in
// the mesh, then bulk-loading them.
// The function data_factory is used to construct the DataType instances.
// The bounds_function is expected to return the envelope of a DataType
// instance.
// The optional function triangle_filter can be used to specify whether a
// triangle should be included in the R-Tree; if not specified, all triangles
// will be included.
// WARNING: The BoundsFunction is saved in the R-Tree; be sure that any
// references or pointers that it holds remain valid for the lifetime of the
// R-Tree.
template <typename DataType>
std::unique_ptr<RTree<DataType>> MakeRTreeFromMeshTriangles(
    const Mesh &mesh,
    std::function<DataType(const Mesh &mesh, int triangle_index)> data_factory,
    typename RTree<DataType>::BoundsFunction bounds_function,
    std::function<bool(const Mesh &mesh, int triangle_index)> triangle_filter =
        nullptr) {
  std::vector<DataType> data;
  ASSERT(mesh.idx.size() % 3 == 0);
  int n_triangles = mesh.NumberOfTriangles();
  data.reserve(n_triangles);
  for (int i = 0; i < n_triangles; ++i) {
    if (!triangle_filter || triangle_filter(mesh, i))
      data.emplace_back(data_factory(mesh, i));
  }

  return absl::make_unique<RTree<DataType>>(data.begin(), data.end(),
                                            bounds_function);
}

// This convenience overload creates an R-Tree containing geometry::Triangles.
inline std::unique_ptr<RTree<geometry::Triangle>> MakeRTreeFromMeshTriangles(
    const Mesh &mesh) {
  using geometry::Triangle;
  return MakeRTreeFromMeshTriangles<Triangle>(
      mesh,
      [](const Mesh &mesh, int triangle_index) {
        return mesh.GetTriangle(triangle_index);
      },
      [](const Triangle &t) { return geometry::Envelope(t); });
}

}  // namespace spatial
}  // namespace ink

#endif  // INK_ENGINE_GEOMETRY_SPATIAL_RTREE_UTILS_H_
