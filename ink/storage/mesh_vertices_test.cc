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

#include "ink/storage/mesh_vertices.h"

#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "absl/status/status_matchers.h"
#include "absl/status/statusor.h"
#include "ink/geometry/type_matchers.h"
#include "ink/storage/proto/mesh.pb.h"
#include "ink/strokes/internal/legacy_vertex.h"
#include "ink/types/iterator_range.h"
#include "google/protobuf/text_format.h"

namespace ink {
namespace {

using ::absl_testing::IsOk;
using ::absl_testing::IsOkAndHolds;
using ::absl_testing::StatusIs;
using ::ink::proto::CodedMesh;
using ::google::protobuf::TextFormat;
using ::testing::ElementsAre;
using ::testing::HasSubstr;
using ::testing::SizeIs;

TEST(CodedMeshVertexIteratorTest, DecodeTriangleMesh) {
  CodedMesh coded;
  ASSERT_TRUE(TextFormat::ParseFromString(
      R"pb(
        triangle_index { deltas: [ 0, 1, 1 ] }
        x_stroke_space {
          scale: 2.5
          deltas: [ 1, 2, 1 ]
        }
        y_stroke_space {
          scale: 2.5
          deltas: [ 1, -1, 1 ]
        }
      )pb",
      &coded));

  absl::StatusOr<iterator_range<CodedMeshVertexIterator>> range =
      DecodeMeshVertices(coded);
  ASSERT_THAT(range, IsOk());
  std::vector<strokes_internal::LegacyVertex> vertices(range->begin(),
                                                       range->end());
  ASSERT_THAT(vertices, SizeIs(3));

  EXPECT_THAT(vertices[0].position, PointEq({2.5f, 2.5f}));
  EXPECT_THAT(vertices[1].position, PointEq({7.5f, 0.0f}));
  EXPECT_THAT(vertices[2].position, PointEq({10.0f, 2.5f}));
}

TEST(CodedMeshVertexIteratorTest, DecodeEmptyMesh) {
  CodedMesh coded;
  EXPECT_THAT(DecodeMeshVertices(coded), IsOkAndHolds(ElementsAre()));
}

TEST(CodedMeshVertexIteratorTest, DecodeMeshWithMismatchedLengths) {
  CodedMesh coded;
  coded.mutable_y_stroke_space()->add_deltas(1);
  EXPECT_THAT(DecodeMeshVertices(coded),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("mismatched numeric run lengths")));
}

TEST(CodedMeshVertexIteratorTest, IteratorPostIncrement) {
  CodedMesh coded;
  ASSERT_TRUE(TextFormat::ParseFromString(
      R"pb(
        triangle_index { deltas: [ 0, 1, 1 ] }
        x_stroke_space { deltas: [ 1, 2, 1 ] }
        y_stroke_space { deltas: [ 1, -1, 1 ] }
      )pb",
      &coded));

  absl::StatusOr<iterator_range<CodedMeshVertexIterator>> range =
      DecodeMeshVertices(coded);
  ASSERT_THAT(range, IsOk());
  CodedMeshVertexIterator iter = range->begin();
  const CodedMeshVertexIterator iter0 = iter++;
  const CodedMeshVertexIterator iter1 = iter++;
  const CodedMeshVertexIterator iter2 = iter++;

  EXPECT_THAT(iter0->position, PointEq({1, 1}));
  EXPECT_THAT(iter1->position, PointEq({3, 0}));
  EXPECT_THAT(iter2->position, PointEq({4, 1}));
  EXPECT_EQ(iter, range->end());
}

}  // namespace
}  // namespace ink
