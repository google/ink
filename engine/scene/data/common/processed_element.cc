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

#include "ink/engine/scene/data/common/processed_element.h"

#include "third_party/absl/memory/memory.h"
#include "ink/engine/geometry/mesh/shader_type.h"
#include "ink/engine/geometry/mesh/shape_helpers.h"
#include "ink/engine/geometry/spatial/mesh_rtree.h"
#include "ink/engine/scene/data/common/mesh_serializer_provider.h"

namespace ink {

S_WARN_UNUSED_RESULT Status ProcessedElement::Create(
    ElementId id, const Stroke& stroke, ElementAttributes attributes,
    bool low_memory_mode, std::unique_ptr<ProcessedElement>* out) {
  *out = nullptr;
  SLOG(SLOG_DATA_FLOW, "MeshCount: $0", stroke.MeshCount());
  if (stroke.MeshCount() == 0) {
    return ErrorStatus(StatusCode::INVALID_ARGUMENT,
                       "Cannot create element with no meshes.");
  }
  int best_mesh_idx = -1;
  float best_coverage = -std::numeric_limits<float>::infinity();
  for (int i = 0; i < stroke.MeshCount(); ++i) {
    float coverage = -std::numeric_limits<float>::infinity();
    INK_RETURN_UNLESS(stroke.GetCoverage(i, &coverage));
    if (coverage > best_coverage) {
      best_coverage = coverage;
      best_mesh_idx = i;
    }
  }
  ASSERT(best_mesh_idx >= 0 && best_mesh_idx < stroke.MeshCount());

  Mesh mesh;
  auto mesh_reader = mesh::ReaderFor(stroke);
  INK_RETURN_UNLESS(stroke.GetMesh(*mesh_reader, best_mesh_idx, &mesh));
  *out = absl::make_unique<ProcessedElement>(id, mesh, stroke.shader_type(),
                                             low_memory_mode, attributes);
  INK_RETURN_UNLESS(InputPoints::DecompressFromProto(stroke.Proto(),
                                                     &((*out)->input_points)));
  return OkStatus();
}

ProcessedElement::ProcessedElement(ElementId id_in, const Mesh& mesh_in,
                                   ShaderType shader_type_in,
                                   bool low_memory_mode,
                                   ElementAttributes attributes_in)
    : id(id_in),
      mesh(absl::make_unique<OptimizedMesh>(shader_type_in, mesh_in)),
      obj_to_group(mesh->object_matrix),
      attributes(attributes_in) {
  // Generate the spatial index for the mesh.
  if (low_memory_mode) {
    float max_obj_coord = PackedVertList::GetMaxCoordinateForFormat(
        OptimizedMesh::VertexFormat(mesh->type));
    Mesh rect_mesh;
    MakeRectangleMesh(&rect_mesh, Rect(0, 0, max_obj_coord, max_obj_coord));
    spatial_index = std::make_shared<spatial::MeshRTree>(
        OptimizedMesh(mesh->type, rect_mesh));
  } else {
    spatial_index = std::make_shared<spatial::MeshRTree>(*mesh);
  }
}

}  // namespace ink
