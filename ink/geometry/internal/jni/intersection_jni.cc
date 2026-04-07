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

#include "ink/geometry/internal/jni/intersection_native.h"
#include "ink/jni/internal/jni_defines.h"

extern "C" {

JNI_METHOD(geometry, IntersectionNative, jboolean, vecSegmentIntersects)
(JNIEnv* env, jobject object, jfloat vec_x, jfloat vec_y,
 jfloat segment_start_x, jfloat segment_start_y, jfloat segment_end_x,
 jfloat segment_end_y) {
  return IntersectionNative_vecSegmentIntersects(vec_x, vec_y, segment_start_x,
                                                 segment_start_y, segment_end_x,
                                                 segment_end_y);
}

JNI_METHOD(geometry, IntersectionNative, jboolean, vecTriangleIntersects)
(JNIEnv* env, jobject object, jfloat vec_x, jfloat vec_y, jfloat triangle_p0_x,
 jfloat triangle_p0_y, jfloat triangle_p1_x, jfloat triangle_p1_y,
 jfloat triangle_p2_x, jfloat triangle_p2_y) {
  return IntersectionNative_vecTriangleIntersects(
      vec_x, vec_y, triangle_p0_x, triangle_p0_y, triangle_p1_x, triangle_p1_y,
      triangle_p2_x, triangle_p2_y);
}

JNI_METHOD(geometry, IntersectionNative, jboolean, vecBoxIntersects)
(JNIEnv* env, jobject object, jfloat vec_x, jfloat vec_y, jfloat box_x_min,
 jfloat box_y_min, jfloat box_x_max, jfloat box_y_max) {
  return IntersectionNative_vecBoxIntersects(vec_x, vec_y, box_x_min, box_y_min,
                                             box_x_max, box_y_max);
}

JNI_METHOD(geometry, IntersectionNative, jboolean, vecParallelogramIntersects)
(JNIEnv* env, jobject object, jfloat vec_x, jfloat vec_y,
 jfloat parallelogram_center_x, jfloat parallelogram_center_y,
 jfloat parallelogram_width, jfloat parallelogram_height,
 jfloat parallelogram_angle_radian, jfloat parallelogram_shear_factor) {
  return IntersectionNative_vecParallelogramIntersects(
      vec_x, vec_y, parallelogram_center_x, parallelogram_center_y,
      parallelogram_width, parallelogram_height, parallelogram_angle_radian,
      parallelogram_shear_factor);
}

JNI_METHOD(geometry, IntersectionNative, jboolean, segmentSegmentIntersects)
(JNIEnv* env, jobject object, jfloat segment_1_start_x,
 jfloat segment_1_start_y, jfloat segment_1_end_x, jfloat segment_1_end_y,
 jfloat segment_2_start_x, jfloat segment_2_start_y, jfloat segment_2_end_x,
 jfloat segment_2_end_y) {
  return IntersectionNative_segmentSegmentIntersects(
      segment_1_start_x, segment_1_start_y, segment_1_end_x, segment_1_end_y,
      segment_2_start_x, segment_2_start_y, segment_2_end_x, segment_2_end_y);
}

JNI_METHOD(geometry, IntersectionNative, jboolean, segmentTriangleIntersects)
(JNIEnv* env, jobject object, jfloat segment_start_x, jfloat segment_start_y,
 jfloat segment_end_x, jfloat segment_end_y, jfloat triangle_p0_x,
 jfloat triangle_p0_y, jfloat triangle_p1_x, jfloat triangle_p1_y,
 jfloat triangle_p2_x, jfloat triangle_p2_y) {
  return IntersectionNative_segmentTriangleIntersects(
      segment_start_x, segment_start_y, segment_end_x, segment_end_y,
      triangle_p0_x, triangle_p0_y, triangle_p1_x, triangle_p1_y, triangle_p2_x,
      triangle_p2_y);
}

JNI_METHOD(geometry, IntersectionNative, jboolean, segmentBoxIntersects)
(JNIEnv* env, jobject object, jfloat segment_start_x, jfloat segment_start_y,
 jfloat segment_end_x, jfloat segment_end_y, jfloat box_x_min, jfloat box_y_min,
 jfloat box_x_max, jfloat box_y_max) {
  return IntersectionNative_segmentBoxIntersects(
      segment_start_x, segment_start_y, segment_end_x, segment_end_y, box_x_min,
      box_y_min, box_x_max, box_y_max);
}

JNI_METHOD(geometry, IntersectionNative, jboolean,
           segmentParallelogramIntersects)
(JNIEnv* env, jobject object, jfloat segment_start_x, jfloat segment_start_y,
 jfloat segment_end_x, jfloat segment_end_y, jfloat parallelogram_center_x,
 jfloat parallelogram_center_y, jfloat parallelogram_width,
 jfloat parallelogram_height, jfloat parallelogram_angle_radian,
 jfloat parallelogram_shear_factor) {
  return IntersectionNative_segmentParallelogramIntersects(
      segment_start_x, segment_start_y, segment_end_x, segment_end_y,
      parallelogram_center_x, parallelogram_center_y, parallelogram_width,
      parallelogram_height, parallelogram_angle_radian,
      parallelogram_shear_factor);
}

JNI_METHOD(geometry, IntersectionNative, jboolean, triangleTriangleIntersects)
(JNIEnv* env, jobject object, jfloat triangle_1_p0_x, jfloat triangle_1_p0_y,
 jfloat triangle_1_p1_x, jfloat triangle_1_p1_y, jfloat triangle_1_p2_x,
 jfloat triangle_1_p2_y, jfloat triangle_2_p0_x, jfloat triangle_2_p0_y,
 jfloat triangle_2_p1_x, jfloat triangle_2_p1_y, jfloat triangle_2_p2_x,
 jfloat triangle_2_p2_y) {
  return IntersectionNative_triangleTriangleIntersects(
      triangle_1_p0_x, triangle_1_p0_y, triangle_1_p1_x, triangle_1_p1_y,
      triangle_1_p2_x, triangle_1_p2_y, triangle_2_p0_x, triangle_2_p0_y,
      triangle_2_p1_x, triangle_2_p1_y, triangle_2_p2_x, triangle_2_p2_y);
}

JNI_METHOD(geometry, IntersectionNative, jboolean, triangleBoxIntersects)
(JNIEnv* env, jobject object, jfloat triangle_p0_x, jfloat triangle_p0_y,
 jfloat triangle_p1_x, jfloat triangle_p1_y, jfloat triangle_p2_x,
 jfloat triangle_p2_y, jfloat box_x_min, jfloat box_y_min, jfloat box_x_max,
 jfloat box_y_max) {
  return IntersectionNative_triangleBoxIntersects(
      triangle_p0_x, triangle_p0_y, triangle_p1_x, triangle_p1_y, triangle_p2_x,
      triangle_p2_y, box_x_min, box_y_min, box_x_max, box_y_max);
}

JNI_METHOD(geometry, IntersectionNative, jboolean,
           triangleParallelogramIntersects)
(JNIEnv* env, jobject object, jfloat triangle_p0_x, jfloat triangle_p0_y,
 jfloat triangle_p1_x, jfloat triangle_p1_y, jfloat triangle_p2_x,
 jfloat triangle_p2_y, jfloat parallelogram_center_x,
 jfloat parallelogram_center_y, jfloat parallelogram_width,
 jfloat parallelogram_height, jfloat parallelogram_angle_radian,
 jfloat parallelogram_shear_factor) {
  return IntersectionNative_triangleParallelogramIntersects(
      triangle_p0_x, triangle_p0_y, triangle_p1_x, triangle_p1_y, triangle_p2_x,
      triangle_p2_y, parallelogram_center_x, parallelogram_center_y,
      parallelogram_width, parallelogram_height, parallelogram_angle_radian,
      parallelogram_shear_factor);
}

JNI_METHOD(geometry, IntersectionNative, jboolean, boxBoxIntersects)
(JNIEnv* env, jobject object, jfloat box_1_x_min, jfloat box_1_y_min,
 jfloat box_1_x_max, jfloat box_1_y_max, jfloat box_2_x_min, jfloat box_2_y_min,
 jfloat box_2_x_max, jfloat box_2_y_max) {
  return IntersectionNative_boxBoxIntersects(
      box_1_x_min, box_1_y_min, box_1_x_max, box_1_y_max, box_2_x_min,
      box_2_y_min, box_2_x_max, box_2_y_max);
}

JNI_METHOD(geometry, IntersectionNative, jboolean, boxParallelogramIntersects)
(JNIEnv* env, jobject object, jfloat box_x_min, jfloat box_y_min,
 jfloat box_x_max, jfloat box_y_max, jfloat parallelogram_center_x,
 jfloat parallelogram_center_y, jfloat parallelogram_width,
 jfloat parallelogram_height, jfloat parallelogram_angle_radian,
 jfloat parallelogram_shear_factor) {
  return IntersectionNative_boxParallelogramIntersects(
      box_x_min, box_y_min, box_x_max, box_y_max, parallelogram_center_x,
      parallelogram_center_y, parallelogram_width, parallelogram_height,
      parallelogram_angle_radian, parallelogram_shear_factor);
}

JNI_METHOD(geometry, IntersectionNative, jboolean,
           parallelogramParallelogramIntersects)
(JNIEnv* env, jobject object, jfloat parallelogram_1_center_x,
 jfloat parallelogram_1_center_y, jfloat parallelogram_1_width,
 jfloat parallelogram_1_height, jfloat parallelogram_1_angle_in_radian,
 jfloat parallelogram_1_shear_factor, jfloat parallelogram_2_center_x,
 jfloat parallelogram_2_center_y, jfloat parallelogram_2_width,
 jfloat parallelogram_2_height, jfloat parallelogram_2_angle_in_radian,
 jfloat parallelogram_2_shear_factor) {
  return IntersectionNative_parallelogramParallelogramIntersects(
      parallelogram_1_center_x, parallelogram_1_center_y, parallelogram_1_width,
      parallelogram_1_height, parallelogram_1_angle_in_radian,
      parallelogram_1_shear_factor, parallelogram_2_center_x,
      parallelogram_2_center_y, parallelogram_2_width, parallelogram_2_height,
      parallelogram_2_angle_in_radian, parallelogram_2_shear_factor);
}

JNI_METHOD(geometry, IntersectionNative, jboolean, partitionedMeshVecIntersects)
(JNIEnv* env, jobject object, jlong partitioned_mesh_native_pointer,
 jfloat vec_x, jfloat vec_y, jfloat vec_to_partitionedMesh_transform_a,
 jfloat vec_to_partitionedMesh_transform_b,
 jfloat vec_to_partitionedMesh_transform_c,
 jfloat vec_to_partitionedMesh_transform_d,
 jfloat vec_to_partitionedMesh_transform_e,
 jfloat vec_to_partitionedMesh_transform_f) {
  return IntersectionNative_partitionedMeshVecIntersects(
      partitioned_mesh_native_pointer, vec_x, vec_y,
      vec_to_partitionedMesh_transform_a, vec_to_partitionedMesh_transform_b,
      vec_to_partitionedMesh_transform_c, vec_to_partitionedMesh_transform_d,
      vec_to_partitionedMesh_transform_e, vec_to_partitionedMesh_transform_f);
}

JNI_METHOD(geometry, IntersectionNative, jboolean,
           partitionedMeshSegmentIntersects)
(JNIEnv* env, jobject object, jlong partitioned_mesh_native_pointer,
 jfloat segment_start_x, jfloat segment_start_y, jfloat segment_end_x,
 jfloat segment_end_y, jfloat segment_to_partitionedMesh_transform_a,
 jfloat segment_to_partitionedMesh_transform_b,
 jfloat segment_to_partitionedMesh_transform_c,
 jfloat segment_to_partitionedMesh_transform_d,
 jfloat segment_to_partitionedMesh_transform_e,
 jfloat segment_to_partitionedMesh_transform_f) {
  return IntersectionNative_partitionedMeshSegmentIntersects(
      partitioned_mesh_native_pointer, segment_start_x, segment_start_y,
      segment_end_x, segment_end_y, segment_to_partitionedMesh_transform_a,
      segment_to_partitionedMesh_transform_b,
      segment_to_partitionedMesh_transform_c,
      segment_to_partitionedMesh_transform_d,
      segment_to_partitionedMesh_transform_e,
      segment_to_partitionedMesh_transform_f);
}

JNI_METHOD(geometry, IntersectionNative, jboolean,
           partitionedMeshTriangleIntersects)
(JNIEnv* env, jobject object, jlong partitioned_mesh_native_pointer,
 jfloat triangle_p0_x, jfloat triangle_p0_y, jfloat triangle_p1_x,
 jfloat triangle_p1_y, jfloat triangle_p2_x, jfloat triangle_p2_y,
 jfloat triangle_to_partitionedMesh_transform_a,
 jfloat triangle_to_partitionedMesh_transform_b,
 jfloat triangle_to_partitionedMesh_transform_c,
 jfloat triangle_to_partitionedMesh_transform_d,
 jfloat triangle_to_partitionedMesh_transform_e,
 jfloat triangle_to_partitionedMesh_transform_f) {
  return IntersectionNative_partitionedMeshTriangleIntersects(
      partitioned_mesh_native_pointer, triangle_p0_x, triangle_p0_y,
      triangle_p1_x, triangle_p1_y, triangle_p2_x, triangle_p2_y,
      triangle_to_partitionedMesh_transform_a,
      triangle_to_partitionedMesh_transform_b,
      triangle_to_partitionedMesh_transform_c,
      triangle_to_partitionedMesh_transform_d,
      triangle_to_partitionedMesh_transform_e,
      triangle_to_partitionedMesh_transform_f);
}

JNI_METHOD(geometry, IntersectionNative, jboolean, partitionedMeshBoxIntersects)
(JNIEnv* env, jobject object, jlong partitioned_mesh_native_pointer,
 jfloat box_x_min, jfloat box_y_min, jfloat box_x_max, jfloat box_y_max,
 jfloat box_to_partitionedMesh_transform_a,
 jfloat box_to_partitionedMesh_transform_b,
 jfloat box_to_partitionedMesh_transform_c,
 jfloat box_to_partitionedMesh_transform_d,
 jfloat box_to_partitionedMesh_transform_e,
 jfloat box_to_partitionedMesh_transform_f) {
  return IntersectionNative_partitionedMeshBoxIntersects(
      partitioned_mesh_native_pointer, box_x_min, box_y_min, box_x_max,
      box_y_max, box_to_partitionedMesh_transform_a,
      box_to_partitionedMesh_transform_b, box_to_partitionedMesh_transform_c,
      box_to_partitionedMesh_transform_d, box_to_partitionedMesh_transform_e,
      box_to_partitionedMesh_transform_f);
}

JNI_METHOD(geometry, IntersectionNative, jboolean,
           partitionedMeshParallelogramIntersects)
(JNIEnv* env, jobject object, jlong partitioned_mesh_native_pointer,
 jfloat parallelogram_center_x, jfloat parallelogram_center_y,
 jfloat parallelogram_width, jfloat parallelogram_height,
 jfloat parallelogram_angle_radian, jfloat parallelogram_shear_factor,
 jfloat parallelogram_to_partitionedMesh_transform_a,
 jfloat parallelogram_to_partitionedMesh_transform_b,
 jfloat parallelogram_to_partitionedMesh_transform_c,
 jfloat parallelogram_to_partitionedMesh_transform_d,
 jfloat parallelogram_to_partitionedMesh_transform_e,
 jfloat parallelogram_to_partitionedMesh_transform_f) {
  return IntersectionNative_partitionedMeshParallelogramIntersects(
      partitioned_mesh_native_pointer, parallelogram_center_x,
      parallelogram_center_y, parallelogram_width, parallelogram_height,
      parallelogram_angle_radian, parallelogram_shear_factor,
      parallelogram_to_partitionedMesh_transform_a,
      parallelogram_to_partitionedMesh_transform_b,
      parallelogram_to_partitionedMesh_transform_c,
      parallelogram_to_partitionedMesh_transform_d,
      parallelogram_to_partitionedMesh_transform_e,
      parallelogram_to_partitionedMesh_transform_f);
}

JNI_METHOD(geometry, IntersectionNative, jboolean,
           partitionedMeshPartitionedMeshIntersects)
(JNIEnv* env, jobject object, jlong this_partitioned_mesh_native_pointer,
 jlong other_partitioned_mesh_native_pointer, jfloat this_to_common_transform_a,
 jfloat this_to_common_transform_b, jfloat this_to_common_transform_c,
 jfloat this_to_common_transform_d, jfloat this_to_common_transform_e,
 jfloat this_to_common_transform_f, jfloat other_to_common_transform_a,
 jfloat other_to_common_transform_b, jfloat other_to_common_transform_c,
 jfloat other_to_common_transform_d, jfloat other_to_common_transform_e,
 jfloat other_to_common_transform_f) {
  return IntersectionNative_partitionedMeshPartitionedMeshIntersects(
      this_partitioned_mesh_native_pointer,
      other_partitioned_mesh_native_pointer, this_to_common_transform_a,
      this_to_common_transform_b, this_to_common_transform_c,
      this_to_common_transform_d, this_to_common_transform_e,
      this_to_common_transform_f, other_to_common_transform_a,
      other_to_common_transform_b, other_to_common_transform_c,
      other_to_common_transform_d, other_to_common_transform_e,
      other_to_common_transform_f);
}

}  // extern "C"
