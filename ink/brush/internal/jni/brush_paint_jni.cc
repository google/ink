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

#include <vector>

#include "absl/status/status.h"
#include "absl/strings/string_view.h"
#include "ink/brush/brush_paint.h"
#include "ink/brush/internal/jni/brush_jni_helper.h"
#include "ink/geometry/angle.h"
#include "ink/geometry/vec.h"
#include "ink/jni/internal/jni_defines.h"
#include "ink/jni/internal/jni_string_util.h"
#include "ink/jni/internal/jni_throw_util.h"
#include "ink/types/uri.h"

namespace {

ink::BrushPaint::TextureSizeUnit JIntToSizeUnit(jint val) {
  return static_cast<ink::BrushPaint::TextureSizeUnit>(val);
}

ink::BrushPaint::TextureOrigin JIntToOrigin(jint val) {
  return static_cast<ink::BrushPaint::TextureOrigin>(val);
}

ink::BrushPaint::TextureMapping JIntToMapping(jint val) {
  return static_cast<ink::BrushPaint::TextureMapping>(val);
}

ink::BrushPaint::TextureWrap JIntToWrap(jint val) {
  return static_cast<ink::BrushPaint::TextureWrap>(val);
}

ink::BrushPaint::BlendMode JIntToBlendMode(jint val) {
  return static_cast<ink::BrushPaint::BlendMode>(val);
}

}  // namespace

extern "C" {

// Construct a native BrushPaint and return a pointer to it as a long.
JNI_METHOD(brush, BrushPaint, jlong, nativeCreateBrushPaint)
(JNIEnv* env, jobject thiz, jint texture_layers_count) {
  std::vector<ink::BrushPaint::TextureLayer> texture_layers;
  texture_layers.reserve(texture_layers_count);
  return reinterpret_cast<jlong>(
      new ink::BrushPaint{.texture_layers = texture_layers});
}

// Appends a texture layer to a *mutable* C++ BrushPaint object as referenced by
// `native_pointer`. Only call this method within kotlin init{} scope so to keep
// this BrushPaint object immutable after construction and equivalent across
// Kotlin and C++.
JNI_METHOD(brush, BrushPaint, void, nativeAppendTextureLayer)
(JNIEnv* env, jobject thiz, jlong native_pointer, jlong texture_layer_pointer) {
  // Intentionally non-const casting to BrushPaint in this rare native method
  // called during kotlin init scope.
  auto* brush_paint = reinterpret_cast<ink::BrushPaint*>(native_pointer);
  const ink::BrushPaint::TextureLayer texture_layer =
      *reinterpret_cast<ink::BrushPaint::TextureLayer*>(texture_layer_pointer);
  brush_paint->texture_layers.push_back(texture_layer);
  absl::Status status = ink::brush_internal::ValidateBrushPaint(*brush_paint);

  if (!status.ok()) {
    ink::jni::ThrowExceptionFromStatus(env, status);
    return;
  }
}

JNI_METHOD(brush, BrushPaint, void, nativeFreeBrushPaint)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  delete reinterpret_cast<ink::BrushPaint*>(native_pointer);
}

// ************ Native Implementation of BrushPaint TextureLayer ************

// Constructs a native BrushPaint TextureLayer and returns a pointer to it as a
// long.
JNI_METHOD_INNER(brush, BrushPaint, TextureLayer, jlong,
                 nativeCreateTextureLayer)
(JNIEnv* env, jobject thiz, jstring color_texture_uri, jfloat size_x,
 jfloat size_y, jfloat offset_x, jfloat offset_y, jfloat rotation_in_radians,
 jfloat opacity, jint animation_frames, jint size_unit, jint origin,
 jint mapping, jint wrap_x, jint wrap_y, jint blend_mode) {
  auto uri = ink::Uri::Parse(
      ink::jni::JStringView(env, color_texture_uri).string_view());
  if (!uri.ok()) {
    ink::jni::ThrowExceptionFromStatus(env, uri.status());
    return -1;  // Invalid Uri.
  }

  ink::BrushPaint::TextureLayer* texture_layer =
      new ink::BrushPaint::TextureLayer{
          .color_texture_uri = *uri,
          .mapping = JIntToMapping(mapping),
          .origin = JIntToOrigin(origin),
          .size_unit = JIntToSizeUnit(size_unit),
          .wrap_x = JIntToWrap(wrap_x),
          .wrap_y = JIntToWrap(wrap_y),
          .size = ink::Vec{size_x, size_y},
          .offset = ink::Vec{offset_x, offset_y},
          .rotation = ink::Angle::Radians(rotation_in_radians),
          .opacity = opacity,
          .animation_frames = animation_frames,
          .blend_mode = JIntToBlendMode(blend_mode),
      };

  absl::Status status =
      ink::brush_internal::ValidateBrushPaintTextureLayer(*texture_layer);

  if (!status.ok()) {
    ink::jni::ThrowExceptionFromStatus(env, status);
    // delete texture_layer;
    return -1;
  }
  return reinterpret_cast<jlong>(texture_layer);
}

JNI_METHOD_INNER(brush, BrushPaint, TextureLayer, void, nativeFreeTextureLayer)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  delete reinterpret_cast<ink::BrushPaint::TextureLayer*>(native_pointer);
}

}  // extern "C"
