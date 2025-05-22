// Copyright 2025 Google LLC
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

#include "ink/jni/internal/jni_jvm_interface.h"

#include <jni.h>

#include "absl/log/absl_check.h"
#include "ink/jni/internal/jni_defines.h"

namespace ink::jni {

namespace {

static jclass class_immutable_vec = nullptr;
static jmethodID method_immutable_vec_init_x_y = nullptr;

static jclass class_mutable_vec = nullptr;
static jmethodID method_mutable_vec_set_x = nullptr;
static jmethodID method_mutable_vec_set_y = nullptr;

static jclass class_immutable_box = nullptr;
static jmethodID method_immutable_box_from_two_points = nullptr;

static jclass class_mutable_box = nullptr;
static jmethodID method_mutable_box_set_x_bounds = nullptr;
static jmethodID method_mutable_box_set_y_bounds = nullptr;

static jclass class_box_accumulator = nullptr;
static jmethodID method_box_accumulator_reset = nullptr;
static jmethodID method_box_accumulator_populate_from = nullptr;

static jclass class_immutable_parallelogram = nullptr;
static jmethodID method_immutable_parallelogram_from_center_dim_rot_shear =
    nullptr;

static jclass class_mutable_parallelogram = nullptr;
static jmethodID
    method_mutable_parallelogram_set_center_dimensions_rotation_and_shear =
        nullptr;

static jclass class_brush_native_callbacks = nullptr;
static jmethodID method_brush_native_compose_color_long_from_components =
    nullptr;

static jclass class_input_tool_type = nullptr;
static jmethodID method_input_tool_type_from = nullptr;

static jclass class_stroke_input = nullptr;
static jmethodID method_stroke_input_update = nullptr;

jmethodID GetMethodIDAndCheck(JNIEnv* env, jclass cached_class,
                              const char* method_name, const char* signature) {
  jmethodID method_id = env->GetMethodID(cached_class, method_name, signature);
  ABSL_CHECK_NE(method_id, nullptr) << "Method not found: " << method_name;
  return method_id;
}

jmethodID GetStaticMethodIDAndCheck(JNIEnv* env, jclass cached_class,
                                    const char* method_name,
                                    const char* signature) {
  jmethodID method_id =
      env->GetStaticMethodID(cached_class, method_name, signature);
  ABSL_CHECK_NE(method_id, nullptr)
      << "Static method not found: " << method_name;
  return method_id;
}

jclass FindAndCacheClass(JNIEnv* env, const char* class_name) {
  jclass cached_class = env->FindClass(class_name);
  if (cached_class == nullptr) {
    return nullptr;
  }
  return static_cast<jclass>(env->NewGlobalRef(cached_class));
}

void DeleteCachedClass(JNIEnv* env, jclass& cached_class) {
  if (cached_class != nullptr) {
    env->DeleteGlobalRef(cached_class);
    cached_class = nullptr;
  }
}

}  // namespace

void LoadJvmInterface(JNIEnv* env) {
  // The Ink C++ shared library is monolithic, but the Jetpack library is
  // modular, so some of the classes with JVM interfaces used from JNI may not
  // be present when this is initialized. Thus, cache the classes if present. If
  // a class is present, strictly check that the expected methods are available.

  class_immutable_vec =
      FindAndCacheClass(env, INK_PACKAGE "/geometry/ImmutableVec");
  if (class_immutable_vec != nullptr) {
    method_immutable_vec_init_x_y =
        GetMethodIDAndCheck(env, class_immutable_vec, "<init>", "(FF)V");
  }

  class_mutable_vec =
      FindAndCacheClass(env, INK_PACKAGE "/geometry/MutableVec");
  if (class_mutable_vec != nullptr) {
    method_mutable_vec_set_x =
        GetMethodIDAndCheck(env, class_mutable_vec, "setX", "(F)V");
    method_mutable_vec_set_y =
        GetMethodIDAndCheck(env, class_mutable_vec, "setY", "(F)V");
  }

  class_immutable_box =
      FindAndCacheClass(env, INK_PACKAGE "/geometry/ImmutableBox");
  if (class_immutable_box != nullptr) {
    method_immutable_box_from_two_points = GetStaticMethodIDAndCheck(
        env, class_immutable_box, "fromTwoPoints",
        "(L" INK_PACKAGE "/geometry/Vec;L" INK_PACKAGE
        "/geometry/Vec;)L" INK_PACKAGE "/geometry/ImmutableBox;");
  }

  class_mutable_box =
      FindAndCacheClass(env, INK_PACKAGE "/geometry/MutableBox");
  if (class_mutable_box != nullptr) {
    method_mutable_box_set_x_bounds =
        GetMethodIDAndCheck(env, class_mutable_box, "setXBounds",
                            "(FF)L" INK_PACKAGE "/geometry/MutableBox;");
    method_mutable_box_set_y_bounds =
        GetMethodIDAndCheck(env, class_mutable_box, "setYBounds",
                            "(FF)L" INK_PACKAGE "/geometry/MutableBox;");
  }

  class_box_accumulator =
      FindAndCacheClass(env, INK_PACKAGE "/geometry/BoxAccumulator");
  if (class_box_accumulator != nullptr) {
    method_box_accumulator_reset =
        GetMethodIDAndCheck(env, class_box_accumulator, "reset",
                            "()L" INK_PACKAGE "/geometry/BoxAccumulator;");
    method_box_accumulator_populate_from =
        GetMethodIDAndCheck(env, class_box_accumulator, "populateFrom",
                            "(FFFF)L" INK_PACKAGE "/geometry/BoxAccumulator;");
  }

  class_immutable_parallelogram =
      FindAndCacheClass(env, INK_PACKAGE "/geometry/ImmutableParallelogram");
  if (class_immutable_parallelogram != nullptr) {
    method_immutable_parallelogram_from_center_dim_rot_shear =
        GetStaticMethodIDAndCheck(env, class_immutable_parallelogram,
                                  "fromCenterDimensionsRotationAndShear",
                                  "(L" INK_PACKAGE
                                  "/geometry/ImmutableVec;FFFF)L" INK_PACKAGE
                                  "/geometry/ImmutableParallelogram;");
  }

  class_mutable_parallelogram =
      FindAndCacheClass(env, INK_PACKAGE "/geometry/MutableParallelogram");
  if (class_mutable_parallelogram != nullptr) {
    method_mutable_parallelogram_set_center_dimensions_rotation_and_shear =
        GetMethodIDAndCheck(env, class_mutable_parallelogram,
                            "setCenterDimensionsRotationAndShear", "(FFFFFF)V");
  }

  class_brush_native_callbacks =
      FindAndCacheClass(env, INK_PACKAGE "/brush/BrushNativeCallbacks");
  if (class_brush_native_callbacks != nullptr) {
    method_brush_native_compose_color_long_from_components =
        GetMethodIDAndCheck(env, class_brush_native_callbacks,
                            "composeColorLongFromComponents", "(IFFFF)J");
  }

  class_input_tool_type =
      FindAndCacheClass(env, INK_PACKAGE "/brush/InputToolType");
  if (class_input_tool_type != nullptr) {
    method_input_tool_type_from =
        GetStaticMethodIDAndCheck(env, class_input_tool_type, "from",
                                  "(I)L" INK_PACKAGE "/brush/InputToolType;");
  }

  class_stroke_input =
      FindAndCacheClass(env, INK_PACKAGE "/strokes/StrokeInput");
  if (class_stroke_input != nullptr) {
    method_stroke_input_update =
        GetMethodIDAndCheck(env, class_stroke_input, "update",
                            "(FFJL" INK_PACKAGE "/brush/InputToolType;FFFF)V");
  }
}

void UnloadJvmInterface(JNIEnv* env) {
  DeleteCachedClass(env, class_immutable_vec);
  DeleteCachedClass(env, class_mutable_vec);
  DeleteCachedClass(env, class_immutable_box);
  DeleteCachedClass(env, class_mutable_box);
  DeleteCachedClass(env, class_box_accumulator);
  DeleteCachedClass(env, class_immutable_parallelogram);
  DeleteCachedClass(env, class_mutable_parallelogram);
  DeleteCachedClass(env, class_brush_native_callbacks);
  DeleteCachedClass(env, class_input_tool_type);
  DeleteCachedClass(env, class_stroke_input);
}

jclass ClassImmutableVec() {
  ABSL_CHECK_NE(class_immutable_vec, nullptr);
  return class_immutable_vec;
}

jmethodID MethodImmutableVecInitXY() {
  ABSL_CHECK_NE(method_immutable_vec_init_x_y, nullptr);
  return method_immutable_vec_init_x_y;
}

jclass ClassMutableVec() {
  ABSL_CHECK_NE(class_mutable_vec, nullptr);
  return class_mutable_vec;
}

jmethodID MethodMutableVecSetX() {
  ABSL_CHECK_NE(method_mutable_vec_set_x, nullptr);
  return method_mutable_vec_set_x;
}

jmethodID MethodMutableVecSetY() {
  ABSL_CHECK_NE(method_mutable_vec_set_y, nullptr);
  return method_mutable_vec_set_y;
}

jclass ClassImmutableBox() {
  ABSL_CHECK_NE(class_immutable_box, nullptr);
  return class_immutable_box;
}

jmethodID MethodImmutableBoxFromTwoPoints() {
  ABSL_CHECK_NE(method_immutable_box_from_two_points, nullptr);
  return method_immutable_box_from_two_points;
}

jclass ClassMutableBox() {
  ABSL_CHECK_NE(class_mutable_box, nullptr);
  return class_mutable_box;
}

jmethodID MethodMutableBoxSetXBounds() {
  ABSL_CHECK_NE(method_mutable_box_set_x_bounds, nullptr);
  return method_mutable_box_set_x_bounds;
}

jmethodID MethodMutableBoxSetYBounds() {
  ABSL_CHECK_NE(method_mutable_box_set_y_bounds, nullptr);
  return method_mutable_box_set_y_bounds;
}

jclass ClassBoxAccumulator() {
  ABSL_CHECK_NE(class_box_accumulator, nullptr);
  return class_box_accumulator;
}

jmethodID MethodBoxAccumulatorReset() {
  ABSL_CHECK_NE(method_box_accumulator_reset, nullptr);
  return method_box_accumulator_reset;
}

jmethodID MethodBoxAccumulatorPopulateFrom() {
  ABSL_CHECK_NE(method_box_accumulator_populate_from, nullptr);
  return method_box_accumulator_populate_from;
}

jclass ClassImmutableParallelogram() {
  ABSL_CHECK_NE(class_immutable_parallelogram, nullptr);
  return class_immutable_parallelogram;
}

jmethodID MethodImmutableParallelogramFromCenterDimRotShear() {
  ABSL_CHECK_NE(method_immutable_parallelogram_from_center_dim_rot_shear,
                nullptr);
  return method_immutable_parallelogram_from_center_dim_rot_shear;
}

jclass ClassMutableParallelogram() {
  ABSL_CHECK_NE(class_mutable_parallelogram, nullptr);
  return class_mutable_parallelogram;
}

jmethodID MethodMutableParallelogramSetCenterDimensionsRotationAndShear() {
  ABSL_CHECK_NE(
      method_mutable_parallelogram_set_center_dimensions_rotation_and_shear,
      nullptr);
  return method_mutable_parallelogram_set_center_dimensions_rotation_and_shear;
}

jclass ClassBrushNative() {
  ABSL_CHECK_NE(class_brush_native_callbacks, nullptr);
  return class_brush_native_callbacks;
}

jmethodID MethodBrushNativeComposeColorLongFromComponents() {
  ABSL_CHECK_NE(method_brush_native_compose_color_long_from_components,
                nullptr);
  return method_brush_native_compose_color_long_from_components;
}

jclass ClassInputToolType() {
  ABSL_CHECK_NE(class_input_tool_type, nullptr);
  return class_input_tool_type;
}

jmethodID MethodInputToolTypeFrom() {
  ABSL_CHECK_NE(method_input_tool_type_from, nullptr);
  return method_input_tool_type_from;
}

jclass ClassStrokeInput() {
  ABSL_CHECK_NE(class_stroke_input, nullptr);
  return class_stroke_input;
}

jmethodID MethodStrokeInputUpdate() {
  ABSL_CHECK_NE(method_stroke_input_update, nullptr);
  return method_stroke_input_update;
}

}  // namespace ink::jni
