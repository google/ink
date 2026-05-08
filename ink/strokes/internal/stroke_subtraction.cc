// Copyright 2026 Google LLC
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

#include "ink/strokes/internal/stroke_subtraction.h"

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "ink/geometry/affine_transform.h"
#include "ink/geometry/intersects.h"
#include "ink/geometry/mesh.h"
#include "ink/geometry/mesh_format.h"
#include "ink/geometry/mutable_mesh.h"
#include "ink/geometry/partitioned_mesh.h"
#include "ink/strokes/internal/stroke_vertex.h"

namespace ink::strokes_internal {

absl::StatusOr<PartitionedMesh> Subtract(const PartitionedMesh& mesh_a,
                                         const AffineTransform& transform_a,
                                         const PartitionedMesh& mesh_b,
                                         const AffineTransform& transform_b) {
  std::optional<AffineTransform> inv_transform_a = transform_a.Inverse();
  if (!inv_transform_a.has_value() || !transform_b.Inverse().has_value())
    return absl::InvalidArgumentError("Transforms must be invertible.");

  AffineTransform b_to_a = *inv_transform_a * transform_b;

  std::vector<PartitionedMesh::MutableMeshGroup> groups;
  groups.reserve(mesh_a.RenderGroupCount());

  std::vector<MutableMesh> group_mutable_meshes;
  group_mutable_meshes.reserve(mesh_a.RenderGroupCount());
  std::vector<StrokeVertex::CustomPackingArray> packing_arrays;
  packing_arrays.reserve(mesh_a.RenderGroupCount());

  for (uint32_t group_index = 0; group_index < mesh_a.RenderGroupCount();
       ++group_index) {
    const MeshFormat& format = mesh_a.RenderGroupFormat(group_index);
    group_mutable_meshes.push_back(MutableMesh(format));
    MutableMesh& mutable_mesh = group_mutable_meshes.back();

    for (const Mesh& mesh : mesh_a.RenderGroupMeshes(group_index)) {
      uint32_t vertex_offset = mutable_mesh.VertexCount();
      for (uint32_t vertex_index = 0; vertex_index < mesh.VertexCount();
           ++vertex_index) {
        mutable_mesh.AppendVertex(mesh.VertexPosition(vertex_index));
        for (uint32_t attribute_index = 0;
             attribute_index < format.Attributes().size(); ++attribute_index) {
          // The position is already set when the vertex is appended above.
          if (attribute_index == format.PositionAttributeIndex()) continue;
          mutable_mesh.SetFloatVertexAttribute(
              vertex_offset + vertex_index, attribute_index,
              mesh.FloatVertexAttribute(vertex_index, attribute_index));
        }
      }

      for (uint32_t triangle_index = 0; triangle_index < mesh.TriangleCount();
           ++triangle_index) {
        if (Intersects(mesh.GetTriangle(triangle_index), mesh_b, b_to_a))
          continue;
        auto indices = mesh.TriangleIndices(triangle_index);
        indices[0] += vertex_offset;
        indices[1] += vertex_offset;
        indices[2] += vertex_offset;
        mutable_mesh.AppendTriangleIndices(indices);
      }
    }

    packing_arrays.push_back(StrokeVertex::MakeCustomPackingArray(format));

    // TODO(b/509625875): Compute the mesh outline.
    groups.push_back(PartitionedMesh::MutableMeshGroup{
        .mesh = &mutable_mesh,
        .outlines = {},
        .packing_params = packing_arrays.back().Values(),
    });
  }

  return PartitionedMesh::FromMutableMeshGroups(groups);
}

}  // namespace ink::strokes_internal
