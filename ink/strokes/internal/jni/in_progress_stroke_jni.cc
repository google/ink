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

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <vector>

#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/types/span.h"
#include "ink/brush/brush.h"
#include "ink/brush/internal/jni/brush_jni_helper.h"
#include "ink/geometry/envelope.h"
#include "ink/geometry/internal/jni/envelope_jni_helper.h"
#include "ink/geometry/internal/jni/vec_jni_helper.h"
#include "ink/geometry/mesh_format.h"
#include "ink/geometry/mutable_mesh.h"
#include "ink/geometry/point.h"
#include "ink/jni/internal/jni_defines.h"
#include "ink/strokes/in_progress_stroke.h"
#include "ink/strokes/input/stroke_input.h"
#include "ink/strokes/input/stroke_input_batch.h"
#include "ink/strokes/internal/jni/stroke_input_jni_helper.h"
#include "ink/strokes/stroke.h"
#include "ink/types/duration.h"

namespace {

using ::ink::Brush;
using ::ink::CastToBrush;
using ::ink::CastToMutableStrokeInputBatch;
using ::ink::CastToStrokeInputBatch;
using ::ink::Duration32;
using ::ink::Envelope;
using ::ink::FillJMutableEnvelope;
using ::ink::InProgressStroke;
using ::ink::MeshFormat;
using ::ink::MutableMesh;
using ::ink::Point;
using ::ink::Stroke;
using ::ink::StrokeInput;
using ::ink::StrokeInputBatch;

// Associates an `InProgressStroke` with a cached triangle index buffer instance
// that is used for converting 32-bit indices to 16-bit indices, without needing
// to allocate a new buffer for this conversion every time.
// TODO: b/294561921 - Delete this when the underlying index data is in 16-bit
//   values.
struct InProgressStrokeWrapper {
  InProgressStroke in_progress_stroke;

  // For each brush coat, holds data previously returned by
  // nativeGetRawTriangleIndexData as a direct byte buffer, so that conversion
  // from 32-bit to 16-bit indices does not need to happen at the JVM layer with
  // JVM allocations.
  std::vector<std::vector<uint16_t>> triangle_index_data_caches;
};

InProgressStrokeWrapper* GetInProgressStrokeWrapper(
    jlong raw_ptr_to_in_progress_stroke_wrapper) {
  ABSL_CHECK_NE(raw_ptr_to_in_progress_stroke_wrapper, 0);
  return reinterpret_cast<InProgressStrokeWrapper*>(
      raw_ptr_to_in_progress_stroke_wrapper);
}

}  // namespace

extern "C" {

// Construct a native InProgressStroke and return a pointer to it as a long.
JNI_METHOD(strokes, InProgressStroke, jlong, nativeCreateInProgressStroke)
(JNIEnv* env, jobject thiz) {
  return reinterpret_cast<jlong>(new InProgressStrokeWrapper());
}

JNI_METHOD(strokes, InProgressStroke, void, nativeFreeInProgressStroke)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  delete GetInProgressStrokeWrapper(native_pointer);
}

JNI_METHOD(strokes, InProgressStroke, void, nativeClear)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  InProgressStroke& in_progress_stroke =
      GetInProgressStrokeWrapper(native_pointer)->in_progress_stroke;
  in_progress_stroke.Clear();
}

// Starts the stroke with a brush.
JNI_METHOD(strokes, InProgressStroke, void, nativeStart)
(JNIEnv* env, jobject thiz, jlong native_pointer, jlong brush_native_pointer) {
  InProgressStroke& in_progress_stroke =
      GetInProgressStrokeWrapper(native_pointer)->in_progress_stroke;
  const Brush& brush = CastToBrush(brush_native_pointer);
  in_progress_stroke.Start(brush);
}

JNI_METHOD(strokes, InProgressStroke, jstring, nativeEnqueueInputs)
(JNIEnv* env, jobject thiz, jlong native_pointer, jlong real_inputs_pointer,
 jlong predicted_inputs_pointer) {
  InProgressStroke& in_progress_stroke =
      GetInProgressStrokeWrapper(native_pointer)->in_progress_stroke;
  ABSL_CHECK_NE(real_inputs_pointer, 0);
  const StrokeInputBatch& real_inputs =
      CastToStrokeInputBatch(real_inputs_pointer);
  ABSL_CHECK_NE(predicted_inputs_pointer, 0);
  const StrokeInputBatch& predicted_inputs =
      CastToStrokeInputBatch(predicted_inputs_pointer);
  absl::Status status =
      in_progress_stroke.EnqueueInputs(real_inputs, predicted_inputs);
  if (!status.ok()) {
    return env->NewStringUTF(status.ToString().c_str());
  }
  return nullptr;
}

