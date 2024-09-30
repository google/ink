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

#ifndef INK_STROKES_BRUSH_INTERNAL_JNI_BRUSH_JNI_HELPER_H_
#define INK_STROKES_BRUSH_INTERNAL_JNI_BRUSH_JNI_HELPER_H_

#include <jni.h>

#include <string>

#include "ink/brush/brush.h"
#include "ink/brush/brush_behavior.h"
#include "ink/brush/brush_coat.h"
#include "ink/brush/brush_family.h"
#include "ink/brush/brush_paint.h"
#include "ink/brush/brush_tip.h"

namespace ink {

// Casts a Kotlin Brush.nativePointer to a C++ Brush. The returned
// Brush is a const ref as the kotlin Brush is immutable.
inline const ink::Brush& CastToBrush(jlong brush_native_pointer) {
  return *reinterpret_cast<ink::Brush*>(brush_native_pointer);
}

// Casts a Kotlin BrushFamily.nativePointer to a C++ BrushFamily. The returned
// BrushFamily is a const ref as the kotlin BrushFamily is immutable.
inline const ink::BrushFamily& CastToBrushFamily(
    jlong brush_family_native_pointer) {
  return *reinterpret_cast<ink::BrushFamily*>(brush_family_native_pointer);
}

// Casts a Kotlin BrushCoat.nativePointer to a C++ BrushCoat. The returned
// BrushCoat is a const ref as the kotlin BrushCoat is immutable.
inline const ink::BrushCoat& CastToBrushCoat(jlong brush_coat_native_pointer) {
  return *reinterpret_cast<ink::BrushCoat*>(brush_coat_native_pointer);
}

// Casts a Kotlin BrushPaint.nativePointer to a C++ BrushPaint. The returned
// BrushPaint is a const ref as the kotlin BrushPaint is immutable.
inline const ink::BrushPaint& CastToBrushPaint(
    jlong brush_paint_native_pointer) {
  return *reinterpret_cast<ink::BrushPaint*>(brush_paint_native_pointer);
}

// Casts a Kotlin BrushTip.nativePointer to a C++ BrushTip. The returned
// BrushTip is a const ref as the kotlin BrushTip is immutable.
inline const ink::BrushTip& CastToBrushTip(jlong brush_tip_native_pointer) {
  return *reinterpret_cast<ink::BrushTip*>(brush_tip_native_pointer);
}

// Casts a Kotlin BrushBehavior.nativePointer to a C++ BrushBehavior. The
// returned BrushBehavior is a const ref as the kotlin BrushBehavior is
// immutable.
inline const ink::BrushBehavior& CastToBrushBehavior(
    jlong brush_behavior_native_pointer) {
  return *reinterpret_cast<ink::BrushBehavior*>(brush_behavior_native_pointer);
}

}  // namespace ink
#endif  // INK_STROKES_BRUSH_INTERNAL_JNI_BRUSH_JNI_HELPER_H_
