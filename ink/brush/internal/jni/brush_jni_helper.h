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

#ifndef INK_STROKES_BRUSH_INTERNAL_JNI_BRUSH_JNI_HELPER_H_
#define INK_STROKES_BRUSH_INTERNAL_JNI_BRUSH_JNI_HELPER_H_

#include <jni.h>

#include "ink/brush/brush.h"
#include "ink/brush/brush_behavior.h"
#include "ink/brush/brush_coat.h"
#include "ink/brush/brush_family.h"
#include "ink/brush/brush_paint.h"
#include "ink/brush/brush_tip.h"

namespace ink::jni {

// Casts a Kotlin Brush.nativePointer to a C++ Brush. The returned
// Brush is a const ref as the Kotlin Brush is immutable.
inline const Brush& CastToBrush(jlong brush_native_pointer) {
  return *reinterpret_cast<Brush*>(brush_native_pointer);
}

// Casts a Kotlin BrushFamily.nativePointer to a C++ BrushFamily. The returned
// BrushFamily is a const ref as the Kotlin BrushFamily is immutable.
inline const BrushFamily& CastToBrushFamily(jlong brush_family_native_pointer) {
  return *reinterpret_cast<BrushFamily*>(brush_family_native_pointer);
}

// Casts a Kotlin BrushCoat.nativePointer to a C++ BrushCoat. The returned
// BrushCoat is a const ref as the Kotlin BrushCoat is immutable.
inline const BrushCoat& CastToBrushCoat(jlong brush_coat_native_pointer) {
  return *reinterpret_cast<BrushCoat*>(brush_coat_native_pointer);
}

// Casts a Kotlin BrushPaint.nativePointer to a C++ BrushPaint. The returned
// BrushPaint is a const ref as the Kotlin BrushPaint is immutable.
inline const BrushPaint& CastToBrushPaint(jlong brush_paint_native_pointer) {
  return *reinterpret_cast<BrushPaint*>(brush_paint_native_pointer);
}

// Casts a Kotlin BrushBehavior.nativePointer to a C++ BrushPaint::TextureLayer.
// The returned TextureLayer is a const ref as the Kotlin BrushPaint is
// immutable.
inline const BrushPaint::TextureLayer& CastToTextureLayer(
    jlong texture_layer_native_pointer) {
  return *reinterpret_cast<BrushPaint::TextureLayer*>(
      texture_layer_native_pointer);
}

// Casts a Kotlin BrushTip.nativePointer to a C++ BrushTip. The returned
// BrushTip is a const ref as the Kotlin BrushTip is immutable.
inline const BrushTip& CastToBrushTip(jlong brush_tip_native_pointer) {
  return *reinterpret_cast<BrushTip*>(brush_tip_native_pointer);
}

// Casts a Kotlin BrushBehavior.nativePointer to a C++ BrushBehavior. The
// returned BrushBehavior is a const ref as the Kotlin BrushBehavior is
// immutable.
inline const BrushBehavior& CastToBrushBehavior(
    jlong brush_behavior_native_pointer) {
  return *reinterpret_cast<BrushBehavior*>(brush_behavior_native_pointer);
}

// Casts a Kotlin BrushBehavior.Node.nativePointer to a C++ BrushBehavior::Node.
// The returned Node is a const ref as the Kotlin BrushBehavior is
// immutable.
inline const BrushBehavior::Node& CastToBrushBehaviorNode(
    jlong node_native_pointer) {
  return *reinterpret_cast<BrushBehavior::Node*>(node_native_pointer);
}

}  // namespace ink::jni

#endif  // INK_STROKES_BRUSH_INTERNAL_JNI_BRUSH_JNI_HELPER_H_