JNI_METHOD(strokes, InProgressStroke, jstring, nativeUpdateShape)
(JNIEnv* env, jobject thiz, jlong native_pointer,
 jlong j_current_elapsed_time_millis) {
  InProgressStroke& in_progress_stroke =
      GetInProgressStrokeWrapper(native_pointer)->in_progress_stroke;
  Duration32 current_elapsed_time =
      Duration32::Millis(j_current_elapsed_time_millis);
  absl::Status status = in_progress_stroke.UpdateShape(current_elapsed_time);
  if (!status.ok()) {
    return env->NewStringUTF(status.ToString().c_str());
  }
  return nullptr;
}

JNI_METHOD(strokes, InProgressStroke, void, nativeFinishInput)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  InProgressStroke& in_progress_stroke =
      GetInProgressStrokeWrapper(native_pointer)->in_progress_stroke;
  in_progress_stroke.FinishInputs();
}

JNI_METHOD(strokes, InProgressStroke, jboolean, nativeIsInputFinished)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  const InProgressStroke& in_progress_stroke =
      GetInProgressStrokeWrapper(native_pointer)->in_progress_stroke;
  return in_progress_stroke.InputsAreFinished();
}

JNI_METHOD(strokes, InProgressStroke, jboolean, nativeNeedsUpdate)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  const InProgressStroke& in_progress_stroke =
      GetInProgressStrokeWrapper(native_pointer)->in_progress_stroke;
  return in_progress_stroke.NeedsUpdate();
}

JNI_METHOD(strokes, InProgressStroke, jlong, nativeCopyToStroke)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  const InProgressStroke& in_progress_stroke =
      GetInProgressStrokeWrapper(native_pointer)->in_progress_stroke;
  const Stroke stroke = in_progress_stroke.CopyToStroke();
  return reinterpret_cast<jlong>(new Stroke(stroke));
}

JNI_METHOD(strokes, InProgressStroke, jint, nativeInputCount)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  const InProgressStroke& in_progress_stroke =
      GetInProgressStrokeWrapper(native_pointer)->in_progress_stroke;
  return in_progress_stroke.InputCount();
}

JNI_METHOD(strokes, InProgressStroke, jint, nativeRealInputCount)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  const InProgressStroke& in_progress_stroke =
      GetInProgressStrokeWrapper(native_pointer)->in_progress_stroke;
  return in_progress_stroke.RealInputCount();
}

JNI_METHOD(strokes, InProgressStroke, jint, nativePredictedInputCount)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  const InProgressStroke& in_progress_stroke =
      GetInProgressStrokeWrapper(native_pointer)->in_progress_stroke;
  return in_progress_stroke.PredictedInputCount();
}

JNI_METHOD(strokes, InProgressStroke, jint, nativeFillInputs)
(JNIEnv* env, jobject thiz, jlong native_pointer,
 jlong mutable_stroke_input_batch_pointer, jint from, jint to) {
  const InProgressStroke& in_progress_stroke =
      GetInProgressStrokeWrapper(native_pointer)->in_progress_stroke;
  StrokeInputBatch* batch =
      CastToMutableStrokeInputBatch(mutable_stroke_input_batch_pointer);
  const StrokeInputBatch& inputs = in_progress_stroke.GetInputs();
  for (int i = from; i < to; ++i) {
    // The input here should have already been validated.
    ABSL_CHECK_OK(batch->Append(inputs.Get(i)));
  }
  return in_progress_stroke.GetInputs().Size();
}

JNI_METHOD(strokes, InProgressStroke, void, nativeGetAndOverwriteInput)
(JNIEnv* env, jobject thiz, jlong native_pointer, jobject j_input, jint index,
 jclass input_tool_type_class) {
  const InProgressStroke& in_progress_stroke =
      GetInProgressStrokeWrapper(native_pointer)->in_progress_stroke;
  StrokeInput input = in_progress_stroke.GetInputs().Get(index);
  UpdateJObjectInput(env, input, j_input, input_tool_type_class);
}

JNI_METHOD(strokes, InProgressStroke, jint, nativeBrushCoatCount)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  const InProgressStroke& in_progress_stroke =
      GetInProgressStrokeWrapper(native_pointer)->in_progress_stroke;
  return in_progress_stroke.BrushCoatCount();
}

