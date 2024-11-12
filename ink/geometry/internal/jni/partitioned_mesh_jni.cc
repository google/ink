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

#include <memory>
#include <vector>

#include "absl/types/span.h"
#include "ink/geometry/affine_transform.h"
#include "ink/geometry/angle.h"
#include "ink/geometry/mesh.h"
#include "ink/geometry/mesh_format.h"
#include "ink/geometry/partitioned_mesh.h"
#include "ink/geometry/point.h"
#include "ink/geometry/quad.h"
#include "ink/geometry/rect.h"
#include "ink/geometry/triangle.h"
#include "ink/jni/internal/jni_defines.h"

namespace {

using ::ink::AffineTransform;
using ::ink::Angle;
using ::ink::Mesh;
using ::ink::MeshFormat;
using ::ink::PartitionedMesh;
using ::ink::Point;
using ::ink::Quad;
using ::ink::Rect;
using ::ink::Triangle;

PartitionedMesh* GetPartitionedMesh(jlong raw_ptr_to_partitioned_mesh) {
  return reinterpret_cast<PartitionedMesh*>(raw_ptr_to_partitioned_mesh);
}

}  // namespace

extern "C" {

// Free the given `PartitionedMesh`.
JNI_METHOD(geometry, PartitionedMeshNative, void, free)
(JNIEnv* env, jclass clazz, jlong raw_ptr_to_partitioned_mesh) {
  delete GetPartitionedMesh(raw_ptr_to_partitioned_mesh);
}

JNI_METHOD(geometry, PartitionedMeshNative, jlongArray,
           getNativeAddressesOfMeshes)
(JNIEnv* env, jclass clazz, jlong raw_ptr_to_partitioned_mesh,
 jint group_index) {
  absl::Span<const Mesh> meshes =
      GetPartitionedMesh(raw_ptr_to_partitioned_mesh)
          ->RenderGroupMeshes(group_index);
  std::vector<jlong> meshes_ptrs(meshes.size());
  for (int i = 0; i < meshes.size(); ++i) {
    // Create new heap-allocated copies of each `Mesh` object, to be owned by
    // the `Mesh.kt` objects of the `PartitionedMesh.kt` that is under
    // construction. The C++ `Mesh` type is cheap to copy because internally it
    // keeps a `shared_ptr` to its (immutable) data.
    meshes_ptrs[i] = reinterpret_cast<jlong>(new Mesh(meshes[i]));
  }
  jlongArray meshes_addresses = env->NewLongArray(meshes.size());
  env->SetLongArrayRegion(meshes_addresses, 0, meshes.size(),
                          meshes_ptrs.data());
  return meshes_addresses;
}

JNI_METHOD(geometry, PartitionedMeshNative, jint, getRenderGroupCount)
(JNIEnv* env, jclass clazz, jlong raw_ptr_to_partitioned_mesh) {
  return GetPartitionedMesh(raw_ptr_to_partitioned_mesh)->RenderGroupCount();
}

JNI_METHOD(geometry, PartitionedMeshNative, jlong, getRenderGroupFormat)
(JNIEnv* env, jclass clazz, jlong raw_ptr_to_partitioned_mesh,
 jint group_index) {
  const PartitionedMesh* partitioned_mesh =
      GetPartitionedMesh(raw_ptr_to_partitioned_mesh);
  auto mesh_format_ptr = std::make_unique<MeshFormat>(
      partitioned_mesh->RenderGroupFormat(group_index));
  return reinterpret_cast<jlong>(mesh_format_ptr.release());
}

JNI_METHOD(geometry, PartitionedMeshNative, jint, getOutlineCount)
(JNIEnv* env, jclass clazz, jlong raw_ptr_to_partitioned_mesh,
 jint group_index) {
  return GetPartitionedMesh(raw_ptr_to_partitioned_mesh)
      ->OutlineCount(group_index);
}

JNI_METHOD(geometry, PartitionedMeshNative, jint, getOutlineVertexCount)
(JNIEnv* env, jclass clazz, jlong raw_ptr_to_partitioned_mesh, jint group_index,
 jint outline_index) {
  const PartitionedMesh* partitioned_mesh =
      GetPartitionedMesh(raw_ptr_to_partitioned_mesh);
  return partitioned_mesh->Outline(group_index, outline_index).size();
}

JNI_METHOD(geometry, PartitionedMeshNative, void,
           fillOutlineMeshIndexAndMeshVertexIndex)
(JNIEnv* env, jclass clazz, jlong raw_ptr_to_partitioned_mesh, jint group_index,
 jint outline_index, jint outline_vertex_index,
 jintArray out_mesh_index_and_mesh_vertex_index) {
  const PartitionedMesh* partitioned_mesh =
      GetPartitionedMesh(raw_ptr_to_partitioned_mesh);
  absl::Span<const PartitionedMesh::VertexIndexPair> outline =
      partitioned_mesh->Outline(group_index, outline_index);
  PartitionedMesh::VertexIndexPair index_pair = outline[outline_vertex_index];
  jint mesh_index_and_mesh_vertex_index[] = {index_pair.mesh_index,
                                             index_pair.vertex_index};
  env->SetIntArrayRegion(out_mesh_index_and_mesh_vertex_index, 0, 2,
                         mesh_index_and_mesh_vertex_index);
}

// Allocate an empty `PartitionedMesh` and return a pointer to it.
JNI_METHOD(geometry, PartitionedMeshNative, jlong, alloc)
(JNIEnv* env, jclass clazz) {
  auto partitioned_mesh = std::make_unique<PartitionedMesh>();
  return reinterpret_cast<jlong>(partitioned_mesh.release());
}

JNI_METHOD(geometry, PartitionedMeshNative, jfloat,
           partitionedMeshTriangleCoverage)
(JNIEnv* env, jclass clazz, jlong raw_ptr_to_partitioned_mesh,
 jfloat triangle_p0_x, jfloat triangle_p0_y, jfloat triangle_p1_x,
 jfloat triangle_p1_y, jfloat triangle_p2_x, jfloat triangle_p2_y,
 jfloat triangle_to_partitionedMesh_transform_a,
 jfloat triangle_to_partitionedMesh_transform_b,
 jfloat triangle_to_partitionedMesh_transform_c,
 jfloat triangle_to_partitionedMesh_transform_d,
 jfloat triangle_to_partitionedMesh_transform_e,
 jfloat triangle_to_partitionedMesh_transform_f) {
  PartitionedMesh* partitioned_mesh =
      GetPartitionedMesh(raw_ptr_to_partitioned_mesh);
  Triangle triangle{{triangle_p0_x, triangle_p0_y},
                    {triangle_p1_x, triangle_p1_y},
                    {triangle_p2_x, triangle_p2_y}};
  AffineTransform transform(triangle_to_partitionedMesh_transform_a,
                            triangle_to_partitionedMesh_transform_b,
                            triangle_to_partitionedMesh_transform_c,
                            triangle_to_partitionedMesh_transform_d,
                            triangle_to_partitionedMesh_transform_e,
                            triangle_to_partitionedMesh_transform_f);
  return partitioned_mesh->Coverage(triangle, transform);
}

JNI_METHOD(geometry, PartitionedMeshNative, jfloat, partitionedMeshBoxCoverage)
(JNIEnv* env, jclass clazz, jlong raw_ptr_to_partitioned_mesh,
 jfloat rect_x_min, jfloat rect_y_min, jfloat rect_x_max, jfloat rect_y_max,
 jfloat rect_to_partitionedMesh_transform_a,
 jfloat rect_to_partitionedMesh_transform_b,
 jfloat rect_to_partitionedMesh_transform_c,
 jfloat rect_to_partitionedMesh_transform_d,
 jfloat rect_to_partitionedMesh_transform_e,
 jfloat rect_to_partitionedMesh_transform_f) {
  PartitionedMesh* partitioned_mesh =
      GetPartitionedMesh(raw_ptr_to_partitioned_mesh);
  Rect rect =
      Rect::FromTwoPoints({rect_x_min, rect_y_min}, {rect_x_max, rect_y_max});
  AffineTransform transform(
      rect_to_partitionedMesh_transform_a, rect_to_partitionedMesh_transform_b,
      rect_to_partitionedMesh_transform_c, rect_to_partitionedMesh_transform_d,
      rect_to_partitionedMesh_transform_e, rect_to_partitionedMesh_transform_f);
  return partitioned_mesh->Coverage(rect, transform);
}

JNI_METHOD(geometry, PartitionedMeshNative, jfloat,
           partitionedMeshParallelogramCoverage)
(JNIEnv* env, jclass clazz, jlong raw_ptr_to_partitioned_mesh,
 jfloat quad_center_x, jfloat quad_center_y, jfloat quad_width,
 jfloat quad_height, jfloat quad_angle_radian, jfloat quad_shear_factor,
 jfloat quad_to_partitionedMesh_transform_a,
 jfloat quad_to_partitionedMesh_transform_b,
 jfloat quad_to_partitionedMesh_transform_c,
 jfloat quad_to_partitionedMesh_transform_d,
 jfloat quad_to_partitionedMesh_transform_e,
 jfloat quad_to_partitionedMesh_transform_f) {
  PartitionedMesh* partitioned_mesh =
      GetPartitionedMesh(raw_ptr_to_partitioned_mesh);
  Quad quad = Quad::FromCenterDimensionsRotationAndShear(
      Point{quad_center_x, quad_center_y}, quad_width, quad_height,
      Angle::Radians(quad_angle_radian), quad_shear_factor);
  AffineTransform transform(
      quad_to_partitionedMesh_transform_a, quad_to_partitionedMesh_transform_b,
      quad_to_partitionedMesh_transform_c, quad_to_partitionedMesh_transform_d,
      quad_to_partitionedMesh_transform_e, quad_to_partitionedMesh_transform_f);
  return partitioned_mesh->Coverage(quad, transform);
}

JNI_METHOD(geometry, PartitionedMeshNative, jfloat,
           partitionedMeshPartitionedMeshCoverage)
(JNIEnv* env, jclass clazz, jlong raw_ptr_to_this_partitioned_mesh,
 jlong raw_ptr_to_other_partitioned_mesh, jfloat other_to_this_transform_a,
 jfloat other_to_this_transform_b, jfloat other_to_this_transform_c,
 jfloat other_to_this_transform_d, jfloat other_to_this_transform_e,
 jfloat other_to_this_transform_f) {
  PartitionedMesh* this_partitioned_mesh =
      GetPartitionedMesh(raw_ptr_to_this_partitioned_mesh);
  PartitionedMesh* other_partitioned_mesh =
      GetPartitionedMesh(raw_ptr_to_other_partitioned_mesh);
  AffineTransform transform(
      other_to_this_transform_a, other_to_this_transform_b,
      other_to_this_transform_c, other_to_this_transform_d,
      other_to_this_transform_e, other_to_this_transform_f);
  return this_partitioned_mesh->Coverage(*other_partitioned_mesh, transform);
}

JNI_METHOD(geometry, PartitionedMeshNative, jboolean,
           partitionedMeshTriangleCoverageIsGreaterThan)
(JNIEnv* env, jclass clazz, jlong raw_ptr_to_partitioned_mesh,
 jfloat triangle_p0_x, jfloat triangle_p0_y, jfloat triangle_p1_x,
 jfloat triangle_p1_y, jfloat triangle_p2_x, jfloat triangle_p2_y,
 jfloat coverage_threshold, jfloat triangle_to_partitionedMesh_transform_a,
 jfloat triangle_to_partitionedMesh_transform_b,
 jfloat triangle_to_partitionedMesh_transform_c,
 jfloat triangle_to_partitionedMesh_transform_d,
 jfloat triangle_to_partitionedMesh_transform_e,
 jfloat triangle_to_partitionedMesh_transform_f) {
  PartitionedMesh* partitioned_mesh =
      GetPartitionedMesh(raw_ptr_to_partitioned_mesh);
  Triangle triangle{{triangle_p0_x, triangle_p0_y},
                    {triangle_p1_x, triangle_p1_y},
                    {triangle_p2_x, triangle_p2_y}};
  AffineTransform transform(triangle_to_partitionedMesh_transform_a,
                            triangle_to_partitionedMesh_transform_b,
                            triangle_to_partitionedMesh_transform_c,
                            triangle_to_partitionedMesh_transform_d,
                            triangle_to_partitionedMesh_transform_e,
                            triangle_to_partitionedMesh_transform_f);
  return partitioned_mesh->CoverageIsGreaterThan(triangle, coverage_threshold,
                                                 transform);
}

JNI_METHOD(geometry, PartitionedMeshNative, jboolean,
           partitionedMeshBoxCoverageIsGreaterThan)
(JNIEnv* env, jclass clazz, jlong raw_ptr_to_partitioned_mesh,
 jfloat rect_x_min, jfloat rect_y_min, jfloat rect_x_max, jfloat rect_y_max,
 jfloat coverage_threshold, jfloat rect_to_partitionedMesh_transform_a,
 jfloat rect_to_partitionedMesh_transform_b,
 jfloat rect_to_partitionedMesh_transform_c,
 jfloat rect_to_partitionedMesh_transform_d,
 jfloat rect_to_partitionedMesh_transform_e,
 jfloat rect_to_partitionedMesh_transform_f) {
  PartitionedMesh* partitioned_mesh =
      GetPartitionedMesh(raw_ptr_to_partitioned_mesh);
  Rect rect =
      Rect::FromTwoPoints({rect_x_min, rect_y_min}, {rect_x_max, rect_y_max});
  AffineTransform transform(
      rect_to_partitionedMesh_transform_a, rect_to_partitionedMesh_transform_b,
      rect_to_partitionedMesh_transform_c, rect_to_partitionedMesh_transform_d,
      rect_to_partitionedMesh_transform_e, rect_to_partitionedMesh_transform_f);
  return partitioned_mesh->CoverageIsGreaterThan(rect, coverage_threshold,
                                                 transform);
}

JNI_METHOD(geometry, PartitionedMeshNative, jboolean,
           partitionedMeshParallelogramCoverageIsGreaterThan)
(JNIEnv* env, jclass clazz, jlong raw_ptr_to_partitioned_mesh,
 jfloat quad_center_x, jfloat quad_center_y, jfloat quad_width,
 jfloat quad_height, jfloat quad_angle_radian, jfloat quad_shear_factor,
 jfloat coverage_threshold, jfloat quad_to_partitionedMesh_transform_a,
 jfloat quad_to_partitionedMesh_transform_b,
 jfloat quad_to_partitionedMesh_transform_c,
 jfloat quad_to_partitionedMesh_transform_d,
 jfloat quad_to_partitionedMesh_transform_e,
 jfloat quad_to_partitionedMesh_transform_f) {
  PartitionedMesh* partitioned_mesh =
      GetPartitionedMesh(raw_ptr_to_partitioned_mesh);
  Quad quad = Quad::FromCenterDimensionsRotationAndShear(
      Point{quad_center_x, quad_center_y}, quad_width, quad_height,
      Angle::Radians(quad_angle_radian), quad_shear_factor);
  AffineTransform transform(
      quad_to_partitionedMesh_transform_a, quad_to_partitionedMesh_transform_b,
      quad_to_partitionedMesh_transform_c, quad_to_partitionedMesh_transform_d,
      quad_to_partitionedMesh_transform_e, quad_to_partitionedMesh_transform_f);
  return partitioned_mesh->CoverageIsGreaterThan(quad, coverage_threshold,
                                                 transform);
}

JNI_METHOD(geometry, PartitionedMeshNative, jboolean,
           partitionedMeshPartitionedMeshCoverageIsGreaterThan)
(JNIEnv* env, jclass clazz, jlong raw_ptr_to_this_partitioned_mesh,
 jlong raw_ptr_to_other_partitioned_mesh, jfloat coverage_threshold,
 jfloat other_to_this_transform_a, jfloat other_to_this_transform_b,
 jfloat other_to_this_transform_c, jfloat other_to_this_transform_d,
 jfloat other_to_this_transform_e, jfloat other_to_this_transform_f) {
  PartitionedMesh* this_partitioned_mesh =
      GetPartitionedMesh(raw_ptr_to_this_partitioned_mesh);
  PartitionedMesh* other_partitioned_mesh =
      GetPartitionedMesh(raw_ptr_to_other_partitioned_mesh);
  AffineTransform transform(
      other_to_this_transform_a, other_to_this_transform_b,
      other_to_this_transform_c, other_to_this_transform_d,
      other_to_this_transform_e, other_to_this_transform_f);
  return this_partitioned_mesh->CoverageIsGreaterThan(
      *other_partitioned_mesh, coverage_threshold, transform);
}

JNI_METHOD(geometry, PartitionedMeshNative, void, initializeSpatialIndex)
(JNIEnv* env, jclass clazz, jlong raw_ptr_to_this_partitioned_mesh) {
  PartitionedMesh* this_partitioned_mesh =
      GetPartitionedMesh(raw_ptr_to_this_partitioned_mesh);
  this_partitioned_mesh->InitializeSpatialIndex();
}

JNI_METHOD(geometry, PartitionedMeshNative, jboolean, isSpatialIndexInitialized)
(JNIEnv* env, jclass clazz, jlong raw_ptr_to_this_partitioned_mesh) {
  const PartitionedMesh* this_partitioned_mesh =
      GetPartitionedMesh(raw_ptr_to_this_partitioned_mesh);
  return this_partitioned_mesh->IsSpatialIndexInitialized();
}

}  // extern "C"
