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

#include "ink/geometry/internal/jni/mesh_native.h"

#include <cstddef>
#include <cstdint>
#include <optional>

#include "absl/log/absl_check.h"
#include "absl/types/span.h"
#include "ink/geometry/internal/jni/mesh_format_native_helper.h"
#include "ink/geometry/internal/jni/mesh_native_helper.h"
#include "ink/geometry/mesh.h"
#include "ink/geometry/mesh_format.h"
#include "ink/geometry/mesh_packing_types.h"
#include "ink/geometry/point.h"
#include "ink/geometry/rect.h"

using ::ink::Point;
using ::ink::Rect;
using ::ink::native::CastToMesh;
using ::ink::native::DeleteNativeMesh;
using ::ink::native::NewNativeMesh;
using ::ink::native::NewNativeMeshFormat;

extern "C" {

void MeshNative_free(int64_t native_ptr) { DeleteNativeMesh(native_ptr); }

int64_t MeshNative_createEmpty() { return NewNativeMesh(); }

int64_t MeshNative_newCopy(int64_t other_native_ptr) {
  return NewNativeMesh(CastToMesh(other_native_ptr));
}

int MeshNative_getVertexCount(int64_t native_ptr) {
  return CastToMesh(native_ptr).VertexCount();
}

int MeshNative_getVertexStride(int64_t native_ptr) {
  return CastToMesh(native_ptr).VertexStride();
}

int MeshNative_getTriangleCount(int64_t native_ptr) {
  return CastToMesh(native_ptr).TriangleCount();
}

int MeshNative_getAttributeCount(int64_t native_ptr) {
  return CastToMesh(native_ptr).Format().Attributes().size();
}

bool MeshNative_isEmpty(int64_t native_ptr) {
  return CastToMesh(native_ptr).Bounds().IsEmpty();
}

MeshNative_Box MeshNative_getBounds(int64_t native_ptr) {
  const std::optional<Rect>& bounds_rect =
      CastToMesh(native_ptr).Bounds().AsRect();
  ABSL_CHECK(bounds_rect.has_value());
  return {bounds_rect->XMin(), bounds_rect->YMin(), bounds_rect->XMax(),
          bounds_rect->YMax()};
}

int MeshNative_fillAttributeUnpackingParams(int64_t native_ptr,
                                            int attribute_index, float* offsets,
                                            float* scales) {
  absl::Span<const ink::MeshAttributeCodingParams::ComponentCodingParams>
      coding_params = CastToMesh(native_ptr)
                          .VertexAttributeUnpackingParams(attribute_index)
                          .components.Values();
  for (size_t i = 0; i < coding_params.size(); ++i) {
    offsets[i] = coding_params[i].offset;
    scales[i] = coding_params[i].scale;
  }
  return coding_params.size();
}

int64_t MeshNative_newCopyOfFormat(int64_t native_ptr) {
  return NewNativeMeshFormat(CastToMesh(native_ptr).Format());
}

MeshNative_Vec MeshNative_getVertexPosition(int64_t native_ptr,
                                            int vertex_index) {
  Point p = CastToMesh(native_ptr).VertexPosition(vertex_index);
  return {p.x, p.y};
}

}  // extern "C"