JNI_METHOD(strokes, InProgressStroke, void, nativeGetMeshBounds)
(JNIEnv* env, jobject thiz, jlong native_pointer, jint coat_index,
 jobject j_out_envelope) {
  const InProgressStroke& in_progress_stroke =
      GetInProgressStrokeWrapper(native_pointer)->in_progress_stroke;
  FillJMutableEnvelope(env, in_progress_stroke.GetMeshBounds(coat_index),
                       j_out_envelope);
}

JNI_METHOD(strokes, InProgressStroke, void, nativeFillUpdatedRegion)
(JNIEnv* env, jobject thiz, jlong native_pointer, jobject j_out_envelope) {
  const InProgressStroke& in_progress_stroke =
      GetInProgressStrokeWrapper(native_pointer)->in_progress_stroke;
  const Envelope& updated_region = in_progress_stroke.GetUpdatedRegion();
  FillJMutableEnvelope(env, updated_region, j_out_envelope);
}

JNI_METHOD(strokes, InProgressStroke, void, nativeResetUpdatedRegion)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  InProgressStroke& in_progress_stroke =
      GetInProgressStrokeWrapper(native_pointer)->in_progress_stroke;
  in_progress_stroke.ResetUpdatedRegion();
}

JNI_METHOD(strokes, InProgressStroke, jint, nativeGetOutlineCount)
(JNIEnv* env, jobject thiz, jlong native_pointer, jint coat_index) {
  const InProgressStroke& in_progress_stroke =
      GetInProgressStrokeWrapper(native_pointer)->in_progress_stroke;
  return in_progress_stroke.GetCoatOutlines(coat_index).size();
}

JNI_METHOD(strokes, InProgressStroke, jint, nativeGetOutlineVertexCount)
(JNIEnv* env, jobject thiz, jlong native_pointer, jint coat_index,
 jint outline_index) {
  return GetInProgressStrokeWrapper(native_pointer)
      ->in_progress_stroke.GetCoatOutlines(coat_index)[outline_index]
      .size();
}

JNI_METHOD(strokes, InProgressStroke, void, nativeFillOutlinePosition)
(JNIEnv* env, jobject thiz, jlong native_pointer, jint coat_index,
 jint outline_index, jint outline_vertex_index, jobject out_position) {
  const InProgressStroke& in_progress_stroke =
      GetInProgressStrokeWrapper(native_pointer)->in_progress_stroke;
  absl::Span<const uint32_t> outline =
      in_progress_stroke.GetCoatOutlines(coat_index)[outline_index];
  // TODO: b/294561921 - Implement multiple meshes.
  Point position = in_progress_stroke.GetMesh(coat_index)
                       .VertexPosition(outline[outline_vertex_index]);
  FillJMutableVecFromPoint(env, out_position, position);
}

JNI_METHOD(strokes, InProgressStroke, jint, nativeGetMeshPartitionCount)
(JNIEnv* env, jobject thiz, jlong native_pointer, jint coat_index) {
  // TODO: b/294561921 - Implement multiple meshes.
  return 1;
}

JNI_METHOD(strokes, InProgressStroke, jint, nativeGetVertexCount)
(JNIEnv* env, jobject thiz, jlong native_pointer, jint coat_index,
 jint mesh_index) {
  const InProgressStroke& in_progress_stroke =
      GetInProgressStrokeWrapper(native_pointer)->in_progress_stroke;
  // TODO: b/294561921 - Implement multiple meshes.
  return in_progress_stroke.GetMesh(coat_index).VertexCount();
}

JNI_METHOD(strokes, InProgressStroke, jobject, nativeGetRawVertexData)
(JNIEnv* env, jobject thiz, jlong native_pointer, jint coat_index,
 jint mesh_index) {
  const InProgressStroke& in_progress_stroke =
      GetInProgressStrokeWrapper(native_pointer)->in_progress_stroke;
  // TODO: b/294561921 - Implement multiple meshes.
  const absl::Span<const std::byte> raw_vertex_data =
      in_progress_stroke.GetMesh(coat_index).RawVertexData();
  if (raw_vertex_data.data() == nullptr) return nullptr;
  return env->NewDirectByteBuffer(
      // NewDirectByteBuffer needs a non-const void*. The resulting buffer is
      // writeable, but it will be wrapped at the Kotlin layer in a read-only
      // buffer that delegates to this one.
      const_cast<std::byte*>(raw_vertex_data.data()), raw_vertex_data.size());
}

