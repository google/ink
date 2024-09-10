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
#include "ink/geometry/modeled_shape.h"
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
using ::ink::ModeledShape;
using ::ink::Point;
using ::ink::Quad;
using ::ink::Rect;
using ::ink::Triangle;

ModeledShape* GetModeledShape(jlong raw_ptr_to_modeled_shape) {
  return reinterpret_cast<ModeledShape*>(raw_ptr_to_modeled_shape);
}

}  // namespace

extern "C" {

// Free the given `ModeledShape`.
JNI_METHOD(geometry, ModeledShapeNative, void, free)
(JNIEnv* env, jclass clazz, jlong raw_ptr_to_modeled_shape) {
  delete GetModeledShape(raw_ptr_to_modeled_shape);
}

JNI_METHOD(geometry, ModeledShapeNative, jlongArray, getNativeAddressesOfMeshes)
(JNIEnv* env, jclass clazz, jlong raw_ptr_to_modeled_shape, jint group_index) {
  absl::Span<const Mesh> meshes =
      GetModeledShape(raw_ptr_to_modeled_shape)->RenderGroupMeshes(group_index);
  std::vector<jlong> meshes_ptrs(meshes.size());
  for (int i = 0; i < meshes.size(); ++i) {
    // Create new heap-allocated copies of each `Mesh` object, to be owned by
    // the `Mesh.kt` objects of the `ModeledShape.kt` that is under
    // construction. The C++ `Mesh` type is cheap to copy because internally it
    // keeps a `shared_ptr` to its (immutable) data.
    meshes_ptrs[i] = reinterpret_cast<jlong>(new Mesh(meshes[i]));
  }
  jlongArray meshes_addresses = env->NewLongArray(meshes.size());
  env->SetLongArrayRegion(meshes_addresses, 0, meshes.size(),
                          meshes_ptrs.data());
  return meshes_addresses;
}

JNI_METHOD(geometry, ModeledShapeNative, jint, getRenderGroupCount)
(JNIEnv* env, jclass clazz, jlong raw_ptr_to_modeled_shape) {
  return GetModeledShape(raw_ptr_to_modeled_shape)->RenderGroupCount();
}

JNI_METHOD(geometry, ModeledShapeNative, jlong, getRenderGroupFormat)
(JNIEnv* env, jclass clazz, jlong raw_ptr_to_modeled_shape, jint group_index) {
  const ModeledShape* modeled_shape = GetModeledShape(raw_ptr_to_modeled_shape);
  auto mesh_format_ptr = std::make_unique<MeshFormat>(
      modeled_shape->RenderGroupFormat(group_index));
  return reinterpret_cast<jlong>(mesh_format_ptr.release());
}

JNI_METHOD(geometry, ModeledShapeNative, jint, getOutlineCount)
(JNIEnv* env, jclass clazz, jlong raw_ptr_to_modeled_shape, jint group_index) {
  return GetModeledShape(raw_ptr_to_modeled_shape)->OutlineCount(group_index);
}

JNI_METHOD(geometry, ModeledShapeNative, jint, getOutlineVertexCount)
(JNIEnv* env, jclass clazz, jlong raw_ptr_to_modeled_shape, jint group_index,
 jint outline_index) {
  const ModeledShape* modeled_shape = GetModeledShape(raw_ptr_to_modeled_shape);
  return modeled_shape->Outline(group_index, outline_index).size();
}

JNI_METHOD(geometry, ModeledShapeNative, void,
           fillOutlineMeshIndexAndMeshVertexIndex)
(JNIEnv* env, jclass clazz, jlong raw_ptr_to_modeled_shape, jint group_index,
 jint outline_index, jint outline_vertex_index,
 jintArray out_mesh_index_and_mesh_vertex_index) {
  const ModeledShape* modeled_shape = GetModeledShape(raw_ptr_to_modeled_shape);
  absl::Span<const ModeledShape::VertexIndexPair> outline =
      modeled_shape->Outline(group_index, outline_index);
  ModeledShape::VertexIndexPair index_pair = outline[outline_vertex_index];
  jint mesh_index_and_mesh_vertex_index[] = {index_pair.mesh_index,
                                             index_pair.vertex_index};
  env->SetIntArrayRegion(out_mesh_index_and_mesh_vertex_index, 0, 2,
                         mesh_index_and_mesh_vertex_index);
}

// Allocate an empty `ModeledShape` and return a pointer to it.
JNI_METHOD(geometry, ModeledShapeNative, jlong, alloc)
(JNIEnv* env, jclass clazz) {
  auto modeled_shape = std::make_unique<ModeledShape>();
  return reinterpret_cast<jlong>(modeled_shape.release());
}

JNI_METHOD(geometry, ModeledShapeNative, jfloat, modeledShapeTriangleCoverage)
(JNIEnv* env, jclass clazz, jlong raw_ptr_to_modeled_shape,
 jfloat triangle_p0_x, jfloat triangle_p0_y, jfloat triangle_p1_x,
 jfloat triangle_p1_y, jfloat triangle_p2_x, jfloat triangle_p2_y,
 jfloat triangle_to_modeledShape_transform_a,
 jfloat triangle_to_modeledShape_transform_b,
 jfloat triangle_to_modeledShape_transform_c,
 jfloat triangle_to_modeledShape_transform_d,
 jfloat triangle_to_modeledShape_transform_e,
 jfloat triangle_to_modeledShape_transform_f) {
  ModeledShape* modeled_shape = GetModeledShape(raw_ptr_to_modeled_shape);
  Triangle triangle{{triangle_p0_x, triangle_p0_y},
                    {triangle_p1_x, triangle_p1_y},
                    {triangle_p2_x, triangle_p2_y}};
  AffineTransform transform(triangle_to_modeledShape_transform_a,
                            triangle_to_modeledShape_transform_b,
                            triangle_to_modeledShape_transform_c,
                            triangle_to_modeledShape_transform_d,
                            triangle_to_modeledShape_transform_e,
                            triangle_to_modeledShape_transform_f);
  return modeled_shape->Coverage(triangle, transform);
}

JNI_METHOD(geometry, ModeledShapeNative, jfloat, modeledShapeBoxCoverage)
(JNIEnv* env, jclass clazz, jlong raw_ptr_to_modeled_shape, jfloat rect_x_min,
 jfloat rect_y_min, jfloat rect_x_max, jfloat rect_y_max,
 jfloat rect_to_modeledShape_transform_a,
 jfloat rect_to_modeledShape_transform_b,
 jfloat rect_to_modeledShape_transform_c,
 jfloat rect_to_modeledShape_transform_d,
 jfloat rect_to_modeledShape_transform_e,
 jfloat rect_to_modeledShape_transform_f) {
  ModeledShape* modeled_shape = GetModeledShape(raw_ptr_to_modeled_shape);
  Rect rect =
      Rect::FromTwoPoints({rect_x_min, rect_y_min}, {rect_x_max, rect_y_max});
  AffineTransform transform(
      rect_to_modeledShape_transform_a, rect_to_modeledShape_transform_b,
      rect_to_modeledShape_transform_c, rect_to_modeledShape_transform_d,
      rect_to_modeledShape_transform_e, rect_to_modeledShape_transform_f);
  return modeled_shape->Coverage(rect, transform);
}

JNI_METHOD(geometry, ModeledShapeNative, jfloat,
           modeledShapeParallelogramCoverage)
(JNIEnv* env, jclass clazz, jlong raw_ptr_to_modeled_shape,
 jfloat quad_center_x, jfloat quad_center_y, jfloat quad_width,
 jfloat quad_height, jfloat quad_angle_radian, jfloat quad_shear_factor,
 jfloat quad_to_modeledShape_transform_a,
 jfloat quad_to_modeledShape_transform_b,
 jfloat quad_to_modeledShape_transform_c,
 jfloat quad_to_modeledShape_transform_d,
 jfloat quad_to_modeledShape_transform_e,
 jfloat quad_to_modeledShape_transform_f) {
  ModeledShape* modeled_shape = GetModeledShape(raw_ptr_to_modeled_shape);
  Quad quad = Quad::FromCenterDimensionsRotationAndShear(
      Point{quad_center_x, quad_center_y}, quad_width, quad_height,
      Angle::Radians(quad_angle_radian), quad_shear_factor);
  AffineTransform transform(
      quad_to_modeledShape_transform_a, quad_to_modeledShape_transform_b,
      quad_to_modeledShape_transform_c, quad_to_modeledShape_transform_d,
      quad_to_modeledShape_transform_e, quad_to_modeledShape_transform_f);
  return modeled_shape->Coverage(quad, transform);
}

JNI_METHOD(geometry, ModeledShapeNative, jfloat,
           modeledShapeModeledShapeCoverage)
(JNIEnv* env, jclass clazz, jlong raw_ptr_to_this_modeled_shape,
 jlong raw_ptr_to_other_modeled_shape, jfloat other_to_this_transform_a,
 jfloat other_to_this_transform_b, jfloat other_to_this_transform_c,
 jfloat other_to_this_transform_d, jfloat other_to_this_transform_e,
 jfloat other_to_this_transform_f) {
  ModeledShape* this_modeled_shape =
      GetModeledShape(raw_ptr_to_this_modeled_shape);
  ModeledShape* other_modeled_shape =
      GetModeledShape(raw_ptr_to_other_modeled_shape);
  AffineTransform transform(
      other_to_this_transform_a, other_to_this_transform_b,
      other_to_this_transform_c, other_to_this_transform_d,
      other_to_this_transform_e, other_to_this_transform_f);
  return this_modeled_shape->Coverage(*other_modeled_shape, transform);
}

JNI_METHOD(geometry, ModeledShapeNative, jboolean,
           modeledShapeTriangleCoverageIsGreaterThan)
(JNIEnv* env, jclass clazz, jlong raw_ptr_to_modeled_shape,
 jfloat triangle_p0_x, jfloat triangle_p0_y, jfloat triangle_p1_x,
 jfloat triangle_p1_y, jfloat triangle_p2_x, jfloat triangle_p2_y,
 jfloat coverage_threshold, jfloat triangle_to_modeledShape_transform_a,
 jfloat triangle_to_modeledShape_transform_b,
 jfloat triangle_to_modeledShape_transform_c,
 jfloat triangle_to_modeledShape_transform_d,
 jfloat triangle_to_modeledShape_transform_e,
 jfloat triangle_to_modeledShape_transform_f) {
  ModeledShape* modeled_shape = GetModeledShape(raw_ptr_to_modeled_shape);
  Triangle triangle{{triangle_p0_x, triangle_p0_y},
                    {triangle_p1_x, triangle_p1_y},
                    {triangle_p2_x, triangle_p2_y}};
  AffineTransform transform(triangle_to_modeledShape_transform_a,
                            triangle_to_modeledShape_transform_b,
                            triangle_to_modeledShape_transform_c,
                            triangle_to_modeledShape_transform_d,
                            triangle_to_modeledShape_transform_e,
                            triangle_to_modeledShape_transform_f);
  return modeled_shape->CoverageIsGreaterThan(triangle, coverage_threshold,
                                              transform);
}

JNI_METHOD(geometry, ModeledShapeNative, jboolean,
           modeledShapeBoxCoverageIsGreaterThan)
(JNIEnv* env, jclass clazz, jlong raw_ptr_to_modeled_shape, jfloat rect_x_min,
 jfloat rect_y_min, jfloat rect_x_max, jfloat rect_y_max,
 jfloat coverage_threshold, jfloat rect_to_modeledShape_transform_a,
 jfloat rect_to_modeledShape_transform_b,
 jfloat rect_to_modeledShape_transform_c,
 jfloat rect_to_modeledShape_transform_d,
 jfloat rect_to_modeledShape_transform_e,
 jfloat rect_to_modeledShape_transform_f) {
  ModeledShape* modeled_shape = GetModeledShape(raw_ptr_to_modeled_shape);
  Rect rect =
      Rect::FromTwoPoints({rect_x_min, rect_y_min}, {rect_x_max, rect_y_max});
  AffineTransform transform(
      rect_to_modeledShape_transform_a, rect_to_modeledShape_transform_b,
      rect_to_modeledShape_transform_c, rect_to_modeledShape_transform_d,
      rect_to_modeledShape_transform_e, rect_to_modeledShape_transform_f);
  return modeled_shape->CoverageIsGreaterThan(rect, coverage_threshold,
                                              transform);
}

JNI_METHOD(geometry, ModeledShapeNative, jboolean,
           modeledShapeParallelogramCoverageIsGreaterThan)
(JNIEnv* env, jclass clazz, jlong raw_ptr_to_modeled_shape,
 jfloat quad_center_x, jfloat quad_center_y, jfloat quad_width,
 jfloat quad_height, jfloat quad_angle_radian, jfloat quad_shear_factor,
 jfloat coverage_threshold, jfloat quad_to_modeledShape_transform_a,
 jfloat quad_to_modeledShape_transform_b,
 jfloat quad_to_modeledShape_transform_c,
 jfloat quad_to_modeledShape_transform_d,
 jfloat quad_to_modeledShape_transform_e,
 jfloat quad_to_modeledShape_transform_f) {
  ModeledShape* modeled_shape = GetModeledShape(raw_ptr_to_modeled_shape);
  Quad quad = Quad::FromCenterDimensionsRotationAndShear(
      Point{quad_center_x, quad_center_y}, quad_width, quad_height,
      Angle::Radians(quad_angle_radian), quad_shear_factor);
  AffineTransform transform(
      quad_to_modeledShape_transform_a, quad_to_modeledShape_transform_b,
      quad_to_modeledShape_transform_c, quad_to_modeledShape_transform_d,
      quad_to_modeledShape_transform_e, quad_to_modeledShape_transform_f);
  return modeled_shape->CoverageIsGreaterThan(quad, coverage_threshold,
                                              transform);
}

JNI_METHOD(geometry, ModeledShapeNative, jboolean,
           modeledShapeModeledShapeCoverageIsGreaterThan)
(JNIEnv* env, jclass clazz, jlong raw_ptr_to_this_modeled_shape,
 jlong raw_ptr_to_other_modeled_shape, jfloat coverage_threshold,
 jfloat other_to_this_transform_a, jfloat other_to_this_transform_b,
 jfloat other_to_this_transform_c, jfloat other_to_this_transform_d,
 jfloat other_to_this_transform_e, jfloat other_to_this_transform_f) {
  ModeledShape* this_modeled_shape =
      GetModeledShape(raw_ptr_to_this_modeled_shape);
  ModeledShape* other_modeled_shape =
      GetModeledShape(raw_ptr_to_other_modeled_shape);
  AffineTransform transform(
      other_to_this_transform_a, other_to_this_transform_b,
      other_to_this_transform_c, other_to_this_transform_d,
      other_to_this_transform_e, other_to_this_transform_f);
  return this_modeled_shape->CoverageIsGreaterThan(
      *other_modeled_shape, coverage_threshold, transform);
}

JNI_METHOD(geometry, ModeledShapeNative, void, initializeSpatialIndex)
(JNIEnv* env, jclass clazz, jlong raw_ptr_to_this_modeled_shape) {
  ModeledShape* this_modeled_shape =
      GetModeledShape(raw_ptr_to_this_modeled_shape);
  this_modeled_shape->InitializeSpatialIndex();
}

JNI_METHOD(geometry, ModeledShapeNative, jboolean, isSpatialIndexInitialized)
(JNIEnv* env, jclass clazz, jlong raw_ptr_to_this_modeled_shape) {
  const ModeledShape* this_modeled_shape =
      GetModeledShape(raw_ptr_to_this_modeled_shape);
  return this_modeled_shape->IsSpatialIndexInitialized();
}

}  // extern "C"
