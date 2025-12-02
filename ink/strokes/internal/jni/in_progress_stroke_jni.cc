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

#include "absl/base/nullability.h"
#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/types/span.h"
#include "ink/brush/internal/jni/brush_jni_helper.h"
#include "ink/geometry/envelope.h"
#include "ink/geometry/internal/jni/box_accumulator_jni_helper.h"
#include "ink/geometry/internal/jni/mesh_format_jni_helper.h"
#include "ink/geometry/internal/jni/vec_jni_helper.h"
#include "ink/geometry/mutable_mesh.h"
#include "ink/geometry/point.h"
#include "ink/jni/internal/jni_defines.h"
#include "ink/jni/internal/jni_throw_util.h"
#include "ink/strokes/in_progress_stroke.h"
#include "ink/strokes/input/stroke_input.h"
#include "ink/strokes/input/stroke_input_batch.h"
#include "ink/strokes/internal/jni/in_progress_stroke_jni_helper.h"
#include "ink/strokes/internal/jni/stroke_input_jni_helper.h"
#include "ink/strokes/internal/jni/stroke_jni_helper.h"
#include "ink/types/duration.h"

namespace {

using ::ink::Duration32;
using ::ink::Envelope;
using ::ink::InProgressStroke;
using ::ink::Point;
using ::ink::StrokeInput;
using ::ink::StrokeInputBatch;
using ::ink::jni::CastToBrush;
using ::ink::jni::CastToInProgressStrokeWrapper;
using ::ink::jni::CastToMutableInProgressStrokeWrapper;
using ::ink::jni::CastToMutableStrokeInputBatch;
using ::ink::jni::CastToStrokeInputBatch;
using ::ink::jni::DeleteNativeInProgressStroke;
using ::ink::jni::FillJBoxAccumulatorOrThrow;
using ::ink::jni::FillJMutableVecFromPointOrThrow;
using ::ink::jni::InProgressStrokeWrapper;
using ::ink::jni::NewNativeInProgressStroke;
using ::ink::jni::NewNativeMeshFormat;
using ::ink::jni::NewNativeStroke;
using ::ink::jni::ThrowExceptionFromStatus;
using ::ink::jni::UpdateJObjectInputOrThrow;

}  // namespace

extern "C" {

// Construct a native InProgressStroke and return a pointer to it as a long.
JNI_METHOD(strokes, InProgressStrokeNative, jlong, create)
(JNIEnv* env, jobject thiz) { return NewNativeInProgressStroke(); }

JNI_METHOD(strokes, InProgressStrokeNative, void, free)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  DeleteNativeInProgressStroke(native_pointer);
}

JNI_METHOD(strokes, InProgressStrokeNative, void, clear)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  CastToMutableInProgressStrokeWrapper(native_pointer).Stroke().Clear();
}

// Starts the stroke with a brush.
JNI_METHOD(strokes, InProgressStrokeNative, void, start)
(JNIEnv* env, jobject thiz, jlong native_pointer, jlong brush_native_pointer,
 jint noise_seed) {
  CastToMutableInProgressStrokeWrapper(native_pointer)
      .Start(CastToBrush(brush_native_pointer), noise_seed);
}

JNI_METHOD(strokes, InProgressStrokeNative, jboolean, enqueueInputs)
(JNIEnv* env, jobject thiz, jlong native_pointer, jlong real_inputs_pointer,
 jlong predicted_inputs_pointer) {
  InProgressStroke& in_progress_stroke =
      CastToMutableInProgressStrokeWrapper(native_pointer).Stroke();
  if (absl::Status status = in_progress_stroke.EnqueueInputs(
          CastToStrokeInputBatch(real_inputs_pointer),
          CastToStrokeInputBatch(predicted_inputs_pointer));
      !status.ok()) {
    ThrowExceptionFromStatus(env, status);
    return false;
  }
  return true;
}

JNI_METHOD(strokes, InProgressStrokeNative, jboolean, updateShape)
(JNIEnv* env, jobject thiz, jlong native_pointer,
 jlong j_current_elapsed_time_millis) {
  if (absl::Status status =
          CastToMutableInProgressStrokeWrapper(native_pointer)
              .UpdateShape(Duration32::Millis(j_current_elapsed_time_millis));
      !status.ok()) {
    ThrowExceptionFromStatus(env, status);
    return false;
  }
  return true;
}

