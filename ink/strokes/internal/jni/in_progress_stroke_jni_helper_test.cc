#include "ink/strokes/internal/jni/in_progress_stroke_jni_helper.h"

#include <cstdint>
#include <limits>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace ink::jni {
namespace {

using ::ink::jni::internal::PartitionedCoatIndices;
using ::ink::jni::internal::UpdatePartitionedCoatIndices;
using ::testing::ElementsAre;
using ::testing::Field;
using ::testing::IsEmpty;
using ::testing::Matcher;

constexpr int kMax16BitIndex = std::numeric_limits<uint16_t>::max();

Matcher<PartitionedCoatIndices::Partition> PartitionIs(
    const PartitionedCoatIndices::Partition& expected) {
  return AllOf(Field("index_buffer_offset",
                     &PartitionedCoatIndices::Partition::index_buffer_offset,
                     expected.index_buffer_offset),
               Field("vertex_buffer_offset",
                     &PartitionedCoatIndices::Partition::vertex_buffer_offset,
                     expected.vertex_buffer_offset),
               Field("vertex_buffer_size",
                     &PartitionedCoatIndices::Partition::vertex_buffer_size,
                     expected.vertex_buffer_size));
}

TEST(UpdatePartitionedCoatIndicesTest,
     UpdateCacheCreatesFullPartitionIfPossible) {
  std::vector<uint32_t> indices = {10, 11, 15};
  PartitionedCoatIndices cache{
      .converted_index_buffer = {500, 501, 502},
      .partitions = {{.index_buffer_offset = 10,
                      .vertex_buffer_offset = 20,
                      .vertex_buffer_size = 30},
                     {}},
  };
  UpdatePartitionedCoatIndices(indices, cache);
  EXPECT_THAT(cache.converted_index_buffer, ElementsAre(10, 11, 15));
  EXPECT_THAT(cache.partitions,
              ElementsAre(PartitionIs({
                  // This starts at 0 to avoid a scan to find the
                  // start offset.
                  .index_buffer_offset = 0,
                  .vertex_buffer_offset = 0,
                  .vertex_buffer_size = 16,
              })));
}

TEST(UpdatePartitionedCoatIndicesTest, ClearsExistingData) {
  std::vector<uint32_t> indices = {1, 2, 3};
  PartitionedCoatIndices cache{
      .converted_index_buffer = {500, 501, 502},
      .partitions = {{.index_buffer_offset = 10,
                      .vertex_buffer_offset = 20,
                      .vertex_buffer_size = 30}},
  };
  UpdatePartitionedCoatIndices(indices, cache);
  EXPECT_THAT(cache.converted_index_buffer, ElementsAre(1, 2, 3));
  EXPECT_THAT(cache.partitions, ElementsAre(PartitionIs({
                                    .index_buffer_offset = 0,
                                    .vertex_buffer_offset = 0,
                                    .vertex_buffer_size = 4,
                                })));
}

TEST(UpdatePartitionedCoatIndicesTest,
     CreatesSingleEmptyPartitionForEmptyIndices) {
  std::vector<uint32_t> indices = {};
  PartitionedCoatIndices cache;
  UpdatePartitionedCoatIndices(indices, cache);
  EXPECT_THAT(cache.converted_index_buffer, IsEmpty());
  EXPECT_THAT(cache.partitions, ElementsAre(PartitionIs({
                                    .index_buffer_offset = 0,
                                    .vertex_buffer_offset = 0,
                                    .vertex_buffer_size = 0,
                                })));
}

TEST(UpdatePartitionedCoatIndicesTest, PartitionsAtWholeTriangleBounds) {
  std::vector<uint32_t> indices = {
      // First partition.
      10, 11, 12, 13, 14, 15,
      // Second partition. Even though it's the second vertex of this triangle
      // that exceeds the bounds, the partition starts at the triangle.
      20, kMax16BitIndex + 19, 21,
      // Third partition. This time the first vertex that exceeds the bounds.
      // It still chooses the min vertex, not necessarily the first vertex, as
      // the lower bound of the partition. That lower bound is also not the
      // first
      // value that would have been out of bounds in the previous partition.
      kMax16BitIndex + 22, kMax16BitIndex + 21, kMax16BitIndex + 23};
  PartitionedCoatIndices cache;
  UpdatePartitionedCoatIndices(indices, cache);
  EXPECT_THAT(cache.converted_index_buffer, ElementsAre(
                                                // First partition
                                                10, 11, 12, 13, 14, 15,
                                                // Second partition
                                                0, kMax16BitIndex - 1, 1,
                                                // Third partition
                                                1, 0, 2));
  EXPECT_THAT(cache.partitions,
              ElementsAre(PartitionIs({
                              .index_buffer_offset = 0,
                              .vertex_buffer_offset = 0,
                              // Note this is a little oversized because we
                              // don't bother keeping track of the max vertex
                              // at the last complete triangle so we can
                              // backtrack to it. Seems not worth the extra
                              // complexity.
                              .vertex_buffer_size = 21,
                          }),
                          PartitionIs({
                              .index_buffer_offset = 6,
                              .vertex_buffer_offset = 20,
                              .vertex_buffer_size = kMax16BitIndex,
                          }),
                          PartitionIs({
                              .index_buffer_offset = 9,
                              .vertex_buffer_offset = kMax16BitIndex + 21,
                              .vertex_buffer_size = 3,
                          })));
}

TEST(UpdatePartitionedCoatIndicesTest,
     FirstIndexExceeding16BitLimitAvoidsCreatingEmptyPartition) {
  std::vector<uint32_t> indices = {kMax16BitIndex + 10, kMax16BitIndex + 11,
                                   kMax16BitIndex + 15};
  PartitionedCoatIndices cache;
  UpdatePartitionedCoatIndices(indices, cache);
  EXPECT_THAT(cache.converted_index_buffer, ElementsAre(0, 1, 5));
  EXPECT_THAT(cache.partitions, ElementsAre(PartitionIs({
                                    .index_buffer_offset = 0,
                                    .vertex_buffer_offset = kMax16BitIndex + 10,
                                    .vertex_buffer_size = 6,
                                })));
}

TEST(UpdatePartitionedCoatIndicesTest,
     TriangleSpanningMoreThan16BitLimitTruncates) {
  std::vector<uint32_t> indices = {1, 2, 3, 4, 5, kMax16BitIndex + 5};
  PartitionedCoatIndices cache;
  UpdatePartitionedCoatIndices(indices, cache);
  // There's no way to span that second triangle, so we truncate to the end
  // of the first triangle and give up.
  EXPECT_THAT(cache.converted_index_buffer, ElementsAre(1, 2, 3));
  EXPECT_THAT(cache.partitions, ElementsAre(PartitionIs({
                                    .index_buffer_offset = 0,
                                    .vertex_buffer_offset = 0,
                                    // This partition is slightly oversized, but
                                    // that's expected.
                                    .vertex_buffer_size = 6,
                                })));
}

}  // namespace
}  // namespace ink::jni
