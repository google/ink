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
#include <string>

#include "ink/brush/internal/jni/brush_paint_native.h"
#include "ink/color/internal/jni/color_jni_helper.h"
#include "ink/jni/internal/jni_defines.h"
#include "ink/jni/internal/jni_string_util.h"
#include "ink/jni/internal/status_jni_helper.h"

using ::ink::jni::ComposeColorLongFromComponentsCallback;
using ::ink::jni::JStringToStdString;
using ::ink::jni::ThrowExceptionFromStatusCallback;

extern "C" {

JNI_METHOD(brush, BrushPaintNative, jlong, create)
(JNIEnv* env, jobject thiz, jlongArray texture_layer_native_pointers_array,
 jlongArray color_function_native_pointers_array, jint self_overlap_int) {
  const jsize texture_layers_count =
      env->GetArrayLength(texture_layer_native_pointers_array);
  jlong* texture_layer_native_pointers =
      env->GetLongArrayElements(texture_layer_native_pointers_array, nullptr);

  const jsize color_functions_count =
      env->GetArrayLength(color_function_native_pointers_array);
  jlong* color_function_native_pointers =
      env->GetLongArrayElements(color_function_native_pointers_array, nullptr);

  // Both `jlong` and `int64_t` are required to be 64-bit integers which JNI and
  // Kotlin-cinterop respectively both map to Kotlin `Long`. However, on MacOS
  // they represent two distinct (though equivalent) types, `jlong` is `long`
  // but `int64_t` is `long long`.
  static_assert(sizeof(jlong) == sizeof(int64_t));
  int64_t result = BrushPaintNative_create(
      env, reinterpret_cast<int64_t*>(texture_layer_native_pointers),
      texture_layers_count,
      reinterpret_cast<int64_t*>(color_function_native_pointers),
      color_functions_count, self_overlap_int,
      &ThrowExceptionFromStatusCallback);

  env->ReleaseLongArrayElements(texture_layer_native_pointers_array,
                                texture_layer_native_pointers, JNI_ABORT);
  env->ReleaseLongArrayElements(color_function_native_pointers_array,
                                color_function_native_pointers, JNI_ABORT);

  return result;
}

JNI_METHOD(brush, BrushPaintNative, void, free)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  BrushPaintNative_free(native_pointer);
}

JNI_METHOD(brush, BrushPaintNative, jint, getTextureLayerCount)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return BrushPaintNative_getTextureLayerCount(native_pointer);
}

JNI_METHOD(brush, BrushPaintNative, jint, getTextureLayerMappingInt)
(JNIEnv* env, jobject thiz, jlong native_pointer, jint index) {
  return BrushPaintNative_getTextureLayerMappingInt(native_pointer, index);
}

JNI_METHOD(brush, BrushPaintNative, jlong, newCopyOfTextureLayer)
(JNIEnv* env, jobject thiz, jlong native_pointer, jint index) {
  return BrushPaintNative_newCopyOfTextureLayer(native_pointer, index);
}

JNI_METHOD(brush, BrushPaintNative, jint, getColorFunctionCount)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return BrushPaintNative_getColorFunctionCount(native_pointer);
}

JNI_METHOD(brush, BrushPaintNative, jint, getColorFunctionParametersTypeInt)
(JNIEnv* env, jobject thiz, jlong native_pointer, jint index) {
  return BrushPaintNative_getColorFunctionParametersTypeInt(native_pointer,
                                                            index);
}

JNI_METHOD(brush, BrushPaintNative, jlong, newCopyOfColorFunction)
(JNIEnv* env, jobject thiz, jlong native_pointer, jint index) {
  return BrushPaintNative_newCopyOfColorFunction(native_pointer, index);
}

JNI_METHOD(brush, BrushPaintNative, jint, getSelfOverlapInt)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return BrushPaintNative_getSelfOverlapInt(native_pointer);
}

JNI_METHOD(brush, BrushPaintNative, jboolean, isCompatibleWithMeshFormat)
(JNIEnv* env, jobject obj, jlong native_pointer,
 jlong mesh_format_native_pointer) {
  return BrushPaintNative_isCompatibleWithMeshFormat(
      native_pointer, mesh_format_native_pointer);
}

// Native Implementation of BrushPaint.TextureLayer:

JNI_METHOD(brush, TilingTextureNative, jlong, create)
(JNIEnv* env, jobject thiz, jstring client_texture_id, jfloat size_x,
 jfloat size_y, jfloat offset_x, jfloat offset_y, jfloat rotation_degrees,
 jint size_unit, jint origin, jint wrap_x, jint wrap_y, jint blend_mode) {
  std::string client_texture_id_str =
      JStringToStdString(env, client_texture_id);
  return TilingTextureNative_create(
      env, client_texture_id_str.c_str(), size_x, size_y, offset_x, offset_y,
      rotation_degrees, size_unit, origin, wrap_x, wrap_y, blend_mode,
      &ThrowExceptionFromStatusCallback);
}

