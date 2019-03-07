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

#include "ink/engine/geometry/mesh/mesh_splitter.h"

#include <iterator>
#include <memory>
#include <vector>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/algorithms/boolean_operation.h"
#include "ink/engine/geometry/algorithms/envelope.h"
#include "ink/engine/geometry/algorithms/transform.h"
#include "ink/engine/geometry/mesh/vertex.h"
#include "ink/engine/geometry/primitives/polygon.h"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/geometry/primitives/triangle.h"
#include "ink/engine/geometry/spatial/rtree_utils.h"
#include "ink/engine/geometry/tess/tessellator.h"
#include "ink/engine/util/dbg/log.h"

namespace ink {
namespace {

using geometry::BarycentricInterpolate;
using geometry::Polygon;
using geometry::Triangle;

// This returns a copy of the cutting mesh, transformed to the
// object-coordinate-system of the base mesh.
Mesh TransformCuttingMesh(const Mesh &cutting_mesh,
                          const glm::mat4 &base_object_matrix) {
  Mesh transformed_mesh = cutting_mesh;

  auto cutting_to_base =
      glm::inverse(base_object_matrix) * cutting_mesh.object_matrix;
  for (auto &v : transformed_mesh.verts)
    v.position = geometry::Transform(v.position, cutting_to_base);
  return transformed_mesh;
}

}  // namespace

MeshSplitter::MeshSplitter(const OptimizedMesh &base_mesh)
    : base_mesh_(base_mesh), is_base_mesh_changed_(false) {}

void MeshSplitter::Split(const Mesh &cutting_mesh) {
  if (!rtree_) InitializeRTree();

  auto transformed_mesh =
      TransformCuttingMesh(cutting_mesh, base_mesh_.object_matrix);

  for (int i = 0; i < transformed_mesh.NumberOfTriangles(); ++i) {
    Triangle cutting_triangle = transformed_mesh.GetTriangle(i);

    if (cutting_triangle.SignedArea() == 0) continue;
    ASSERT(cutting_triangle.SignedArea() > 0);

    Rect cutting_mbr = geometry::Envelope(cutting_triangle);
    std::vector<IndexedTriangle> triangles_to_cut;
    rtree_->FindAll(cutting_mbr, std::back_inserter(triangles_to_cut));
    rtree_->RemoveAll(cutting_mbr);

    for (const auto base_triangle : triangles_to_cut) {
      if (base_triangle.triangle.SignedArea() == 0) continue;

      ASSERT(base_triangle.triangle.SignedArea() > 0);
      std::vector<Polygon> difference =
          geometry::Difference(Polygon(base_triangle.triangle.Points()),
                               Polygon(cutting_triangle.Points()));

      // If the difference is the same as the original triangle, just re-insert
      // it,
      if (difference.size() == 1 && difference[0].Size() == 3 &&
          (difference[0].Points() == base_triangle.triangle.Points() ||
           difference[0].CircularShift(1).Points() ==
               base_triangle.triangle.Points() ||
           difference[0].CircularShift(2).Points() ==
               base_triangle.triangle.Points())) {
        rtree_->Insert(base_triangle);
        continue;
      }


      is_base_mesh_changed_ = true;

      if (difference.empty()) continue;

      Tessellator tessellator;
      if (tessellator.Tessellate(difference) && tessellator.HasMesh()) {
        tessellator.mesh_.NormalizeTriangleOrientation();
        for (int j = 0; j < tessellator.mesh_.NumberOfTriangles(); ++j) {
          IndexedTriangle t{tessellator.mesh_.GetTriangle(j),
                            base_triangle.original_index};
          if (!t.triangle.IsDegenerate()) rtree_->Insert(t);
        }
      } else {
        SLOG(SLOG_WARNING,
             "Failed to tessellate polygon difference ($0). Re-inserting "
             "original triangle.",
             difference);
        rtree_->Insert(base_triangle);
      }
    }
  }
}

bool MeshSplitter::GetResult(Mesh *result_mesh) const {
  if (!IsMeshChanged()) return false;

  std::vector<IndexedTriangle> result_triangles;
  result_triangles.reserve(rtree_->Size());
  rtree_->FindAll(rtree_->Bounds(), std::back_inserter(result_triangles));
  result_mesh->verts.reserve(3 * result_triangles.size());

  // We maintain an R-Tree of vertex positions, and the indices of the vertex in
  // the result mesh, so that we don't add duplicate vertices.
  struct IndexedVertex {
    int index;
    glm::vec2 position{0, 0};

    IndexedVertex(int index_in, glm::vec2 position_in)
        : index(index_in), position(position_in) {}
  };
  spatial::RTree<IndexedVertex> vertex_rtree([](const IndexedVertex &v) {
    return Rect::CreateAtPoint(v.position, 0, 0);
  });

  Mesh unpacked_mesh = base_mesh_.ToMesh();
  for (const auto &t : result_triangles) {
    if (t.triangle.IsDegenerate()) continue;

    auto original_triangle = unpacked_mesh.GetTriangle(t.original_index);
    ASSERT(!original_triangle.IsDegenerate());
    const auto &vertex0 = unpacked_mesh.GetVertex(t.original_index, 0);
    const auto &vertex1 = unpacked_mesh.GetVertex(t.original_index, 1);
    const auto &vertex2 = unpacked_mesh.GetVertex(t.original_index, 2);
    for (int i = 0; i < 3; ++i) {
      // Construct the new vertex by interpolating over the original triangle.
      Vertex v(t.triangle[i]);
      auto barycentric = original_triangle.ConvertToBarycentric(t.triangle[i]);
      v.color = BarycentricInterpolate(barycentric, vertex0.color,
                                       vertex1.color, vertex2.color);
      v.texture_coords = BarycentricInterpolate(
          barycentric, vertex0.texture_coords, vertex1.texture_coords,
          vertex2.texture_coords);

      // Search to see if we have already created this exact vertex.
      auto existing_vertex =
          vertex_rtree.FindAny(Rect::CreateAtPoint(v.position, 0, 0),
                               [&result_mesh, &v](const IndexedVertex &iv) {
                                 return result_mesh->verts[iv.index] == v;
                               });
      if (existing_vertex.has_value()) {
        // We found a match, use that vertex.
        result_mesh->idx.emplace_back(existing_vertex->index);
      } else {
        // We didn't find a match, add the new vertex to both the mesh and the
        // vertex R-Tree.
        int new_index = result_mesh->verts.size();
        result_mesh->verts.push_back(v);
        result_mesh->idx.push_back(new_index);
        vertex_rtree.Insert({new_index, v.position});
      }
    }
  }
  result_mesh->object_matrix = base_mesh_.object_matrix;
  if (base_mesh_.texture != nullptr)
    result_mesh->texture = absl::make_unique<TextureInfo>(*base_mesh_.texture);
  return true;
}

void MeshSplitter::InitializeRTree() {
  Mesh unpacked_mesh = base_mesh_.ToMesh();
  rtree_ = spatial::MakeRTreeFromMeshTriangles<IndexedTriangle>(
      unpacked_mesh,
      [](const Mesh &m, int i) {
        return IndexedTriangle{m.GetTriangle(i), i};
      },
      [](const IndexedTriangle &t) { return geometry::Envelope(t.triangle); },
      [](const Mesh &m, int i) { return !m.GetTriangle(i).IsDegenerate(); });
}

}  // namespace ink
