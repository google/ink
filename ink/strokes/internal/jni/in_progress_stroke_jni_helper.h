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
#include "absl/types/span.h"
#include "ink/brush/brush.h"
#include "ink/strokes/in_progress_stroke.h"
#include "ink/types/duration.h"

namespace ink::jni {

namespace internal {

struct PartitionedCoatIndices {
  struct Partition {
    // The offset of the start of this partition of the index buffer. The
    // partition ends at the start of the next partition, or the start of the
    // whole buffer for the last partition.
    int index_buffer_offset = 0;

    // The first vertex in this partition. This is one of the raw 32-bit indices
    // into the vertex buffer.
    uint32_t vertex_buffer_offset = 0;

    // The vertex buffer size, which comes from the difference between two
    // 32-bit indices. This doesn't quite fit in a 16-bit index, since the
    // max partition size is the max 16-bit index plus one. This needs to be
    // tracked separately from vertex_buffer_offset because the partitions of
    // the vertex buffer may overlap.
    int vertex_buffer_size = 0;
  };

  // The whole index buffer for a coat, converted to 16-bit indices from
  // 32-bit. Note that these indices index into the relevant partition of the
  // vertex buffer, not the overall vertex buffer. This can differ if the
  // indices exceed the 16-bit limit.
  //
  // The vertex buffer for the coat lives in
  // in_progress_stroke_.GetMesh(coat_index).RawVertexData(), and portions of
  // that can be used as-is for each partition's vertex buffer.
  std::vector<uint16_t> converted_index_buffer;

  // Partitions of the index buffer and vertex buffer. If the range of indices
  // exceeds the 16-bit limit, the index buffer will be split into
  // non-overlapping partitions (the corresponding partitions of the vertex
  // buffer may overlap).
  std::vector<Partition> partitions;
};

// Exposes the core of InProgressStrokeWrapper::UpdateCache(int coat_index)
// for testing.
void UpdatePartitionedCoatIndices(absl::Span<const uint32_t> index_data,
                                  PartitionedCoatIndices& cache);

}  // namespace internal

// Associates an `InProgressStroke` with a cached triangle index buffer instance
// that is used for converting 32-bit indices to 16-bit indices, without needing
// to allocate a new buffer for this conversion every time.
//
// TODO: b/294561921 - Change MutableMesh to create partitions as it goes and
// always use 16-bit indices, then remove this wrapper.
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
  void UpdateCaches();
  void UpdateCache(int coat_index);

  InProgressStroke in_progress_stroke_;

  // For each brush coat, holds data underlying ShortBuffers of indices used for
  // mesh rendering. Since the indices need to fit in a ShortBuffer, they are
  // converted from 32-bit to 16-bit and possibly partitioned into multiple
  // buffers underlying multiple meshes.
  std::vector<internal::PartitionedCoatIndices> coat_buffer_partitions_;
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
