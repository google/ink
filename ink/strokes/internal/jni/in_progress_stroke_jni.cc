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

#include "absl/base/nullability.h"
#include "ink/geometry/internal/jni/box_accumulator_jni_helper.h"
#include "ink/geometry/internal/jni/vec_jni_helper.h"
#include "ink/jni/internal/jni_buffer_util.h"
#include "ink/jni/internal/jni_defines.h"
#include "ink/jni/internal/status_jni_helper.h"
#include "ink/strokes/internal/jni/in_progress_stroke_native.h"
#include "ink/strokes/internal/jni/in_progress_stroke_native_helper.h"
#include "ink/strokes/internal/jni/stroke_input_jni_helper.h"

using ::ink::jni::ByteSpanToUnsafelyMutableDirectByteBuffer;
using ::ink::jni::FillJBoxAccumulatorOrThrow;
using ::ink::jni::FillJMutableVecOrThrow;
using ::ink::jni::ThrowExceptionFromStatusCallback;
using ::ink::jni::UpdateJStrokeInputOrThrow;
using ::ink::native::CastToInProgressStrokeWrapper;

extern "C" {

// Construct a native InProgressStroke and return a pointer to it as a long.
JNI_METHOD(strokes, InProgressStrokeNative, jlong, create)
(JNIEnv* env, jobject thiz) { return InProgressStrokeNative_create(); }

JNI_METHOD(strokes, InProgressStrokeNative, void, free)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  InProgressStrokeNative_free(native_pointer);
}

JNI_METHOD(strokes, InProgressStrokeNative, void, clear)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  InProgressStrokeNative_clear(native_pointer);
}

// Starts the stroke with a brush.
JNI_METHOD(strokes, InProgressStrokeNative, void, start)
(JNIEnv* env, jobject thiz, jlong native_pointer, jlong brush_native_pointer,
 jint noise_seed, jfloat base_animation_phase) {
  InProgressStrokeNative_start(native_pointer, brush_native_pointer, noise_seed,
                               base_animation_phase);
}

JNI_METHOD(strokes, InProgressStrokeNative, jboolean, enqueueInputs)
(JNIEnv* env, jobject thiz, jlong native_pointer, jlong real_inputs_pointer,
 jlong predicted_inputs_pointer) {
  return InProgressStrokeNative_enqueueInputs(
      env, native_pointer, real_inputs_pointer, predicted_inputs_pointer,
      &ThrowExceptionFromStatusCallback);
}

JNI_METHOD(strokes, InProgressStrokeNative, jboolean, updateShape)
(JNIEnv* env, jobject thiz, jlong native_pointer,
 jlong current_elapsed_time_millis) {
  return InProgressStrokeNative_updateShape(env, native_pointer,
                                            current_elapsed_time_millis,
                                            &ThrowExceptionFromStatusCallback);
}

JNI_METHOD(strokes, InProgressStrokeNative, void, finishInput)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  InProgressStrokeNative_finishInput(native_pointer);
}

JNI_METHOD(strokes, InProgressStrokeNative, jboolean, isInputFinished)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return InProgressStrokeNative_isInputFinished(native_pointer);
}

JNI_METHOD(strokes, InProgressStrokeNative, jboolean, isUpdateNeeded)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return InProgressStrokeNative_isUpdateNeeded(native_pointer);
}

JNI_METHOD(strokes, InProgressStrokeNative, jboolean, changesWithTime)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return InProgressStrokeNative_changesWithTime(native_pointer);
}

JNI_METHOD(strokes, InProgressStrokeNative, jlong, newStrokeFromCopy)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return InProgressStrokeNative_newStrokeFromCopy(native_pointer);
}

JNI_METHOD(strokes, InProgressStrokeNative, jlong, newStrokeFromPrunedCopy)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return InProgressStrokeNative_newStrokeFromPrunedCopy(native_pointer);
}

JNI_METHOD(strokes, InProgressStrokeNative, jint, getInputCount)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return InProgressStrokeNative_getInputCount(native_pointer);
}

JNI_METHOD(strokes, InProgressStrokeNative, jint, getRealInputCount)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return InProgressStrokeNative_getRealInputCount(native_pointer);
}

JNI_METHOD(strokes, InProgressStrokeNative, jint, getPredictedInputCount)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return InProgressStrokeNative_getPredictedInputCount(native_pointer);
}

JNI_METHOD(strokes, InProgressStrokeNative, void, populateInputs)
(JNIEnv* env, jobject thiz, jlong native_pointer,
 jlong mutable_stroke_input_batch_pointer, jint from, jint to) {
  InProgressStrokeNative_populateInputs(
      native_pointer, mutable_stroke_input_batch_pointer, from, to);
}

JNI_METHOD(strokes, InProgressStrokeNative, void, getAndOverwriteInput)
(JNIEnv* env, jobject thiz, jlong native_pointer, jobject j_input, jint index,
 jclass input_tool_type_class) {
  UpdateJStrokeInputOrThrow(
      env, InProgressStrokeNative_getInput(native_pointer, index), j_input);
}

JNI_METHOD(strokes, InProgressStrokeNative, jfloat, getBaseAnimationPhase)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return InProgressStrokeNative_getBaseAnimationPhase(native_pointer);
}

