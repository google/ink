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

#ifndef INK_BRUSH_INTERNAL_JNI_BRUSH_NATIVE_HELPER_H_
#define INK_BRUSH_INTERNAL_JNI_BRUSH_NATIVE_HELPER_H_

#include <cstdint>

#include "absl/log/absl_check.h"
#include "absl/status/statusor.h"
#include "ink/brush/brush.h"
#include "ink/brush/brush_behavior.h"
#include "ink/brush/brush_coat.h"
#include "ink/brush/brush_family.h"
#include "ink/brush/brush_paint.h"
#include "ink/brush/brush_tip.h"
#include "ink/brush/color_function.h"
#include "ink/brush/easing_function.h"
#include "ink/brush/version.h"

namespace ink::native {

// Creates a new heap-allocated copy of the given Brush and returns a pointer
// to it as a int64_t, suitable for wrapping in a Kotlin Brush.
inline int64_t NewNativeBrush(const Brush& brush) {
  return reinterpret_cast<int64_t>(new Brush(brush));
}

// Casts a Kotlin Brush.nativePointer to a C++ Brush. The returned
// Brush is a const ref as the Kotlin Brush is immutable.
inline const Brush& CastToBrush(int64_t brush_native_pointer) {
  ABSL_CHECK_NE(brush_native_pointer, 0);
  return *reinterpret_cast<Brush*>(brush_native_pointer);
}

// Frees a Kotlin Brush.nativePointer.
inline void DeleteNativeBrush(int64_t brush_native_pointer) {
  if (brush_native_pointer == 0) return;
  delete reinterpret_cast<Brush*>(brush_native_pointer);
}

// Creates a new heap-allocated copy of the given BrushFamily and returns a
// pointer to it as a int64_t, suitable for wrapping in a Kotlin BrushFamily.
inline int64_t NewNativeBrushFamily(const BrushFamily& brush_family) {
  return reinterpret_cast<int64_t>(new BrushFamily(brush_family));
}

// Casts a Kotlin BrushFamily.nativePointer to a C++ BrushFamily. The
// returned BrushFamily is a const ref as the Kotlin BrushFamily is immutable.
inline const BrushFamily& CastToBrushFamily(
    int64_t brush_family_native_pointer) {
  ABSL_CHECK_NE(brush_family_native_pointer, 0);
  return *reinterpret_cast<BrushFamily*>(brush_family_native_pointer);
}

// Frees a Kotlin BrushFamily.nativePointer.
inline void DeleteNativeBrushFamily(int64_t brush_family_native_pointer) {
  if (brush_family_native_pointer == 0) return;
  delete reinterpret_cast<BrushFamily*>(brush_family_native_pointer);
}

// Creates a new heap-allocated copy of the given InputModel and returns a
// pointer to it as a int64_t, suitable for wrapping in a Kotlin InputModel.
inline int64_t NewNativeInputModel(const BrushFamily::InputModel& input_model) {
  return reinterpret_cast<int64_t>(new BrushFamily::InputModel(input_model));
}

// Casts a Kotlin InputModel.nativePointer to a C++ InputModel. The
// returned InputModel is a const ref as the Kotlin InputModel is immutable.
inline const BrushFamily::InputModel& CastToInputModel(
    int64_t input_model_native_pointer) {
  ABSL_CHECK_NE(input_model_native_pointer, 0);
  return *reinterpret_cast<BrushFamily::InputModel*>(
      input_model_native_pointer);
}

// Frees a Kotlin InputModel.nativePointer.
inline void DeleteNativeInputModel(int64_t input_model_native_pointer) {
  if (input_model_native_pointer == 0) return;
  delete reinterpret_cast<BrushFamily::InputModel*>(input_model_native_pointer);
}

// Creates a new heap-allocated copy of the given BrushCoat and returns a
// pointer to it as a int64_t, suitable for wrapping in a Kotlin BrushCoat.
inline int64_t NewNativeBrushCoat(const BrushCoat& brush_coat) {
  return reinterpret_cast<int64_t>(new BrushCoat(brush_coat));
}

// Casts a Kotlin BrushCoat.nativePointer to a C++ BrushCoat. The returned
// BrushCoat is a const ref as the Kotlin BrushCoat is immutable.
inline const BrushCoat& CastToBrushCoat(int64_t brush_coat_native_pointer) {
  ABSL_CHECK_NE(brush_coat_native_pointer, 0);
  return *reinterpret_cast<BrushCoat*>(brush_coat_native_pointer);
}

// Frees a Kotlin BrushCoat.nativePointer.
inline void DeleteNativeBrushCoat(int64_t brush_coat_native_pointer) {
  if (brush_coat_native_pointer == 0) return;
  delete reinterpret_cast<BrushCoat*>(brush_coat_native_pointer);
}

// Creates a new heap-allocated copy of the given BrushPaint and returns a
// pointer to it as a int64_t, suitable for wrapping in a Kotlin BrushPaint.
inline int64_t NewNativeBrushPaint(const BrushPaint& brush_paint) {
  return reinterpret_cast<int64_t>(new BrushPaint(brush_paint));
}

// Casts a Kotlin BrushPaint.nativePointer to a C++ BrushPaint. The returned
// BrushPaint is a const ref as the Kotlin BrushPaint is immutable.
inline const BrushPaint& CastToBrushPaint(int64_t brush_paint_native_pointer) {
  ABSL_CHECK_NE(brush_paint_native_pointer, 0);
  return *reinterpret_cast<BrushPaint*>(brush_paint_native_pointer);
}

// Frees a Kotlin BrushPaint.nativePointer.
inline void DeleteNativeBrushPaint(int64_t brush_paint_native_pointer) {
  if (brush_paint_native_pointer == 0) return;
  delete reinterpret_cast<BrushPaint*>(brush_paint_native_pointer);
}

// Creates a new heap-allocated copy of the given BrushPaint::TextureLayer and
// returns a pointer to it as a int64_t, suitable for wrapping in a Kotlin
// BrushPaint.TextureLayer.
inline int64_t NewNativeTextureLayer(
    const BrushPaint::TextureLayer& texture_layer) {
  return reinterpret_cast<int64_t>(new BrushPaint::TextureLayer(texture_layer));
}

// Casts a Kotlin BrushBehavior.nativePointer to a C++
// BrushPaint::TextureLayer. The returned TextureLayer is a const ref as the
// Kotlin BrushPaint is immutable.
inline const BrushPaint::TextureLayer& CastToTextureLayer(
    int64_t texture_layer_native_pointer) {
  ABSL_CHECK_NE(texture_layer_native_pointer, 0);
  return *reinterpret_cast<BrushPaint::TextureLayer*>(
      texture_layer_native_pointer);
}

// Frees a Kotlin BrushPaint::TextureLayer.nativePointer.
inline void DeleteNativeTextureLayer(int64_t texture_layer_native_pointer) {
  if (texture_layer_native_pointer == 0) return;
  delete reinterpret_cast<BrushPaint::TextureLayer*>(
      texture_layer_native_pointer);
}

// Creates a new heap-allocated copy of the given BrushTip and returns a
// pointer to it as a int64_t, suitable for wrapping in a Kotlin BrushTip.
inline int64_t NewNativeBrushTip(const BrushTip& brush_tip) {
  return reinterpret_cast<int64_t>(new BrushTip(brush_tip));
}

// Casts a Kotlin BrushTip.nativePointer to a C++ BrushTip. The returned
// BrushTip is a const ref as the Kotlin BrushTip is immutable.
inline const BrushTip& CastToBrushTip(int64_t brush_tip_native_pointer) {
  ABSL_CHECK_NE(brush_tip_native_pointer, 0);
  return *reinterpret_cast<BrushTip*>(brush_tip_native_pointer);
}

// Frees a Kotlin BrushTip.nativePointer.
inline void DeleteNativeBrushTip(int64_t brush_tip_native_pointer) {
  if (brush_tip_native_pointer == 0) return;
  delete reinterpret_cast<BrushTip*>(brush_tip_native_pointer);
}

// Creates a new heap-allocated copy of the given BrushBehavior and returns a
// pointer to it as a int64_t, suitable for wrapping in a Kotlin BrushBehavior.
inline int64_t NewNativeBrushBehavior(const BrushBehavior& brush_behavior) {
  return reinterpret_cast<int64_t>(new BrushBehavior(brush_behavior));
}

// Casts a Kotlin BrushBehavior.nativePointer to a C++ BrushBehavior. The
// returned BrushBehavior is a const ref as the Kotlin BrushBehavior is
// immutable.
inline const BrushBehavior& CastToBrushBehavior(
    int64_t brush_behavior_native_pointer) {
  ABSL_CHECK_NE(brush_behavior_native_pointer, 0);
  return *reinterpret_cast<BrushBehavior*>(brush_behavior_native_pointer);
}

// Frees a Kotlin BrushBehavior.nativePointer.
inline void DeleteNativeBrushBehavior(int64_t brush_behavior_native_pointer) {
  if (brush_behavior_native_pointer == 0) return;
  delete reinterpret_cast<BrushBehavior*>(brush_behavior_native_pointer);
}

// Creates a new heap-allocated copy of the given BrushBehavior::Node and
// returns a pointer to it as a int64_t, suitable for wrapping in a Kotlin
// BrushBehavior.Node.
inline int64_t NewNativeBrushBehaviorNode(const BrushBehavior::Node& node) {
  return reinterpret_cast<int64_t>(new BrushBehavior::Node(node));
}

// Casts a Kotlin BrushBehavior.Node.nativePointer to a C++
// BrushBehavior::Node. The returned Node is a const ref as the Kotlin
// BrushBehavior is immutable.
inline const BrushBehavior::Node& CastToBrushBehaviorNode(
    int64_t node_native_pointer) {
  ABSL_CHECK_NE(node_native_pointer, 0);
  return *reinterpret_cast<BrushBehavior::Node*>(node_native_pointer);
}

// Frees a Kotlin BrushBehavior::Node.nativePointer.
inline void DeleteNativeBrushBehaviorNode(int64_t node_native_pointer) {
  if (node_native_pointer == 0) return;
  delete reinterpret_cast<BrushBehavior::Node*>(node_native_pointer);
}

// Creates a new heap-allocated copy of the given ColorFunction and returns a
// pointer to it as a int64_t, suitable for wrapping in a Kotlin ColorFunction.
inline int64_t NewNativeColorFunction(const ColorFunction& color_function) {
  return reinterpret_cast<int64_t>(new ColorFunction(color_function));
}

// Casts a Kotlin ColorFunction.nativePointer to a C++ ColorFunction. The
// returned ColorFunction is a const ref as the Kotlin ColorFunction is
// immutable.
inline const ColorFunction& CastToColorFunction(
    int64_t color_function_native_pointer) {
  ABSL_CHECK_NE(color_function_native_pointer, 0);
  return *reinterpret_cast<ColorFunction*>(color_function_native_pointer);
}

// Frees a Kotlin ColorFunction.nativePointer.
inline void DeleteNativeColorFunction(int64_t color_function_native_pointer) {
  if (color_function_native_pointer == 0) return;
  delete reinterpret_cast<ColorFunction*>(color_function_native_pointer);
}

// Creates a new heap-allocated copy of the given EasingFunction
// and returns a pointer to it as a int64_t, suitable for wrapping in a Kotlin
// EasingFunction.
inline int64_t NewNativeEasingFunction(const EasingFunction& easing_function) {
  return reinterpret_cast<int64_t>(new EasingFunction(easing_function));
}

// Casts a Kotlin EasingFunction.nativePointer to a C++ EasingFunction. The
// returned EasingFunction is a const ref as the Kotlin EasingFunction is
// immutable.
inline const EasingFunction& CastToEasingFunction(
    int64_t easing_function_native_pointer) {
  ABSL_CHECK_NE(easing_function_native_pointer, 0);
  return *reinterpret_cast<EasingFunction*>(easing_function_native_pointer);
}

// Frees a Kotlin EasingFunction.nativePointer.
inline void DeleteNativeEasingFunction(int64_t easing_function_native_pointer) {
  if (easing_function_native_pointer == 0) return;
  delete reinterpret_cast<EasingFunction*>(easing_function_native_pointer);
}

// Converts the integer value of a Kotlin `Version` into a C++ `Version`.
absl::StatusOr<Version> IntToVersion(int version);

}  // namespace ink::native

#endif  // INK_BRUSH_INTERNAL_JNI_BRUSH_NATIVE_HELPER_H_
