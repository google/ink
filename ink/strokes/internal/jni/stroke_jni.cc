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

#include "ink/jni/internal/jni_defines.h"
#include "ink/jni/internal/status_jni_helper.h"
#include "ink/strokes/internal/jni/stroke_native.h"

extern "C" {

using ::ink::jni::ThrowExceptionFromStatusCallback;

JNI_METHOD(strokes, StrokeNative, jlong, createWithBrushAndInputs)
(JNIEnv* env, jobject object, jlong brush_native_pointer,
 jlong inputs_native_pointer) {
  return StrokeNative_createWithBrushAndInputs(brush_native_pointer,
                                               inputs_native_pointer);
}

JNI_METHOD(strokes, StrokeNative, jlong, createWithBrushInputsAndShape)
(JNIEnv* env, jobject object, jlong brush_native_pointer,
 jlong inputs_native_pointer, jlong partitioned_mesh_native_pointer) {
  return StrokeNative_createWithBrushInputsAndShape(
      brush_native_pointer, inputs_native_pointer,
      partitioned_mesh_native_pointer);
}

// Make a heap-allocated shallow (doesn't replicate all the individual input
// points) copy of the `StrokeInputBatch` belonging to this `Stroke`. Return
// the raw pointer to this copy, so that it can be wrapped by a JVM
// `StrokeInputBatch`, which is responsible for freeing the copy when it is
// garbage collected and finalized.
JNI_METHOD(strokes, StrokeNative, jlong, newShallowCopyOfInputs)
(JNIEnv* env, jobject object, jlong native_pointer_to_stroke) {
  return StrokeNative_newShallowCopyOfInputs(native_pointer_to_stroke);
}

// Make a heap-allocated shallow (doesn't replicate all the individual meshes)
// copy of the `PartitionedMesh` belonging to this `Stroke`. Return the raw
// pointer to this copy, so that it can be wrapped by a JVM `PartitionedMesh`,
// which is responsible for freeing the copy when it is garbage collected and
// finalized.
JNI_METHOD(strokes, StrokeNative, jlong, newShallowCopyOfShape)
(JNIEnv* env, jobject object, jlong native_pointer_to_stroke) {
  return StrokeNative_newShallowCopyOfShape(native_pointer_to_stroke);
}

// Free the given `Stroke`.
JNI_METHOD(strokes, StrokeNative, void, free)
(JNIEnv* env, jobject object, jlong native_pointer_to_stroke) {
  StrokeNative_free(native_pointer_to_stroke);
}

JNI_METHOD(strokes, StrokeNative, jlong, createWithSubtract)
(JNIEnv* env, jobject object, jlong target_stroke_ptr, jlong mask_shape_ptr,
 jfloat mask_a, jfloat mask_b, jfloat mask_c, jfloat mask_d, jfloat mask_e,
 jfloat mask_f, jfloat stroke_a, jfloat stroke_b, jfloat stroke_c,
 jfloat stroke_d, jfloat stroke_e, jfloat stroke_f) {
  return StrokeNative_createWithSubtract(
      target_stroke_ptr, mask_shape_ptr, mask_a, mask_b, mask_c, mask_d, mask_e,
      mask_f, stroke_a, stroke_b, stroke_c, stroke_d, stroke_e, stroke_f);
}

JNI_METHOD(strokes, MultipleStrokesNative, jlong, createWithSplit)
(JNIEnv* env, jobject object, jlong target_stroke_ptr, jfloat transform_a,
 jfloat transform_b, jfloat transform_c, jfloat transform_d, jfloat transform_e,
 jfloat transform_f, jfloat tolerance) {
  return MultipleStrokesNative_createWithSplit(
      env, target_stroke_ptr, transform_a, transform_b, transform_c,
      transform_d, transform_e, transform_f, tolerance,
      &ThrowExceptionFromStatusCallback);
}

JNI_METHOD(strokes, MultipleStrokesNative, jint, getStrokeCount)
(JNIEnv* env, jobject object, jlong native_pointer) {
  return MultipleStrokesNative_getStrokeCount(native_pointer);
}

JNI_METHOD(strokes, MultipleStrokesNative, jlong, releaseStroke)
(JNIEnv* env, jobject object, jlong native_pointer, jint index) {
  return MultipleStrokesNative_releaseStroke(native_pointer, index);
}

JNI_METHOD(strokes, MultipleStrokesNative, void, free)
(JNIEnv* env, jobject object, jlong native_pointer) {
  MultipleStrokesNative_free(native_pointer);
}

}  // extern "C"