JNI_METHOD(strokes, InProgressStrokeNative, jint, getBrushCoatCount)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return InProgressStrokeNative_getBrushCoatCount(native_pointer);
}

JNI_METHOD(strokes, InProgressStrokeNative, void, fillMeshBounds)
(JNIEnv* env, jobject thiz, jlong native_pointer, jint coat_index,
 jobject j_out_envelope) {
  FillJBoxAccumulatorOrThrow(env,
                             CastToInProgressStrokeWrapper(native_pointer)
                                 .Stroke()
                                 .GetMeshBounds(coat_index),
                             j_out_envelope);
}

JNI_METHOD(strokes, InProgressStrokeNative, void, fillUpdatedRegion)
(JNIEnv* env, jobject thiz, jlong native_pointer, jobject j_out_envelope) {
  FillJBoxAccumulatorOrThrow(
      env,
      CastToInProgressStrokeWrapper(native_pointer).Stroke().GetUpdatedRegion(),
      j_out_envelope);
}

JNI_METHOD(strokes, InProgressStrokeNative, void, resetUpdatedRegion)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  InProgressStrokeNative_resetUpdatedRegion(native_pointer);
}

JNI_METHOD(strokes, InProgressStrokeNative, jint, getOutlineCount)
(JNIEnv* env, jobject thiz, jlong native_pointer, jint coat_index) {
  return InProgressStrokeNative_getOutlineCount(native_pointer, coat_index);
}

JNI_METHOD(strokes, InProgressStrokeNative, jint, getOutlineVertexCount)
(JNIEnv* env, jobject thiz, jlong native_pointer, jint coat_index,
 jint outline_index) {
  return InProgressStrokeNative_getOutlineVertexCount(
      native_pointer, coat_index, outline_index);
}

JNI_METHOD(strokes, InProgressStrokeNative, void, fillOutlinePosition)
(JNIEnv* env, jobject thiz, jlong native_pointer, jint coat_index,
 jint outline_index, jint outline_vertex_index, jobject out_position) {
  FillJMutableVecOrThrow(
      env,
      InProgressStrokeNative_getOutlinePosition(
          native_pointer, coat_index, outline_index, outline_vertex_index),
      out_position);
}

JNI_METHOD(strokes, InProgressStrokeNative, void, fillPosition)
(JNIEnv* env, jobject thiz, jlong native_pointer, jint coat_index,
 jint partition_index, jint vertex_index, jobject out_position) {
  FillJMutableVecOrThrow(
      env,
      InProgressStrokeNative_getPosition(native_pointer, coat_index,
                                         partition_index, vertex_index),
      out_position);
}

JNI_METHOD(strokes, InProgressStrokeNative, jint, getMeshPartitionCount)
(JNIEnv* env, jobject thiz, jlong native_pointer, jint coat_index) {
  return InProgressStrokeNative_getMeshPartitionCount(native_pointer,
                                                      coat_index);
}

JNI_METHOD(strokes, InProgressStrokeNative, jint, getVertexCount)
(JNIEnv* env, jobject thiz, jlong native_pointer, jint coat_index,
 jint mesh_index) {
  return InProgressStrokeNative_getVertexCount(native_pointer, coat_index,
                                               mesh_index);
}

JNI_METHOD(strokes, JvmInProgressStrokeNative, absl_nullable jobject,
           getUnsafelyMutableInProgressStrokeOwnedRawVertexData)
(JNIEnv* env, jobject thiz, jlong native_pointer, jint coat_index,
 jint mesh_index) {
  // The resulting buffer will be writeable, but it will be wrapped at the
  // Kotlin layer in a read-only buffer that delegates to this one.
  return ByteSpanToUnsafelyMutableDirectByteBuffer(
      env, CastToInProgressStrokeWrapper(native_pointer)
               .GetRawVertexData(coat_index, mesh_index));
}

// Returns a direct byte buffer of the triangle index data in 16-bit format.
// Automatically converts 32-bit to 16-bit data internally if needed, which
// omits any triangles beyond the point where index values first exceed the
// 16-bit maximum value. Those triangles are still present in the underlying
// data and will be included in `CopyToStroke`, but will not be returned by this
// method (which is typically used for rendering).
// TODO: b/294561921 - Simplify this when the underlying index data is in 16 bit
//   values.
JNI_METHOD(strokes, JvmInProgressStrokeNative, absl_nullable jobject,
           getUnsafelyMutableInProgressStrokeOwnedRawTriangleIndexData)
(JNIEnv* env, jobject thiz, jlong native_pointer, jint coat_index,
 jint mesh_index) {
  // The resulting buffer will be writeable, but it will be wrapped at the
  // Kotlin layer in a read-only buffer that delegates to this one.
  return ByteSpanToUnsafelyMutableDirectByteBuffer(
      env, CastToInProgressStrokeWrapper(native_pointer)
               .GetRawTriangleIndexData(coat_index, mesh_index));
}

// Return a newly allocated copy of the given `Mesh`'s `MeshFormat`.
JNI_METHOD(strokes, InProgressStrokeNative, jlong, newCopyOfMeshFormat)
(JNIEnv* env, jobject thiz, jlong native_pointer, jint coat_index) {
  return InProgressStrokeNative_newCopyOfMeshFormat(native_pointer, coat_index);
}

}  // extern "C"
