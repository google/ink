// Copyright 2024-2025 Google LLC
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

#include "ink/storage/mesh.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "absl/log/absl_check.h"
#include "absl/status/statusor.h"
#include "absl/types/span.h"
#include "ink/geometry/internal/mesh_packing.h"
#include "ink/geometry/mesh.h"
#include "ink/geometry/mesh_format.h"
#include "ink/geometry/mesh_packing_types.h"
#include "ink/geometry/point.h"
#include "ink/storage/mesh_format.h"
#include "ink/storage/numeric_run.h"
#include "ink/storage/proto/coded.pb.h"
#include "ink/types/iterator_range.h"
#include "ink/types/small_array.h"

namespace ink {
namespace {

using ::ink::proto::CodedMesh;
using ::ink::proto::CodedNumericRun;

}  // namespace

namespace {

void InitCodedMeshPositions(
    uint32_t vertex_count,
    const MeshAttributeCodingParams& position_coding_params,
    CodedMesh& coded_mesh) {
  ABSL_DCHECK_GT(vertex_count, 0);
  ABSL_DCHECK_EQ(position_coding_params.components.Size(), 2);
  CodedNumericRun* x_stroke_space = coded_mesh.mutable_x_stroke_space();
  CodedNumericRun* y_stroke_space = coded_mesh.mutable_y_stroke_space();
  x_stroke_space->mutable_deltas()->Clear();
  y_stroke_space->mutable_deltas()->Clear();
  x_stroke_space->mutable_deltas()->Reserve(vertex_count);
  y_stroke_space->mutable_deltas()->Reserve(vertex_count);
  x_stroke_space->set_offset(position_coding_params.components[0].offset);
  y_stroke_space->set_offset(position_coding_params.components[1].offset);
  x_stroke_space->set_scale(position_coding_params.components[0].scale);
  y_stroke_space->set_scale(position_coding_params.components[1].scale);
}

void EncodePackedMeshPositions(const Mesh& mesh, CodedMesh& coded_mesh) {
  uint32_t vertex_count = mesh.VertexCount();
  uint32_t position_attribute_index = mesh.VertexPositionAttributeIndex();
  const MeshAttributeCodingParams& position_coding_params =
      mesh.VertexAttributeUnpackingParams(position_attribute_index);
  InitCodedMeshPositions(vertex_count, position_coding_params, coded_mesh);

  CodedNumericRun* x_stroke_space = coded_mesh.mutable_x_stroke_space();
  CodedNumericRun* y_stroke_space = coded_mesh.mutable_y_stroke_space();
  int prev_x = 0;
  int prev_y = 0;
  for (uint32_t i = 0; i < vertex_count; ++i) {
    SmallArray<uint32_t, 4> packed_integers =
        mesh.PackedIntegersForFloatVertexAttribute(i, position_attribute_index);
    ABSL_DCHECK_EQ(packed_integers.Size(), 2);
    int next_x = packed_integers[0];
    int next_y = packed_integers[1];
    x_stroke_space->add_deltas(next_x - prev_x);
    y_stroke_space->add_deltas(next_y - prev_y);
    prev_x = next_x;
    prev_y = next_y;
  }
}

void EncodeUnpackedMeshPositions(const Mesh& mesh, CodedMesh& coded_mesh) {
  // TODO: b/294865374 - Handle flipped-triangle correction.  Possibly this
  // function could work by (1) creating an equivalent MeshFormat using only
  // packed attributes, (2) creating a new MutableMesh with the same data as the
  // Mesh, but with the new format, (3) calling MutableMesh::AsMeshes to perform
  // the packing and flipped-triangle correction, and (4) calling
  // EncodePackedMeshPositions().
  uint32_t vertex_count = mesh.VertexCount();
  ABSL_DCHECK_GT(vertex_count, 0);
  uint32_t position_attribute_index = mesh.VertexPositionAttributeIndex();
  std::optional<MeshAttributeBounds> position_bounds =
      mesh.AttributeBounds(position_attribute_index);
  ABSL_CHECK(position_bounds.has_value());  // we know mesh is non-empty
  absl::StatusOr<MeshAttributeCodingParams> position_coding_params =
      mesh_internal::ComputeCodingParams(
          MeshFormat::AttributeType::kFloat2PackedIn1Float, *position_bounds);
  ABSL_CHECK_OK(position_coding_params);  // Mesh type guarantees valid bounds
  InitCodedMeshPositions(vertex_count, *position_coding_params, coded_mesh);

  CodedNumericRun* x_stroke_space = coded_mesh.mutable_x_stroke_space();
  CodedNumericRun* y_stroke_space = coded_mesh.mutable_y_stroke_space();
  int prev_x = 0;
  int prev_y = 0;
  for (uint32_t i = 0; i < vertex_count; ++i) {
    Point position = mesh.VertexPosition(i);
    int next_x = mesh_internal::PackSingleFloat(
        position_coding_params->components[0], position.x);
    int next_y = mesh_internal::PackSingleFloat(
        position_coding_params->components[1], position.y);
    x_stroke_space->add_deltas(next_x - prev_x);
    y_stroke_space->add_deltas(next_y - prev_y);
    prev_x = next_x;
    prev_y = next_y;
  }
}

void EncodeMeshTriangleIndex(const Mesh& mesh,
                             CodedNumericRun& triangle_indices) {
  const uint32_t triangle_count = mesh.TriangleCount();
  triangle_indices.mutable_deltas()->Clear();
  triangle_indices.mutable_deltas()->Reserve(triangle_count * 3);
  int prev_triangle_index = 0;
  for (size_t i = 0; i < triangle_count; ++i) {
    for (int j = 0; j < 3; ++j) {
      uint32_t triangle_index = mesh.TriangleIndices(i)[j];
      triangle_indices.add_deltas(triangle_index - prev_triangle_index);
      prev_triangle_index = triangle_index;
    }
  }
}

}  // namespace

void EncodeMeshOmittingFormat(const Mesh& mesh,
                              ink::proto::CodedMesh& coded_mesh) {
  coded_mesh.clear_format();

  const uint32_t vertex_count = mesh.VertexCount();
  if (vertex_count == 0) {
    coded_mesh.clear_triangle_index();
    coded_mesh.clear_x_stroke_space();
    coded_mesh.clear_y_stroke_space();
    return;
  }

  const MeshFormat& format = mesh.Format();
  uint32_t position_attribute_index = format.PositionAttributeIndex();
  MeshFormat::AttributeType position_attribute_type =
      format.Attributes()[position_attribute_index].type;
  if (MeshFormat::IsUnpackedType(position_attribute_type)) {
    EncodeUnpackedMeshPositions(mesh, coded_mesh);
  } else {
    EncodePackedMeshPositions(mesh, coded_mesh);
  }

  EncodeMeshTriangleIndex(mesh, *coded_mesh.mutable_triangle_index());
}

void EncodeMesh(const Mesh& mesh, CodedMesh& coded_mesh) {
  EncodeMeshOmittingFormat(mesh, coded_mesh);
  EncodeMeshFormat(mesh.Format(), *coded_mesh.mutable_format());
}

absl::StatusOr<Mesh> DecodeMesh(const ink::proto::CodedMesh& coded_mesh) {
  absl::StatusOr<MeshFormat> format = MeshFormat();
  if (coded_mesh.has_format()) {
    // TODO: b/295166196 - `IndexFormat`s will be removed soon; until then, just
    // assume a default `IndexFormat` here.
    format =
        DecodeMeshFormat(coded_mesh.format(),
                         MeshFormat::IndexFormat::k32BitUnpacked16BitPacked);
  }
  if (!format.ok()) return format.status();

  return DecodeMeshUsingFormat(*format, coded_mesh);
}

absl::StatusOr<Mesh> DecodeMeshUsingFormat(
    const MeshFormat& format, const ink::proto::CodedMesh& coded_mesh) {
  absl::StatusOr<iterator_range<CodedNumericRunIterator<float>>>
      x_stroke_space = DecodeFloatNumericRun(coded_mesh.x_stroke_space());
  if (!x_stroke_space.ok()) {
    return x_stroke_space.status();
  }
  absl::StatusOr<iterator_range<CodedNumericRunIterator<float>>>
      y_stroke_space = DecodeFloatNumericRun(coded_mesh.y_stroke_space());
  if (!y_stroke_space.ok()) {
    return y_stroke_space.status();
  }
  absl::StatusOr<iterator_range<CodedNumericRunIterator<int32_t>>>
      triangle_range = DecodeIntNumericRun(coded_mesh.triangle_index());
  if (!triangle_range.ok()) {
    return triangle_range.status();
  }

  std::vector<float> x(x_stroke_space->begin(), x_stroke_space->end());
  std::vector<float> y(y_stroke_space->begin(), y_stroke_space->end());
  std::vector<uint32_t> triangle_indices(triangle_range->begin(),
                                         triangle_range->end());

  return ink::Mesh::Create(format, {x, y}, triangle_indices);
}

}  // namespace ink