// Returns a direct byte buffer of the triangle index data in 16-bit format.
// Automatically converts 32-bit to 16-bit data internally if needed, which
// omits any triangles beyond the point where index values first exceed the
// 16-bit maximum value. Those triangles are still present in the underlying
// data and will be included in `CopyToStroke`, but will not be returned by this
// method (which is typically used for rendering).
// TODO: b/294561921 - Simplify this when the underlying index data is in 16 bit
//   values.
JNI_METHOD(strokes, InProgressStroke, jobject, nativeGetRawTriangleIndexData)
(JNIEnv* env, jobject thiz, jlong native_pointer, jint coat_index,
 jint mesh_index) {
  ABSL_CHECK_EQ(mesh_index, 0) << "Unsupported mesh index: " << mesh_index;
  auto in_progress_stroke_wrapper = GetInProgressStrokeWrapper(native_pointer);
  const InProgressStroke& in_progress_stroke =
      in_progress_stroke_wrapper->in_progress_stroke;

  const MutableMesh& mesh = in_progress_stroke.GetMesh(coat_index);
  size_t index_stride = mesh.IndexStride();
  ABSL_CHECK(index_stride == sizeof(uint32_t))
      << "Unsupported index stride: " << index_stride;

  // If necessary, expand the list of caches.
  std::vector<std::vector<uint16_t>>& triangle_index_data_caches =
      in_progress_stroke_wrapper->triangle_index_data_caches;
  if (coat_index >= static_cast<int>(triangle_index_data_caches.size())) {
    triangle_index_data_caches.resize(coat_index + 1);
  }

  std::vector<uint16_t>& triangle_index_data_cache =
      triangle_index_data_caches[coat_index];
  // Clear the contents, but don't give up any of the capacity because it will
  // be filled again right away.
  triangle_index_data_cache.clear();
  const absl::Span<const std::byte> raw_index_data = mesh.RawIndexData();
  uint32_t index_count = 3 * mesh.TriangleCount();
  for (uint32_t i = 0; i < index_count; ++i) {
    uint32_t i_byte = sizeof(uint32_t) * i;
    uint32_t triangle_index_32;
    // Interpret each set of 4 bytes as a 32-bit integer.
    std::memcpy(&triangle_index_32, &raw_index_data[i_byte], sizeof(uint32_t));
    // If that 32-bit integer would not fit into a 16-bit integer, then stop
    // copying content and return all the triangles that have been processed so
    // far.
    if (triangle_index_32 > std::numeric_limits<uint16_t>::max()) {
      // The offending index may have been in the middle of a triangle, so
      // rewind back to the previous multiple of 3 to just return whole
      // triangles.
      uint32_t i_last_multiple_of_3 = (i / 3) * 3;
      triangle_index_data_cache.erase(
          triangle_index_data_cache.begin() + i_last_multiple_of_3,
          triangle_index_data_cache.end());
      break;
    }
    uint16_t triangle_index_16 = triangle_index_32;
    triangle_index_data_cache.push_back(triangle_index_16);
  }

  ABSL_CHECK_EQ(triangle_index_data_cache.size() % 3, 0);

  // This direct byte buffer is writeable, but it will be wrapped at the Kotlin
  // layer in a read-only buffer that delegates to this one.
  if (triangle_index_data_cache.data() == nullptr) return nullptr;
  return env->NewDirectByteBuffer(
      triangle_index_data_cache.data(),
      triangle_index_data_cache.size() * sizeof(uint16_t));
}

JNI_METHOD(strokes, InProgressStroke, jint, nativeGetTriangleIndexStride)
(JNIEnv* env, jobject thiz, jlong native_pointer, jint mesh_index) {
  // The data is converted from uint32_t above in nativeGetRawTriangleIndexData.
  return sizeof(uint16_t);
}

// Return a newly allocated copy of the given `Mesh`'s `MeshFormat`.
JNI_METHOD(strokes, InProgressStroke, jlong, nativeAllocMeshFormatCopy)
(JNIEnv* env, jobject thiz, jlong native_pointer, jint coat_index,
 jint mesh_index) {
  const InProgressStroke& in_progress_stroke =
      GetInProgressStrokeWrapper(native_pointer)->in_progress_stroke;
  // TODO: b/294561921 - Implement multiple meshes.
  auto copied_format = std::make_unique<MeshFormat>(
      in_progress_stroke.GetMesh(coat_index).Format());
  return reinterpret_cast<jlong>(copied_format.release());
}

}  // extern "C"
