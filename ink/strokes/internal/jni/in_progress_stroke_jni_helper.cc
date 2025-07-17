#include "ink/strokes/internal/jni/in_progress_stroke_jni_helper.h"

#include <jni.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
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

namespace {

using internal::PartitionedCoatIndices;
using internal::UpdatePartitionedCoatIndices;

}  // namespace

int InProgressStrokeWrapper::VertexCount(jint coat_index,
                                         jint mesh_partition_index) const {
  ABSL_CHECK_LT(coat_index, coat_buffer_partitions_.size());
  ABSL_CHECK_LT(mesh_partition_index,
                coat_buffer_partitions_[coat_index].partitions.size());
  return coat_buffer_partitions_[coat_index]
      .partitions[mesh_partition_index]
      .vertex_buffer_size;
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
  coat_buffer_partitions_.resize(coat_count);
  for (int coat_index = 0; coat_index < coat_count; ++coat_index) {
    UpdateCache(coat_index);
  }
}

namespace internal {

void UpdatePartitionedCoatIndices(absl::Span<const uint32_t> index_data,
                                  PartitionedCoatIndices& cache) {
  constexpr int kMaxVertexIndexInPartition =
      std::numeric_limits<uint16_t>::max();
  // Clear the contents, but don't give up any of the capacity because it will
  // be filled again right away.
  cache.converted_index_buffer.clear();
  cache.partitions.clear();
  // Start the first partition, vertex_offset and index_offset start at 0. This
  // avoids an extra linear pass in the common case where everything fits in
  // 16-bit indices.
  cache.partitions.emplace_back();
  int index_count = index_data.size();
  for (int i = 0; i < index_count; ++i) {
    uint32_t overall_vertex_index = index_data[i];
    uint32_t current_vertex_offset =
        cache.partitions.back().vertex_buffer_offset;
    int vertex_index_in_partition =
        overall_vertex_index - current_vertex_offset;

    // If this fits into the current partition, add it to the buffer and
    // update where the partition's portion of the vertex buffer ends.
    if (vertex_index_in_partition <= kMaxVertexIndexInPartition) {
      cache.converted_index_buffer.push_back(vertex_index_in_partition);
      PartitionedCoatIndices::Partition& current_partition =
          cache.partitions.back();
      current_partition.vertex_buffer_size = std::max(
          current_partition.vertex_buffer_size, vertex_index_in_partition + 1);
      continue;
    }

    // Otherwise, we need to resize down to the last complete triangle and
    // start a new partition.
    cache.converted_index_buffer.resize(i / 3 * 3);
    ABSL_LOG_EVERY_N_SEC(WARNING, 1)
        << "Triangle index data exceeds 16-bit limit, attempting to "
        << "partition into " << cache.partitions.size() + 1 << " partitions.";
    // Rewind to just before the new partition.
    i = cache.converted_index_buffer.size() - 1;
    // Find the next span of the index buffer that can be represented as
    // 16-bit indices into a subspan of the vertex buffer.
    uint32_t max_later_overall_vertex_index = 0;
    uint32_t min_later_overall_vertex_index =
        std::numeric_limits<uint32_t>::max();
    for (int later_i = i + 1; later_i < index_count; ++later_i) {
      uint32_t later_overall_vertex_index = index_data[later_i];
      min_later_overall_vertex_index =
          std::min(min_later_overall_vertex_index, later_overall_vertex_index);
      max_later_overall_vertex_index =
          std::max(max_later_overall_vertex_index, later_overall_vertex_index);
      if (max_later_overall_vertex_index - min_later_overall_vertex_index >
          kMaxVertexIndexInPartition) {
        // Speaking of hopefully unlikely edge-cases, we do need to be able to
        // fit at least one triangle into a partition to make progress and
        // avoid an infinite loop. Bail out if the next triangle's vertex
        // indices can't fit within a 16-bit-max span.
        if (later_i - i <= 3) {
          ABSL_LOG_EVERY_N_SEC(ERROR, 1)
              << "Partitioning failed because the span of the next "
              << "triangle's vertices is more than the 16-bit limit, giving "
              << "up and truncating.";
          return;
        }
        break;
      }
    }
    // The _first_ index is very unlikely to exceed the 16-bit limit. But for
    // full generality, avoid creating an extra empty partition in that case.
    // If the partition is non-empty, then close it and start a new one.
    if (!cache.converted_index_buffer.empty()) {
      cache.partitions.emplace_back();
    }
    // Set the start bounds of the new partition.
    PartitionedCoatIndices::Partition& current_partition =
        cache.partitions.back();
    current_partition.index_buffer_offset = i + 1;
    current_partition.vertex_buffer_offset = min_later_overall_vertex_index;
  }
}

}  // namespace internal