JNI_METHOD(strokes, InProgressStrokeNative, void, finishInput)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  CastToMutableInProgressStrokeWrapper(native_pointer).Stroke().FinishInputs();
}

JNI_METHOD(strokes, InProgressStrokeNative, jboolean, isInputFinished)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return CastToInProgressStrokeWrapper(native_pointer)
      .Stroke()
      .InputsAreFinished();
}

JNI_METHOD(strokes, InProgressStrokeNative, jboolean, isUpdateNeeded)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return CastToInProgressStrokeWrapper(native_pointer).Stroke().NeedsUpdate();
}

JNI_METHOD(strokes, InProgressStrokeNative, jboolean, changesWithTime)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return CastToInProgressStrokeWrapper(native_pointer)
      .Stroke()
      .ChangesWithTime();
}

JNI_METHOD(strokes, InProgressStrokeNative, jlong, newStrokeFromCopy)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return NewNativeStroke(
      CastToInProgressStrokeWrapper(native_pointer).Stroke().CopyToStroke());
}

JNI_METHOD(strokes, InProgressStrokeNative, jlong, newStrokeFromPrunedCopy)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return NewNativeStroke(
      CastToInProgressStrokeWrapper(native_pointer)
          .Stroke()
          .CopyToStroke(InProgressStroke::RetainAttributes::kUsedByThisBrush));
}

JNI_METHOD(strokes, InProgressStrokeNative, jint, getInputCount)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return CastToInProgressStrokeWrapper(native_pointer).Stroke().InputCount();
}

JNI_METHOD(strokes, InProgressStrokeNative, jint, getRealInputCount)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  const InProgressStroke& in_progress_stroke =
      CastToInProgressStrokeWrapper(native_pointer).Stroke();
  return in_progress_stroke.RealInputCount();
}

JNI_METHOD(strokes, InProgressStrokeNative, jint, getPredictedInputCount)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  const InProgressStroke& in_progress_stroke =
      CastToInProgressStrokeWrapper(native_pointer).Stroke();
  return in_progress_stroke.PredictedInputCount();
}

JNI_METHOD(strokes, InProgressStrokeNative, jint, populateInputs)
(JNIEnv* env, jobject thiz, jlong native_pointer,
 jlong mutable_stroke_input_batch_pointer, jint from, jint to) {
  const InProgressStroke& in_progress_stroke =
      CastToInProgressStrokeWrapper(native_pointer).Stroke();
  StrokeInputBatch& batch =
      CastToMutableStrokeInputBatch(mutable_stroke_input_batch_pointer);
  batch.Clear();
  const StrokeInputBatch& inputs = in_progress_stroke.GetInputs();
  for (int i = from; i < to; ++i) {
    // The input here should have already been validated.
    ABSL_CHECK_OK(batch.Append(inputs.Get(i)));
  }
  return in_progress_stroke.GetInputs().Size();
}

JNI_METHOD(strokes, InProgressStrokeNative, void, getAndOverwriteInput)
(JNIEnv* env, jobject thiz, jlong native_pointer, jobject j_input, jint index,
 jclass input_tool_type_class) {
  const InProgressStroke& in_progress_stroke =
      CastToInProgressStrokeWrapper(native_pointer).Stroke();
  StrokeInput input = in_progress_stroke.GetInputs().Get(index);
  UpdateJObjectInputOrThrow(env, input, j_input);
}

JNI_METHOD(strokes, InProgressStrokeNative, jint, getBrushCoatCount)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return CastToInProgressStrokeWrapper(native_pointer)
      .Stroke()
      .BrushCoatCount();
}

JNI_METHOD(strokes, InProgressStrokeNative, void, getMeshBounds)
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
  const InProgressStroke& in_progress_stroke =
      CastToInProgressStrokeWrapper(native_pointer).Stroke();
  const Envelope& updated_region = in_progress_stroke.GetUpdatedRegion();
  FillJBoxAccumulatorOrThrow(env, updated_region, j_out_envelope);
}

JNI_METHOD(strokes, InProgressStrokeNative, void, resetUpdatedRegion)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  CastToMutableInProgressStrokeWrapper(native_pointer)
      .Stroke()
      .ResetUpdatedRegion();
}

