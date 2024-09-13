// Copyright 2024 Google LLC
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

#include <jni.h>

#include <cstddef>
#include <memory>

#include "absl/log/absl_check.h"
#include "absl/types/span.h"
#include "ink/geometry/internal/jni/envelope_jni_helper.h"
#include "ink/geometry/internal/jni/vec_jni_helper.h"
#include "ink/geometry/mesh.h"
#include "ink/geometry/mesh_format.h"
#include "ink/geometry/mesh_packing_types.h"
#include "ink/geometry/point.h"
#include "ink/jni/internal/jni_defines.h"

namespace {

using ::ink::Mesh;
using ::ink::MeshFormat;
using ::ink::Point;

// The maximum supported number of attribute unpacking components.
//
// This should equal `ink::MeshAttributeCodingParams::components.MaxSize()`, but
// we keep a separate copy here while `SmallArray::MaxSize()` is not
// `constexpr`.
//
// This should match the attribute max components value in Mesh.kt.
constexpr int kMaxAttributeUnpackingParamComponents = 4;

Mesh* GetMesh(jlong raw_ptr_to_mesh) {
  return reinterpret_cast<Mesh*>(raw_ptr_to_mesh);
}

}  // namespace

extern "C" {

// Free the given `Mesh`.
JNI_METHOD(geometry, MeshNative, void, freeNative)
(JNIEnv* env, jclass clazz, jlong raw_ptr_to_mesh) {
  delete GetMesh(raw_ptr_to_mesh);
}

// Returns a direct [ByteBuffer] wrapped around the contents of
// `ink::Mesh::RawVertexData`. It will be writeable, so be sure to only expose a
// read-only wrapper of it.
JNI_METHOD(geometry, MeshNative, jobject, createRawVertexBuffer)
(JNIEnv* env, jclass clazz, jlong raw_ptr_to_shared_ptr_mesh) {
  const Mesh* mesh = GetMesh(raw_ptr_to_shared_ptr_mesh);
  const absl::Span<const std::byte> raw_vertex_data = mesh->RawVertexData();
  if (raw_vertex_data.data() == nullptr) return nullptr;
  return env->NewDirectByteBuffer(
      // NewDirectByteBuffer needs a non-const void*. The resulting buffer is
      // writeable, but it will be wrapped at the Kotlin layer in a read-only
      // buffer that delegates to this one.
      const_cast<std::byte*>(raw_vertex_data.data()), raw_vertex_data.size());
}

// Return the number of bytes per vertex in `createRawVertexBuffer`.
JNI_METHOD(geometry, MeshNative, jint, getVertexStride)
(JNIEnv* env, jclass clazz, jlong raw_ptr_to_mesh) {
  return GetMesh(raw_ptr_to_mesh)->VertexStride();
}

// Return the number of vertices in the mesh.
JNI_METHOD(geometry, MeshNative, jint, getVertexCount)
(JNIEnv* env, jclass clazz, jlong raw_ptr_to_mesh) {
  return GetMesh(raw_ptr_to_mesh)->VertexCount();
}

// Returns a direct [ByteBuffer] wrapped around the contents of
// `ink::Mesh::RawIndexData`. It will be writeable, so be sure to only expose a
// read-only wrapper of it. The raw data is stored in `uint16_t`s (two bytes per
// element), so it can be treated as a `ShortBuffer`.
JNI_METHOD(geometry, MeshNative, jobject, createRawTriangleIndexBuffer)
(JNIEnv* env, jclass clazz, jlong raw_ptr_to_mesh) {
  const Mesh* mesh = GetMesh(raw_ptr_to_mesh);
  // `Mesh`'s indices should always be two bytes each.
  ABSL_CHECK_EQ(mesh->IndexStride(), 2);
  const absl::Span<const std::byte> raw_triangle_index_data =
      mesh->RawIndexData();
  if (raw_triangle_index_data.data() == nullptr) return nullptr;
  return env->NewDirectByteBuffer(
      // NewDirectByteBuffer needs a non-const void*. The resulting buffer is
      // writeable, but it will be wrapped at the Kotlin layer in a read-only
      // buffer that delegates to this one.
      const_cast<std::byte*>(raw_triangle_index_data.data()),
      raw_triangle_index_data.size());
}

// Return the number of triangles represented in `createRawTriangleIndexBuffer`.
JNI_METHOD(geometry, MeshNative, jint, getTriangleCount)
(JNIEnv* env, jclass clazz, jlong raw_ptr_to_mesh) {
  return GetMesh(raw_ptr_to_mesh)->TriangleCount();
}

JNI_METHOD(geometry, MeshNative, jint, getAttributeCount)
(JNIEnv* env, jclass clazz, jlong raw_ptr_to_mesh) {
  return GetMesh(raw_ptr_to_mesh)->Format().Attributes().size();
}

JNI_METHOD(geometry, MeshNative, void, fillBounds)
(JNIEnv* env, jclass clazz, jlong raw_ptr_to_mesh, jobject mutable_envelope) {
  const Mesh* mesh = GetMesh(raw_ptr_to_mesh);
  ink::FillJMutableEnvelope(env, mesh->Bounds(), mutable_envelope);
}

JNI_METHOD(geometry, MeshNative, jint, fillAttributeUnpackingParams)
(JNIEnv* env, jclass clazz, jlong raw_ptr_to_mesh, jint attribute_index,
 jfloatArray offsets, jfloatArray scales) {
  const Mesh* mesh = GetMesh(raw_ptr_to_mesh);
  absl::Span<const ink::MeshAttributeCodingParams::ComponentCodingParams>
      coding_params = mesh->VertexAttributeUnpackingParams(attribute_index)
                          .components.Values();
  ABSL_CHECK_LE(coding_params.size(), kMaxAttributeUnpackingParamComponents);
  jfloat offset_values[kMaxAttributeUnpackingParamComponents];
  jfloat scale_values[kMaxAttributeUnpackingParamComponents];
  for (int i = 0; i < coding_params.size(); ++i) {
    offset_values[i] = coding_params[i].offset;
    scale_values[i] = coding_params[i].scale;
  }
  env->SetFloatArrayRegion(offsets, 0, coding_params.size(), offset_values);
  env->SetFloatArrayRegion(scales, 0, coding_params.size(), scale_values);
  return coding_params.size();
}

// Return a newly allocated copy of the given `Mesh`'s `MeshFormat`.
JNI_METHOD(geometry, MeshNative, jlong, allocCopyOfFormat)
(JNIEnv* env, jclass clazz, jlong raw_ptr_to_mesh) {
  auto copied_format =
      std::make_unique<MeshFormat>(GetMesh(raw_ptr_to_mesh)->Format());
  return reinterpret_cast<jlong>(copied_format.release());
}

JNI_METHOD(geometry, MeshNative, void, fillPosition)
(JNIEnv* env, jclass clazz, jlong raw_ptr_to_mesh, jint vertex_index,
 jobject mutable_vec) {
  const Mesh* mesh = GetMesh(raw_ptr_to_mesh);
  Point p = mesh->VertexPosition(vertex_index);
  ink::FillJMutableVecFromPoint(env, mutable_vec, p);
}

// Create a newly allocated empty `Mesh`.
JNI_METHOD(geometry, MeshNative, jlong, allocNativeNewEmptyMesh)
(JNIEnv* env, jclass clazz) { return reinterpret_cast<jlong>(new Mesh); }

}  // extern "C"