void InProgressStrokeWrapper::UpdateCache(int coat_index) {
  const MutableMesh& mesh = in_progress_stroke_.GetMesh(coat_index);
  ABSL_CHECK_EQ(mesh.IndexStride(), sizeof(uint32_t))
      << "Unsupported index stride: " << mesh.IndexStride();
  const absl::Span<const std::byte>& raw_index_data = mesh.RawIndexData();
  int index_count = raw_index_data.size() / sizeof(uint32_t);
  ABSL_CHECK_EQ(index_count, mesh.TriangleCount() * 3);
  UpdatePartitionedCoatIndices(
      absl::MakeConstSpan(
          reinterpret_cast<const uint32_t*>(raw_index_data.data()),
          index_count),
      coat_buffer_partitions_[coat_index]);
}

int InProgressStrokeWrapper::MeshPartitionCount(jint coat_index) const {
  ABSL_CHECK_LT(coat_index, coat_buffer_partitions_.size());
  return coat_buffer_partitions_[coat_index].partitions.size();
}

absl_nullable jobject InProgressStrokeWrapper::GetUnsafelyMutableRawVertexData(
    JNIEnv* env, int coat_index, jint mesh_partition_index) const {
  ABSL_CHECK_LT(coat_index, coat_buffer_partitions_.size());
  ABSL_CHECK_LT(mesh_partition_index,
                coat_buffer_partitions_[coat_index].partitions.size());
  const absl::Span<const std::byte> raw_vertex_data =
      in_progress_stroke_.GetMesh(coat_index).RawVertexData();
  // absl::Span::data() may be nullptr if empty, which NewDirectByteBuffer does
  // not permit (even if the size is zero).
  if (raw_vertex_data.data() == nullptr) {
    return nullptr;
  }
  const PartitionedCoatIndices::Partition& partition =
      coat_buffer_partitions_[coat_index].partitions[mesh_partition_index];
  uint16_t vertex_stride =
      in_progress_stroke_.GetMesh(coat_index).VertexStride();
  ABSL_CHECK_LE(0, partition.vertex_buffer_offset);
  ABSL_CHECK_LE(
      (partition.vertex_buffer_offset + partition.vertex_buffer_size) *
          vertex_stride,
      raw_vertex_data.size());
  return env->NewDirectByteBuffer(
      // NewDirectByteBuffer needs a non-const void*. The resulting buffer is
      // writeable, but it will be wrapped at the Kotlin layer in a read-only
      // buffer that delegates to this one.
      const_cast<std::byte*>(raw_vertex_data.data()) +
          partition.vertex_buffer_offset * vertex_stride,
      partition.vertex_buffer_size * vertex_stride);
}

absl_nullable jobject
InProgressStrokeWrapper::GetUnsafelyMutableRawTriangleIndexData(
    JNIEnv* env, int coat_index, jint mesh_partition_index) const {
  ABSL_CHECK_LT(coat_index, coat_buffer_partitions_.size());
  ABSL_CHECK_LT(mesh_partition_index,
                coat_buffer_partitions_[coat_index].partitions.size());
  const PartitionedCoatIndices& cache = coat_buffer_partitions_[coat_index];
  const std::vector<uint16_t>& triangle_index_data =
      cache.converted_index_buffer;
  // std::vector::data() may be nullptr if empty, which NewDirectByteBuffer
  // does not permit (even if the size is zero).
  if (triangle_index_data.data() == nullptr) {
    return nullptr;
  }
  const PartitionedCoatIndices::Partition& partition =
      cache.partitions[mesh_partition_index];
  ABSL_CHECK_LE(0, partition.index_buffer_offset);
  int next_partition_index_buffer_offset =
      mesh_partition_index == static_cast<int>(cache.partitions.size()) - 1
          ? triangle_index_data.size()
          : cache.partitions[mesh_partition_index + 1].index_buffer_offset;
  int partition_index_buffer_size =
      next_partition_index_buffer_offset - partition.index_buffer_offset;
  ABSL_CHECK_LE(partition.index_buffer_offset + partition_index_buffer_size,
                triangle_index_data.size());
  return env->NewDirectByteBuffer(
      // NewDirectByteBuffer needs a non-const void*. The resulting buffer
      // is writeable, but it will be wrapped at the Kotlin layer in a
      // read-only buffer that delegates to this one. This one needs to be
      // compatible with ShortBuffer, which expects 16-bit values.
      const_cast<uint16_t*>(triangle_index_data.data() +
                            partition.index_buffer_offset),
      partition_index_buffer_size * sizeof(uint16_t));
}

}  // namespace ink::jni
