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

#ifndef INK_STROKES_INTERNAL_JNI_IN_PROGRESS_STROKE_JNI_HELPER_H_
#define INK_STROKES_INTERNAL_JNI_IN_PROGRESS_STROKE_JNI_HELPER_H_

#include <jni.h>

#include <cstdint>
#include <vector>

#include "absl/base/nullability.h"
#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "ink/brush/brush.h"
#include "ink/strokes/in_progress_stroke.h"
#include "ink/types/duration.h"

namespace ink::jni {

// Associates an `InProgressStroke` with a cached triangle index buffer instance
// that is used for converting 32-bit indices to 16-bit indices, without needing
// to allocate a new buffer for this conversion every time.

class InProgressStrokeWrapper {
 public:
  InProgressStrokeWrapper() = default;
  ~InProgressStrokeWrapper() = default;

  // No move or copy. This should live on the heap and be passed by reference.
  InProgressStrokeWrapper(const InProgressStrokeWrapper&) = delete;
  InProgressStrokeWrapper& operator=(const InProgressStrokeWrapper&) = delete;
  InProgressStrokeWrapper(InProgressStrokeWrapper&&) = delete;
  InProgressStrokeWrapper& operator=(InProgressStrokeWrapper&&) = delete;

  const InProgressStroke& Stroke() const { return in_progress_stroke_; }
  InProgressStroke& Stroke() { return in_progress_stroke_; }

  // Starts a stroke and clears the converted index buffers.
  void Start(const Brush& brush, int noise_seed);

  // Updates the shape of the shape of the stroke and updates the converted
  // index buffers.
  absl::Status UpdateShape(Duration32 current_elapsed_time);

  int MeshPartitionCount(jint coat_index) const;
  int VertexCount(jint coat_index, jint mesh_partition_index) const;
  absl_nullable jobject GetUnsafelyMutableRawVertexData(
      JNIEnv* env, int coat_index, jint mesh_partition_index) const;
  absl_nullable jobject GetUnsafelyMutableRawTriangleIndexData(
      JNIEnv* env, int coat_index, jint mesh_partition_index) const;

 private:
  // For each brush coat, holds ByteBuffers for index and vertex data. For the
  // index buffer, this also holds the underlying array of 16-bit values, since
  // the MutableMesh stores 32-bit indices, but the JNI interface needs
  // ShortBuffer.
  struct Cache {
    std::vector<uint16_t> triangle_index_data;
  };

  void UpdateCaches();
  void UpdateCache(int coat_index);

  InProgressStroke in_progress_stroke_;
  std::vector<Cache> coat_buffer_caches_;
};

// Creates a new stack-allocated
// `ink::jni::InProgressStrokeWrapper` containing an empty
// `InProgressStroke` and returns a pointer to it as a jlong, suitable for
// wrapping in a Kotlin InProgressStroke.
inline jlong NewNativeInProgressStroke() {
  return reinterpret_cast<jlong>(new InProgressStrokeWrapper());
}

// Casts a Kotlin InProgressStroke.nativePointer to a mutable reference to a
// C++ InProgressStrokeWrapper.
inline InProgressStrokeWrapper& CastToMutableInProgressStrokeWrapper(
    jlong native_pointer) {
  ABSL_CHECK_NE(native_pointer, 0);
  return *reinterpret_cast<InProgressStrokeWrapper*>(native_pointer);
}

// Casts a Kotlin InProgressStroke.nativePointer to a const reference to a
// C++ InProgressStrokeWrapper.
inline const InProgressStrokeWrapper& CastToInProgressStrokeWrapper(
    jlong native_pointer) {
  ABSL_CHECK_NE(native_pointer, 0);
  return *reinterpret_cast<const InProgressStrokeWrapper*>(native_pointer);
}

// Frees a Kotlin InProgressStroke.nativePointer.
inline void DeleteNativeInProgressStroke(jlong native_pointer) {
  ABSL_CHECK_NE(native_pointer, 0);
  delete reinterpret_cast<InProgressStrokeWrapper*>(native_pointer);
}

}  // namespace ink::jni

#endif  // INK_STROKES_INTERNAL_JNI_IN_PROGRESS_STROKE_JNI_HELPER_H_
