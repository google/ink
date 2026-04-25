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

#include "ink/geometry/internal/jni/partitioned_mesh_native.h"

#include <cstddef>
#include <cstdint>
#include <optional>

#include "absl/log/absl_check.h"
#include "absl/status/statusor.h"
#include "absl/types/span.h"
#include "ink/geometry/affine_transform.h"
#include "ink/geometry/angle.h"
#include "ink/geometry/internal/jni/mesh_format_native_helper.h"
#include "ink/geometry/internal/jni/partitioned_mesh_native_helper.h"
#include "ink/geometry/mesh.h"
#include "ink/geometry/mesh_format.h"
#include "ink/geometry/mesh_index_types.h"
#include "ink/geometry/partitioned_mesh.h"
#include "ink/geometry/point.h"
#include "ink/geometry/quad.h"
#include "ink/geometry/rect.h"
#include "ink/geometry/triangle.h"

using ::ink::AffineTransform;
using ::ink::Angle;
using ::ink::Mesh;
using ::ink::MeshFormat;
using ::ink::PartitionedMesh;
using ::ink::Point;
using ::ink::Quad;
using ::ink::Rect;
using ::ink::Triangle;
using ::ink::VertexIndexPair;
using ::ink::native::CastToPartitionedMesh;
using ::ink::native::DeleteNativePartitionedMesh;
using ::ink::native::NewNativeMeshFormat;
using ::ink::native::NewNativePartitionedMesh;

