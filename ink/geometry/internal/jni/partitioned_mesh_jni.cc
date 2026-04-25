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

#include <jni.h>

#include <cstdint>
#include <vector>

#include "ink/geometry/internal/jni/box_jni_helper.h"
#include "ink/geometry/internal/jni/partitioned_mesh_native.h"
#include "ink/geometry/internal/jni/vec_jni_helper.h"
#include "ink/jni/internal/jni_defines.h"

using ::ink::jni::CreateJImmutableBoxOrThrow;
using ::ink::jni::FillJMutableVecOrThrow;

extern "C" {

JNI_METHOD(geometry, PartitionedMeshNative, jlongArray,
           getRenderGroupMeshPointers)
(JNIEnv* env, jobject object, jlong native_pointer, jint group_index) {
  int group_mesh_count = PartitionedMeshNative_getRenderGroupMeshCount(
      native_pointer, group_index);
  std::vector<int64_t> meshes_ptrs(group_mesh_count);
  PartitionedMeshNative_fillRenderGroupMeshPointers(native_pointer, group_index,
                                                    meshes_ptrs.data());
  jlongArray mesh_pointers = env->NewLongArray(group_mesh_count);
  // Both `jlong` and `int64_t` are required to be 64-bit integers which JNI and
  // Kotlin-cinterop respectively both map to Kotlin `Long`. However, on MacOS
  // they represent two distinct (though equivalent) types, `jlong` is `long`
  // but `int64_t` is `long long`.
  static_assert(sizeof(jlong) == sizeof(int64_t));
  env->SetLongArrayRegion(mesh_pointers, 0, group_mesh_count,
                          reinterpret_cast<jlong*>(meshes_ptrs.data()));
  return mesh_pointers;
}

JNI_METHOD(geometry, PartitionedMeshNative, jint, getRenderGroupCount)
(JNIEnv* env, jobject object, jlong native_pointer) {
  return PartitionedMeshNative_getRenderGroupCount(native_pointer);
}

JNI_METHOD(geometry, PartitionedMeshNative, jlong, newCopyOfRenderGroupFormat)
(JNIEnv* env, jobject object, jlong native_pointer, jint group_index) {
  return PartitionedMeshNative_newCopyOfRenderGroupFormat(native_pointer,
                                                          group_index);
}

JNI_METHOD(geometry, PartitionedMeshNative, jint, getOutlineCount)
(JNIEnv* env, jobject object, jlong native_pointer, jint group_index) {
  return PartitionedMeshNative_getOutlineCount(native_pointer, group_index);
}

JNI_METHOD(geometry, PartitionedMeshNative, jint, getOutlineVertexCount)
(JNIEnv* env, jobject object, jlong native_pointer, jint group_index,
 jint outline_index) {
  return PartitionedMeshNative_getOutlineVertexCount(
      native_pointer, group_index, outline_index);
}

JNI_METHOD(geometry, PartitionedMeshNative, void, populateOutlineVertexPosition)
(JNIEnv* env, jobject object, jlong native_pointer, jint group_index,
 jint outline_index, jint outline_vertex_index,
 jobject out_position_mutable_vec) {
  PartitionedMeshNative_Vec position =
      PartitionedMeshNative_getOutlineVertexPosition(
          native_pointer, group_index, outline_index, outline_vertex_index);
  FillJMutableVecOrThrow(env, position, out_position_mutable_vec);
}

JNI_METHOD(geometry, PartitionedMeshNative, jobject, createBounds)
(JNIEnv* env, jobject object, jlong native_pointer, jobject mutable_box) {
  if (PartitionedMeshNative_isEmpty(native_pointer)) {
    return nullptr;
  }
  return CreateJImmutableBoxOrThrow(
      env, PartitionedMeshNative_getBounds(native_pointer));
}

// Allocate an empty `PartitionedMesh` and return a pointer to it.
JNI_METHOD(geometry, PartitionedMeshNative, jlong, createEmptyForTesting)
(JNIEnv* env, jobject object) {
  return PartitionedMeshNative_createEmptyForTesting();
}

JNI_METHOD(geometry, PartitionedMeshNative, jlong, createFromTriangleForTesting)
(JNIEnv* env, jobject object, jfloat triangle_p0_x, jfloat triangle_p0_y,
 jfloat triangle_p1_x, jfloat triangle_p1_y, jfloat triangle_p2_x,
 jfloat triangle_p2_y) {
  return PartitionedMeshNative_createFromTriangleForTesting(
      triangle_p0_x, triangle_p0_y, triangle_p1_x, triangle_p1_y, triangle_p2_x,
      triangle_p2_y);
}

JNI_METHOD(geometry, PartitionedMeshNative, jfloat, triangleCoverage)
(JNIEnv* env, jobject object, jlong native_pointer, jfloat triangle_p0_x,
 jfloat triangle_p0_y, jfloat triangle_p1_x, jfloat triangle_p1_y,
 jfloat triangle_p2_x, jfloat triangle_p2_y,
 jfloat triangle_to_partitionedMesh_transform_a,
 jfloat triangle_to_partitionedMesh_transform_b,
 jfloat triangle_to_partitionedMesh_transform_c,
 jfloat triangle_to_partitionedMesh_transform_d,
 jfloat triangle_to_partitionedMesh_transform_e,
 jfloat triangle_to_partitionedMesh_transform_f) {
  return PartitionedMeshNative_triangleCoverage(
      native_pointer, triangle_p0_x, triangle_p0_y, triangle_p1_x,
      triangle_p1_y, triangle_p2_x, triangle_p2_y,
      triangle_to_partitionedMesh_transform_a,
      triangle_to_partitionedMesh_transform_b,
      triangle_to_partitionedMesh_transform_c,
      triangle_to_partitionedMesh_transform_d,
      triangle_to_partitionedMesh_transform_e,
      triangle_to_partitionedMesh_transform_f);
}

JNI_METHOD(geometry, PartitionedMeshNative, jfloat, boxCoverage)
(JNIEnv* env, jobject object, jlong native_pointer, jfloat rect_x_min,
 jfloat rect_y_min, jfloat rect_x_max, jfloat rect_y_max,
 jfloat rect_to_partitionedMesh_transform_a,
 jfloat rect_to_partitionedMesh_transform_b,
 jfloat rect_to_partitionedMesh_transform_c,
 jfloat rect_to_partitionedMesh_transform_d,
 jfloat rect_to_partitionedMesh_transform_e,
 jfloat rect_to_partitionedMesh_transform_f) {
  return PartitionedMeshNative_boxCoverage(
      native_pointer, rect_x_min, rect_y_min, rect_x_max, rect_y_max,
      rect_to_partitionedMesh_transform_a, rect_to_partitionedMesh_transform_b,
      rect_to_partitionedMesh_transform_c, rect_to_partitionedMesh_transform_d,
      rect_to_partitionedMesh_transform_e, rect_to_partitionedMesh_transform_f);
}

JNI_METHOD(geometry, PartitionedMeshNative, jfloat, parallelogramCoverage)
(JNIEnv* env, jobject object, jlong native_pointer, jfloat quad_center_x,
 jfloat quad_center_y, jfloat quad_width, jfloat quad_height,
 jfloat quad_angle_radian, jfloat quad_shear_factor,
 jfloat quad_to_partitionedMesh_transform_a,
 jfloat quad_to_partitionedMesh_transform_b,
 jfloat quad_to_partitionedMesh_transform_c,
 jfloat quad_to_partitionedMesh_transform_d,
 jfloat quad_to_partitionedMesh_transform_e,
 jfloat quad_to_partitionedMesh_transform_f) {
  return PartitionedMeshNative_parallelogramCoverage(
      native_pointer, quad_center_x, quad_center_y, quad_width, quad_height,
      quad_angle_radian, quad_shear_factor, quad_to_partitionedMesh_transform_a,
      quad_to_partitionedMesh_transform_b, quad_to_partitionedMesh_transform_c,
      quad_to_partitionedMesh_transform_d, quad_to_partitionedMesh_transform_e,
      quad_to_partitionedMesh_transform_f);
}

JNI_METHOD(geometry, PartitionedMeshNative, jfloat, partitionedMeshCoverage)
(JNIEnv* env, jobject object, jlong native_pointer,
 jlong other_partitioned_mesh_pointer, jfloat other_to_this_transform_a,
 jfloat other_to_this_transform_b, jfloat other_to_this_transform_c,
 jfloat other_to_this_transform_d, jfloat other_to_this_transform_e,
 jfloat other_to_this_transform_f) {
  return PartitionedMeshNative_partitionedMeshCoverage(
      native_pointer, other_partitioned_mesh_pointer, other_to_this_transform_a,
      other_to_this_transform_b, other_to_this_transform_c,
      other_to_this_transform_d, other_to_this_transform_e,
      other_to_this_transform_f);
}

JNI_METHOD(geometry, PartitionedMeshNative, jboolean,
           triangleCoverageIsGreaterThan)
(JNIEnv* env, jobject object, jlong native_pointer, jfloat triangle_p0_x,
 jfloat triangle_p0_y, jfloat triangle_p1_x, jfloat triangle_p1_y,
 jfloat triangle_p2_x, jfloat triangle_p2_y, jfloat coverage_threshold,
 jfloat triangle_to_partitionedMesh_transform_a,
 jfloat triangle_to_partitionedMesh_transform_b,
 jfloat triangle_to_partitionedMesh_transform_c,
 jfloat triangle_to_partitionedMesh_transform_d,
 jfloat triangle_to_partitionedMesh_transform_e,
 jfloat triangle_to_partitionedMesh_transform_f) {
  return PartitionedMeshNative_triangleCoverageIsGreaterThan(
      native_pointer, triangle_p0_x, triangle_p0_y, triangle_p1_x,
      triangle_p1_y, triangle_p2_x, triangle_p2_y, coverage_threshold,
      triangle_to_partitionedMesh_transform_a,
      triangle_to_partitionedMesh_transform_b,
      triangle_to_partitionedMesh_transform_c,
      triangle_to_partitionedMesh_transform_d,
      triangle_to_partitionedMesh_transform_e,
      triangle_to_partitionedMesh_transform_f);
}

JNI_METHOD(geometry, PartitionedMeshNative, jboolean, boxCoverageIsGreaterThan)
(JNIEnv* env, jobject object, jlong native_pointer, jfloat rect_x_min,
 jfloat rect_y_min, jfloat rect_x_max, jfloat rect_y_max,
 jfloat coverage_threshold, jfloat rect_to_partitionedMesh_transform_a,
 jfloat rect_to_partitionedMesh_transform_b,
 jfloat rect_to_partitionedMesh_transform_c,
 jfloat rect_to_partitionedMesh_transform_d,
 jfloat rect_to_partitionedMesh_transform_e,
 jfloat rect_to_partitionedMesh_transform_f) {
  return PartitionedMeshNative_boxCoverageIsGreaterThan(
      native_pointer, rect_x_min, rect_y_min, rect_x_max, rect_y_max,
      coverage_threshold, rect_to_partitionedMesh_transform_a,
      rect_to_partitionedMesh_transform_b, rect_to_partitionedMesh_transform_c,
      rect_to_partitionedMesh_transform_d, rect_to_partitionedMesh_transform_e,
      rect_to_partitionedMesh_transform_f);
}

JNI_METHOD(geometry, PartitionedMeshNative, jboolean,
           parallelogramCoverageIsGreaterThan)
(JNIEnv* env, jobject object, jlong native_pointer, jfloat quad_center_x,
 jfloat quad_center_y, jfloat quad_width, jfloat quad_height,
 jfloat quad_angle_radian, jfloat quad_shear_factor, jfloat coverage_threshold,
 jfloat quad_to_partitionedMesh_transform_a,
 jfloat quad_to_partitionedMesh_transform_b,
 jfloat quad_to_partitionedMesh_transform_c,
 jfloat quad_to_partitionedMesh_transform_d,
 jfloat quad_to_partitionedMesh_transform_e,
 jfloat quad_to_partitionedMesh_transform_f) {
  return PartitionedMeshNative_parallelogramCoverageIsGreaterThan(
      native_pointer, quad_center_x, quad_center_y, quad_width, quad_height,
      quad_angle_radian, quad_shear_factor, coverage_threshold,
      quad_to_partitionedMesh_transform_a, quad_to_partitionedMesh_transform_b,
      quad_to_partitionedMesh_transform_c, quad_to_partitionedMesh_transform_d,
      quad_to_partitionedMesh_transform_e, quad_to_partitionedMesh_transform_f);
}

JNI_METHOD(geometry, PartitionedMeshNative, jboolean,
           partitionedMeshCoverageIsGreaterThan)
(JNIEnv* env, jobject object, jlong native_pointer,
 jlong other_partitioned_mesh_pointer, jfloat coverage_threshold,
 jfloat other_to_this_transform_a, jfloat other_to_this_transform_b,
 jfloat other_to_this_transform_c, jfloat other_to_this_transform_d,
 jfloat other_to_this_transform_e, jfloat other_to_this_transform_f) {
  return PartitionedMeshNative_partitionedMeshCoverageIsGreaterThan(
      native_pointer, other_partitioned_mesh_pointer, coverage_threshold,
      other_to_this_transform_a, other_to_this_transform_b,
      other_to_this_transform_c, other_to_this_transform_d,
      other_to_this_transform_e, other_to_this_transform_f);
}

JNI_METHOD(geometry, PartitionedMeshNative, void, initializeSpatialIndex)
(JNIEnv* env, jobject object, jlong native_pointer) {
  PartitionedMeshNative_initializeSpatialIndex(native_pointer);
}

JNI_METHOD(geometry, PartitionedMeshNative, jboolean, isSpatialIndexInitialized)
(JNIEnv* env, jobject object, jlong native_pointer) {
  return PartitionedMeshNative_isSpatialIndexInitialized(native_pointer);
}

JNI_METHOD(geometry, PartitionedMeshNative, void, free)
(JNIEnv* env, jobject object, jlong native_pointer) {
  PartitionedMeshNative_free(native_pointer);
}

}  // extern "C"