JNI_METHOD(brush, StampingTextureNative, jlong, create)
(JNIEnv* env, jobject thiz, jstring client_texture_id, jint animation_frames,
 jint animation_rows, jint animation_columns, jlong animation_duration_millis,
 jint blend_mode) {
  std::string client_texture_id_str =
      JStringToStdString(env, client_texture_id);
  return StampingTextureNative_create(
      env, client_texture_id_str.c_str(), animation_frames, animation_rows,
      animation_columns, animation_duration_millis, blend_mode,
      &ThrowExceptionFromStatusCallback);
}

JNI_METHOD(brush, TextureLayerNative, void, free)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  TextureLayerNative_free(native_pointer);
}

JNI_METHOD(brush, TextureLayerNative, jstring, getClientTextureId)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return env->NewStringUTF(
      TextureLayerNative_getClientTextureId(native_pointer));
}

JNI_METHOD(brush, TilingTextureNative, jfloat, getSizeX)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return TilingTextureNative_getSizeX(native_pointer);
}

JNI_METHOD(brush, TilingTextureNative, jfloat, getSizeY)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return TilingTextureNative_getSizeY(native_pointer);
}

JNI_METHOD(brush, TilingTextureNative, jfloat, getOffsetX)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return TilingTextureNative_getOffsetX(native_pointer);
}

JNI_METHOD(brush, TilingTextureNative, jfloat, getOffsetY)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return TilingTextureNative_getOffsetY(native_pointer);
}

JNI_METHOD(brush, TilingTextureNative, jfloat, getRotationDegrees)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return TilingTextureNative_getRotationDegrees(native_pointer);
}

JNI_METHOD(brush, StampingTextureNative, jint, getAnimationFrames)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return StampingTextureNative_getAnimationFrames(native_pointer);
}

JNI_METHOD(brush, StampingTextureNative, jint, getAnimationRows)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return StampingTextureNative_getAnimationRows(native_pointer);
}

JNI_METHOD(brush, StampingTextureNative, jint, getAnimationColumns)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return StampingTextureNative_getAnimationColumns(native_pointer);
}

JNI_METHOD(brush, StampingTextureNative, jlong, getAnimationDurationMillis)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return StampingTextureNative_getAnimationDurationMillis(native_pointer);
}

JNI_METHOD(brush, TilingTextureNative, jint, getSizeUnitInt)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return TilingTextureNative_getSizeUnitInt(native_pointer);
}

JNI_METHOD(brush, TilingTextureNative, jint, getOriginInt)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return TilingTextureNative_getOriginInt(native_pointer);
}

JNI_METHOD(brush, TextureLayerNative, jint, getMappingInt)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return TextureLayerNative_getMappingInt(native_pointer);
}

JNI_METHOD(brush, TilingTextureNative, jint, getWrapXInt)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return TilingTextureNative_getWrapXInt(native_pointer);
}

JNI_METHOD(brush, TilingTextureNative, jint, getWrapYInt)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return TilingTextureNative_getWrapYInt(native_pointer);
}

JNI_METHOD(brush, TextureLayerNative, jint, getBlendModeInt)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return TextureLayerNative_getBlendModeInt(native_pointer);
}

// Native Implementation of BrushPaint.ColorFunction

JNI_METHOD(brush, ColorFunctionNative, jlong, createOpacityMultiplier)
(JNIEnv* env, jobject thiz, jfloat multiplier) {
  return ColorFunctionNative_createOpacityMultiplier(
      env, multiplier, &ThrowExceptionFromStatusCallback);
}

JNI_METHOD(brush, ColorFunctionNative, jlong, createReplaceColor)
(JNIEnv* env, jobject thiz, jfloat color_red, jfloat color_green,
 jfloat color_blue, jfloat color_alpha, jint color_space_id) {
  return ColorFunctionNative_createReplaceColor(
      env, color_red, color_green, color_blue, color_alpha, color_space_id,
      &ThrowExceptionFromStatusCallback);
}

JNI_METHOD(brush, ColorFunctionNative, void, free)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  ColorFunctionNative_free(native_pointer);
}

JNI_METHOD(brush, ColorFunctionNative, jfloat, getOpacityMultiplier)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return ColorFunctionNative_getOpacityMultiplier(native_pointer);
}

JNI_METHOD(brush, ColorFunctionNative, jlong, computeReplaceColorLong)
(JNIEnv* env, jobject object, jlong native_pointer) {
  return ColorFunctionNative_computeReplaceColorLong(
      env, native_pointer, &ComposeColorLongFromComponentsCallback);
}

}  // extern "C"