JNI_METHOD(strokes, InProgressStrokeNative, jint, getOutlineCount)
(JNIEnv* env, jobject thiz, jlong native_pointer, jint coat_index) {
  return CastToInProgressStrokeWrapper(native_pointer)
      .Stroke()
      .GetCoatOutlines(coat_index)
      .size();
}

JNI_METHOD(strokes, InProgressStrokeNative, jint, getOutlineVertexCount)
(JNIEnv* env, jobject thiz, jlong native_pointer, jint coat_index,
 jint outline_index) {
  return CastToInProgressStrokeWrapper(native_pointer)
      .Stroke()
      .GetCoatOutlines(coat_index)[outline_index]
      .size();
}

JNI_METHOD(strokes, InProgressStrokeNative, void, fillOutlinePosition)
(JNIEnv* env, jobject thiz, jlong native_pointer, jint coat_index,
 jint outline_index, jint outline_vertex_index, jobject out_position) {
  const InProgressStroke& in_progress_stroke =
      CastToInProgressStrokeWrapper(native_pointer).Stroke();
  absl::Span<const uint32_t> outline =
      in_progress_stroke.GetCoatOutlines(coat_index)[outline_index];
  Point position = in_progress_stroke.GetMesh(coat_index)
                       .VertexPosition(outline[outline_vertex_index]);
  FillJMutableVecFromPointOrThrow(env, out_position, position);
}

JNI_METHOD(strokes, InProgressStrokeNative, void, fillPosition)
(JNIEnv* env, jobject thiz, jlong native_pointer, jint coat_index,
 jint partition_index, jint vertex_index, jobject out_position) {
  const InProgressStroke& in_progress_stroke =
      CastToInProgressStrokeWrapper(native_pointer).Stroke();
  Point position =
      in_progress_stroke.GetMesh(coat_index).VertexPosition(vertex_index);
  FillJMutableVecFromPointOrThrow(env, out_position, position);
}

JNI_METHOD(strokes, InProgressStrokeNative, jint, getMeshPartitionCount)
(JNIEnv* env, jobject thiz, jlong native_pointer, jint coat_index) {
  return CastToInProgressStrokeWrapper(native_pointer)
      .MeshPartitionCount(coat_index);
}

JNI_METHOD(strokes, InProgressStrokeNative, jint, getVertexCount)
(JNIEnv* env, jobject thiz, jlong native_pointer, jint coat_index,
 jint mesh_index) {
  return CastToInProgressStrokeWrapper(native_pointer)
      .VertexCount(coat_index, mesh_index);
}

JNI_METHOD(strokes, InProgressStrokeNative, absl_nullable jobject,
           getUnsafelyMutableRawVertexData)
(JNIEnv* env, jobject thiz, jlong native_pointer, jint coat_index,
 jint mesh_index) {
  return CastToInProgressStrokeWrapper(native_pointer)
      .GetUnsafelyMutableRawVertexData(env, coat_index, mesh_index);
}

// Returns a direct byte buffer of the triangle index data in 16-bit format.
// Automatically converts 32-bit to 16-bit data internally if needed, which
// omits any triangles beyond the point where index values first exceed the
// 16-bit maximum value. Those triangles are still present in the underlying
// data and will be included in `CopyToStroke`, but will not be returned by this
// method (which is typically used for rendering).
// TODO: b/294561921 - Simplify this when the underlying index data is in 16 bit
//   values.
JNI_METHOD(strokes, InProgressStrokeNative, absl_nullable jobject,
           getUnsafelyMutableRawTriangleIndexData)
(JNIEnv* env, jobject thiz, jlong native_pointer, jint coat_index,
 jint mesh_index) {
  return CastToInProgressStrokeWrapper(native_pointer)
      .GetUnsafelyMutableRawTriangleIndexData(env, coat_index, mesh_index);
}

// Return a newly allocated copy of the given `Mesh`'s `MeshFormat`.
JNI_METHOD(strokes, InProgressStrokeNative, jlong, newCopyOfMeshFormat)
(JNIEnv* env, jobject thiz, jlong native_pointer, jint coat_index) {
  const InProgressStrokeWrapper& in_progress_stroke_wrapper =
      CastToInProgressStrokeWrapper(native_pointer);
  return NewNativeMeshFormat(
      in_progress_stroke_wrapper.Stroke().GetMeshFormat(coat_index));
}

}  // extern "C"
