#include "ink/strokes/internal/jni/in_progress_stroke_jni_helper.h"

#include <jni.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <vector>

#include "absl/base/nullability.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/status/status.h"
#include "absl/types/span.h"
#include "ink/brush/brush.h"
#include "ink/geometry/mutable_mesh.h"
#include "ink/types/duration.h"

namespace ink::jni {

int InProgressStrokeWrapper::VertexCount(jint coat_index,
                                         jint mesh_partition_index) const {
  // TODO: b/294561921 - Implement multiple mesh partitions.
  return in_progress_stroke_.GetMesh(coat_index).VertexCount();
}

void InProgressStrokeWrapper::Start(const Brush& brush, int noise_seed) {
  in_progress_stroke_.Start(brush, noise_seed);
  UpdateCaches();
}

absl::Status InProgressStrokeWrapper::UpdateShape(
    Duration32 current_elapsed_time) {
  if (absl::Status status =
          in_progress_stroke_.UpdateShape(current_elapsed_time);
      !status.ok()) {
    return status;
  }
  UpdateCaches();
  return absl::OkStatus();
}

void InProgressStrokeWrapper::UpdateCaches() {
  int coat_count = in_progress_stroke_.BrushCoatCount();
  coat_buffer_caches_.resize(coat_count);
  for (int coat_index = 0; coat_index < coat_count; ++coat_index) {
    UpdateCache(coat_index);
  }
}

void InProgressStrokeWrapper::UpdateCache(int coat_index) {
  const MutableMesh& mesh = in_progress_stroke_.GetMesh(coat_index);
  Cache& cache = coat_buffer_caches_[coat_index];
  size_t index_stride = mesh.IndexStride();
  ABSL_CHECK_EQ(index_stride, sizeof(uint32_t))
      << "Unsupported index stride: " << index_stride;
  // Clear the contents, but don't give up any of the capacity because it will
  // be filled again right away.
  cache.triangle_index_data.clear();
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
      cache.triangle_index_data.erase(
          cache.triangle_index_data.begin() + i_last_multiple_of_3,
          cache.triangle_index_data.end());
      ABSL_LOG_EVERY_N_SEC(WARNING, 1)
          << "Triangle index data exceeds 16-bit limit, truncating.";
      break;
    }
    uint16_t triangle_index_16 = triangle_index_32;
    cache.triangle_index_data.push_back(triangle_index_16);
  }
  ABSL_CHECK_EQ(cache.triangle_index_data.size() % 3, 0u);
}

int InProgressStrokeWrapper::MeshPartitionCount(jint coat_index) const {
  // TODO: b/294561921 - Implement multiple mesh partitions.
  return 1;
}

absl_nullable jobject InProgressStrokeWrapper::GetUnsafelyMutableRawVertexData(
    JNIEnv* env, int coat_index, jint mesh_partition_index) const {
  // TODO: b/294561921 - Implement multiple mesh partitions.
  ABSL_CHECK_EQ(mesh_partition_index, 0)
      << "Unsupported mesh partition index: " << mesh_partition_index;
  ABSL_CHECK_LT(coat_index, coat_buffer_caches_.size());
  const absl::Span<const std::byte> raw_vertex_data =
      in_progress_stroke_.GetMesh(coat_index).RawVertexData();
  // absl::Span::data() may be nullptr if empty, which NewDirectByteBuffer does
  // not permit (even if the size is zero).
  if (raw_vertex_data.data() == nullptr) {
    return nullptr;
  }
  return env->NewDirectByteBuffer(
      // NewDirectByteBuffer needs a non-const void*. The resulting buffer is
      // writeable, but it will be wrapped at the Kotlin layer in a read-only
      // buffer that delegates to this one.
      const_cast<std::byte*>(raw_vertex_data.data()), raw_vertex_data.size());
}

absl_nullable jobject
InProgressStrokeWrapper::GetUnsafelyMutableRawTriangleIndexData(
    JNIEnv* env, int coat_index, jint mesh_partition_index) const {
  // TODO: b/294561921 - Implement multiple mesh partitions.
  ABSL_CHECK_EQ(mesh_partition_index, 0)
      << "Unsupported mesh partition index: " << mesh_partition_index;
  ABSL_CHECK_LT(coat_index, coat_buffer_caches_.size());
  const std::vector<uint16_t>& triangle_index_data =
      coat_buffer_caches_[coat_index].triangle_index_data;
  // std::vector::data() may be nullptr if empty, which NewDirectByteBuffer
  // does not permit (even if the size is zero).
  if (triangle_index_data.data() == nullptr) {
    return nullptr;
  }
  return env->NewDirectByteBuffer(
      // NewDirectByteBuffer needs a non-const void*. The resulting buffer
      // is writeable, but it will be wrapped at the Kotlin layer in a
      // read-only buffer that delegates to this one. This one needs to be
      // compatible with ShortBuffer, which expects 16-bit values.
      const_cast<uint16_t*>(triangle_index_data.data()),
      triangle_index_data.size() * sizeof(uint16_t));
}

}  // namespace ink::jni