extern "C" {

int64_t PartitionedMeshNative_createEmptyForTesting() {
  return NewNativePartitionedMesh();
}

int64_t PartitionedMeshNative_createFromTriangleForTesting(
    float triangle_p0_x, float triangle_p0_y, float triangle_p1_x,
    float triangle_p1_y, float triangle_p2_x, float triangle_p2_y) {
  absl::StatusOr<Mesh> mesh =
      Mesh::Create(MeshFormat(),
                   {{triangle_p0_x, triangle_p1_x, triangle_p2_x},
                    {triangle_p0_y, triangle_p1_y, triangle_p2_y}},
                   {0, 1, 2});
  ABSL_CHECK(mesh.ok());
  absl::StatusOr<PartitionedMesh> partitioned_mesh =
      PartitionedMesh::FromMeshes({*mesh}, {{{0, 0}, {0, 1}, {0, 2}}});
  ABSL_CHECK(partitioned_mesh.ok());
  return NewNativePartitionedMesh(*partitioned_mesh);
}

void PartitionedMeshNative_free(int64_t native_ptr) {
  DeleteNativePartitionedMesh(native_ptr);
}

int PartitionedMeshNative_getRenderGroupCount(int64_t native_ptr) {
  return CastToPartitionedMesh(native_ptr).RenderGroupCount();
}

void PartitionedMeshNative_fillRenderGroupMeshPointers(int64_t native_ptr,
                                                       int group_index,
                                                       int64_t* out_pointers) {
  absl::Span<const Mesh> meshes =
      CastToPartitionedMesh(native_ptr).RenderGroupMeshes(group_index);
  for (size_t i = 0; i < meshes.size(); ++i) {
    out_pointers[i] = reinterpret_cast<int64_t>(&meshes[i]);
  }
}

int64_t PartitionedMeshNative_newCopyOfRenderGroupFormat(int64_t native_ptr,
                                                         int group_index) {
  return NewNativeMeshFormat(
      CastToPartitionedMesh(native_ptr).RenderGroupFormat(group_index));
}

int PartitionedMeshNative_getOutlineCount(int64_t native_ptr, int group_index) {
  return CastToPartitionedMesh(native_ptr).OutlineCount(group_index);
}

int PartitionedMeshNative_getOutlineVertexCount(int64_t native_ptr,
                                                int group_index,
                                                int outline_index) {
  return CastToPartitionedMesh(native_ptr)
      .Outline(group_index, outline_index)
      .size();
}

PartitionedMeshNative_Vec PartitionedMeshNative_getOutlineVertexPosition(
    int64_t native_ptr, int group_index, int outline_index,
    int outline_vertex_index) {
  const PartitionedMesh& partitioned_mesh = CastToPartitionedMesh(native_ptr);
  VertexIndexPair pair = partitioned_mesh.Outline(
      group_index, outline_index)[outline_vertex_index];
  const Mesh& mesh =
      partitioned_mesh.RenderGroupMeshes(group_index)[pair.mesh_index];
  const Point& position = mesh.VertexPosition(pair.vertex_index);
  return {position.x, position.y};
}

bool PartitionedMeshNative_isEmpty(int64_t native_ptr) {
  return CastToPartitionedMesh(native_ptr).Bounds().IsEmpty();
}

PartitionedMeshNative_Box PartitionedMeshNative_getBounds(int64_t native_ptr) {
  const std::optional<Rect>& bounds_rect =
      CastToPartitionedMesh(native_ptr).Bounds().AsRect();
  ABSL_CHECK(bounds_rect.has_value());
  return {bounds_rect->XMin(), bounds_rect->YMin(), bounds_rect->XMax(),
          bounds_rect->YMax()};
}

float PartitionedMeshNative_triangleCoverage(
    int64_t native_ptr, float triangle_p0_x, float triangle_p0_y,
    float triangle_p1_x, float triangle_p1_y, float triangle_p2_x,
    float triangle_p2_y, float a, float b, float c, float d, float e, float f) {
  return CastToPartitionedMesh(native_ptr)
      .Coverage(Triangle{{triangle_p0_x, triangle_p0_y},
                         {triangle_p1_x, triangle_p1_y},
                         {triangle_p2_x, triangle_p2_y}},
                AffineTransform(a, b, c, d, e, f));
}

float PartitionedMeshNative_boxCoverage(int64_t native_ptr, float x_min,
                                        float y_min, float x_max, float y_max,
                                        float a, float b, float c, float d,
                                        float e, float f) {
  return CastToPartitionedMesh(native_ptr)
      .Coverage(Rect::FromTwoPoints({x_min, y_min}, {x_max, y_max}),
                AffineTransform(a, b, c, d, e, f));
}

float PartitionedMeshNative_parallelogramCoverage(
    int64_t native_ptr, float center_x, float center_y, float width,
    float height, float angle_radian, float shear_factor, float a, float b,
    float c, float d, float e, float f) {
  return CastToPartitionedMesh(native_ptr)
      .Coverage(Quad::FromCenterDimensionsRotationAndSkew(
                    {center_x, center_y}, width, height,
                    Angle::Radians(angle_radian), shear_factor),
                AffineTransform(a, b, c, d, e, f));
}

float PartitionedMeshNative_partitionedMeshCoverage(int64_t native_ptr,
                                                    int64_t other_ptr, float a,
                                                    float b, float c, float d,
                                                    float e, float f) {
  return CastToPartitionedMesh(native_ptr)
      .Coverage(CastToPartitionedMesh(other_ptr),
                AffineTransform(a, b, c, d, e, f));
}

bool PartitionedMeshNative_triangleCoverageIsGreaterThan(
    int64_t native_ptr, float triangle_p0_x, float triangle_p0_y,
    float triangle_p1_x, float triangle_p1_y, float triangle_p2_x,
    float triangle_p2_y, float threshold, float a, float b, float c, float d,
    float e, float f) {
  return CastToPartitionedMesh(native_ptr)
      .CoverageIsGreaterThan(Triangle{{triangle_p0_x, triangle_p0_y},
                                      {triangle_p1_x, triangle_p1_y},
                                      {triangle_p2_x, triangle_p2_y}},
                             threshold, AffineTransform(a, b, c, d, e, f));
}

bool PartitionedMeshNative_boxCoverageIsGreaterThan(
    int64_t native_ptr, float x_min, float y_min, float x_max, float y_max,
    float threshold, float a, float b, float c, float d, float e, float f) {
  return CastToPartitionedMesh(native_ptr)
      .CoverageIsGreaterThan(
          Rect::FromTwoPoints({x_min, y_min}, {x_max, y_max}), threshold,
          AffineTransform(a, b, c, d, e, f));
}

bool PartitionedMeshNative_parallelogramCoverageIsGreaterThan(
    int64_t native_ptr, float center_x, float center_y, float width,
    float height, float angle_radian, float shear_factor, float threshold,
    float a, float b, float c, float d, float e, float f) {
  return CastToPartitionedMesh(native_ptr)
      .CoverageIsGreaterThan(Quad::FromCenterDimensionsRotationAndSkew(
                                 {center_x, center_y}, width, height,
                                 Angle::Radians(angle_radian), shear_factor),
                             threshold, AffineTransform(a, b, c, d, e, f));
}

bool PartitionedMeshNative_partitionedMeshCoverageIsGreaterThan(
    int64_t native_ptr, int64_t other_ptr, float threshold, float a, float b,
    float c, float d, float e, float f) {
  return CastToPartitionedMesh(native_ptr)
      .CoverageIsGreaterThan(CastToPartitionedMesh(other_ptr), threshold,
                             AffineTransform(a, b, c, d, e, f));
}

void PartitionedMeshNative_initializeSpatialIndex(int64_t native_ptr) {
  CastToPartitionedMesh(native_ptr).InitializeSpatialIndex();
}

bool PartitionedMeshNative_isSpatialIndexInitialized(int64_t native_ptr) {
  return CastToPartitionedMesh(native_ptr).IsSpatialIndexInitialized();
}

int PartitionedMeshNative_getRenderGroupMeshCount(int64_t native_ptr,
                                                  int group_index) {
  return CastToPartitionedMesh(native_ptr)
      .RenderGroupMeshes(group_index)
      .size();
}

}  // extern "C"
