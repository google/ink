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

#include "ink/storage/mesh.h"

#include <cstdint>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "fuzztest/fuzztest.h"
#include "absl/container/inlined_vector.h"
#include "absl/status/status_matchers.h"
#include "absl/status/statusor.h"
#include "absl/types/span.h"
#include "ink/geometry/fuzz_domains.h"
#include "ink/geometry/mesh.h"
#include "ink/geometry/mesh_format.h"
#include "ink/geometry/point.h"
#include "ink/geometry/type_matchers.h"
#include "ink/storage/numeric_run.h"
#include "ink/storage/proto/mesh.pb.h"
#include "google/protobuf/text_format.h"

namespace ink {
namespace {

using ::absl_testing::IsOk;
using ::absl_testing::IsOkAndHolds;
using ::ink::proto::CodedMesh;
using ::google::protobuf::TextFormat;
using ::testing::ElementsAre;
using ::testing::FloatNear;
using ::testing::SizeIs;
using AttrType = MeshFormat::AttributeType;
using AttrId = MeshFormat::AttributeId;

TEST(MeshTest, EncodeEmptyMesh) {
  // Start with a non-empty CodedMesh.
  CodedMesh coded_mesh;
  ASSERT_TRUE(TextFormat::ParseFromString(R"pb(
                                            triangle_index { deltas: [ 0 ] }
                                            x_stroke_space { deltas: [ 0 ] }
                                            y_stroke_space { deltas: [ 0 ] }
                                          )pb",
                                          &coded_mesh));
  // Encoding an empty mesh should clear the CodedMesh.
  Mesh mesh;
  EncodeMesh(mesh, coded_mesh);
  EXPECT_FALSE(coded_mesh.has_triangle_index());
  EXPECT_FALSE(coded_mesh.has_x_stroke_space());
  EXPECT_FALSE(coded_mesh.has_y_stroke_space());
}

TEST(MeshTest, EncodeTriangleMesh) {
  // Start with a non-empty CodedMesh.
  CodedMesh coded_mesh;
  ASSERT_TRUE(TextFormat::ParseFromString(R"pb(
                                            triangle_index { deltas: [ 0 ] }
                                            x_stroke_space { deltas: [ 17 ] }
                                            y_stroke_space { deltas: [ 42 ] }
                                          )pb",
                                          &coded_mesh));

  std::vector<float> position_x = {1, 3, 5};
  std::vector<float> position_y = {2, 4, 6};
  std::vector<uint32_t> triangles = {0, 1, 2};

  absl::StatusOr<Mesh> mesh =
      Mesh::Create(MeshFormat(), {position_x, position_y}, triangles);
  ASSERT_THAT(mesh, IsOk());

  EncodeMesh(*mesh, coded_mesh);

  EXPECT_THAT(DecodeIntNumericRun(coded_mesh.triangle_index()),
              IsOkAndHolds(ElementsAre(0, 1, 2)));
  const float epsilon = 1e-3;
  EXPECT_THAT(
      DecodeFloatNumericRun(coded_mesh.x_stroke_space()),
      IsOkAndHolds(ElementsAre(FloatNear(1, epsilon), FloatNear(3, epsilon),
                               FloatNear(5, epsilon))));
  EXPECT_THAT(
      DecodeFloatNumericRun(coded_mesh.y_stroke_space()),
      IsOkAndHolds(ElementsAre(FloatNear(2, epsilon), FloatNear(4, epsilon),
                               FloatNear(6, epsilon))));
}

TEST(MeshTest, EncodeTriangleMeshOmittingFormat) {
  // Start with a non-empty format.
  CodedMesh coded_mesh;
  ASSERT_TRUE(TextFormat::ParseFromString(
      R"pb(
        format {
          attribute_types: [ ATTR_TYPE_FLOAT2_PACKED_IN_ONE_FLOAT ]
          attribute_ids: [ ATTR_ID_POSITION ]
        }
      )pb",
      &coded_mesh));

  absl::StatusOr<Mesh> mesh =
      Mesh::Create(MeshFormat(), {{1, 3, 5}, {2, 4, 6}}, {0, 1, 2});
  ASSERT_THAT(mesh, IsOk());
  EncodeMeshOmittingFormat(*mesh, coded_mesh);

  EXPECT_FALSE(coded_mesh.has_format());
  EXPECT_THAT(DecodeIntNumericRun(coded_mesh.triangle_index()),
              IsOkAndHolds(ElementsAre(0, 1, 2)));
}

TEST(MeshTest, EncodePackedTriangleMesh) {
  std::vector<float> position_x = {1, 3, 5};
  std::vector<float> position_y = {2, 4, 6};
  std::vector<uint32_t> triangles = {0, 1, 2};

  absl::StatusOr<MeshFormat> format = MeshFormat::Create(
      {{AttrType::kFloat2PackedInOneFloat, AttrId::kPosition}},
      MeshFormat::IndexFormat::k16BitUnpacked16BitPacked);
  ASSERT_THAT(format, IsOk());
  absl::StatusOr<Mesh> mesh =
      Mesh::Create(*format, {position_x, position_y}, triangles);
  ASSERT_THAT(mesh, IsOk());

  CodedMesh coded_mesh;
  EncodeMesh(*mesh, coded_mesh);

  EXPECT_THAT(DecodeIntNumericRun(coded_mesh.triangle_index()),
              IsOkAndHolds(ElementsAre(0, 1, 2)));
  const float epsilon = 1e-3;
  EXPECT_THAT(
      DecodeFloatNumericRun(coded_mesh.x_stroke_space()),
      IsOkAndHolds(ElementsAre(FloatNear(1, epsilon), FloatNear(3, epsilon),
                               FloatNear(5, epsilon))));
  EXPECT_THAT(
      DecodeFloatNumericRun(coded_mesh.y_stroke_space()),
      IsOkAndHolds(ElementsAre(FloatNear(2, epsilon), FloatNear(4, epsilon),
                               FloatNear(6, epsilon))));
}

TEST(MeshTest, EncodeSingleVertexMesh) {
  // Create a degenerate mesh that has only one vertex.
  std::vector<float> position_x = {-1.75};
  std::vector<float> position_y = {2.25};
  std::vector<uint32_t> triangles = {};
  absl::StatusOr<Mesh> mesh =
      Mesh::Create(MeshFormat(), {position_x, position_y}, triangles);
  ASSERT_THAT(mesh, IsOk());
  // Encode the mesh.
  CodedMesh coded_mesh;
  EncodeMesh(*mesh, coded_mesh);
  // The CodedMesh should be valid even though the envelope of vertices is empty
  // (we should avoid calculating an infinite scale).
  EXPECT_THAT(DecodeFloatNumericRun(coded_mesh.x_stroke_space()),
              IsOkAndHolds(ElementsAre(-1.75f)));
  EXPECT_THAT(DecodeFloatNumericRun(coded_mesh.y_stroke_space()),
              IsOkAndHolds(ElementsAre(2.25f)));
}

TEST(MeshTest, EncodeAndDecodeMeshWithCustomFormat) {
  // Create a mesh format with custom, non-position attributes.
  absl::StatusOr<MeshFormat> format = MeshFormat::Create(
      {{AttrType::kFloat2PackedInOneFloat, AttrId::kPosition},
       {AttrType::kFloat1Unpacked, AttrId::kCustom0}},
      MeshFormat::IndexFormat::k32BitUnpacked16BitPacked);
  ASSERT_THAT(format, IsOk());

  // Create a mesh using that format.
  std::vector<float> position_x = {1, 3, 5};
  std::vector<float> position_y = {2, 4, 6};
  std::vector<float> custom_attr = {-1, 7, 3};
  std::vector<uint32_t> triangles = {0, 1, 2};
  absl::StatusOr<Mesh> mesh =
      Mesh::Create(*format, {position_x, position_y, custom_attr}, triangles);
  ASSERT_THAT(mesh, IsOk());

  // Encode the mesh and ensure that the custom attribute data is included.
  CodedMesh coded_mesh;
  EncodeMesh(*mesh, coded_mesh);
  ASSERT_EQ(coded_mesh.other_attribute_components_size(), 1);
  EXPECT_THAT(DecodeFloatNumericRun(coded_mesh.other_attribute_components(0)),
              IsOkAndHolds(ElementsAre(-1, 7, 3)));

  // Decode the coded mesh, and ensure that the format includes the added
  // opacity shift and anti-aliasing attributes and that the custom attribute
  // data is still there.
  absl::StatusOr<Mesh> decoded = DecodeMesh(coded_mesh);
  ASSERT_THAT(decoded, IsOk());
  absl::StatusOr<MeshFormat> expected_format = MeshFormat::Create(
      {{AttrType::kFloat2PackedInOneFloat, AttrId::kPosition},
       {AttrType::kFloat1PackedInOneUnsignedByte, AttrId::kOpacityShift},
       {AttrType::kFloat1Unpacked, AttrId::kCustom0},
       {AttrType::kFloat2PackedInThreeUnsignedBytes_XY12,
        AttrId::kSideDerivative},
       {AttrType::kFloat1PackedInOneUnsignedByte, AttrId::kSideLabel},
       {AttrType::kFloat2PackedInThreeUnsignedBytes_XY12,
        AttrId::kForwardDerivative},
       {AttrType::kFloat1PackedInOneUnsignedByte, AttrId::kForwardLabel}},
      MeshFormat::IndexFormat::k32BitUnpacked16BitPacked);
  ASSERT_THAT(expected_format, IsOk());
  ASSERT_THAT(decoded->Format(), MeshFormatEq(*expected_format));
  ASSERT_EQ(decoded->VertexCount(), 3);
  EXPECT_EQ(decoded->FloatVertexAttribute(0, 2)[0], -1);
  EXPECT_EQ(decoded->FloatVertexAttribute(1, 2)[0], 7);
  EXPECT_EQ(decoded->FloatVertexAttribute(2, 2)[0], 3);
}

// Encoding and decoding fully packed meshes take slightly different code paths
// than unpacked or partially packed meshes.
TEST(MeshTest, EncodeAndDecodeFullyPackedMesh) {
  absl::StatusOr<MeshFormat> format = MeshFormat::Create(
      {{AttrType::kFloat2PackedInOneFloat, AttrId::kPosition},
       {AttrType::kFloat1PackedInOneUnsignedByte, AttrId::kOpacityShift},
       {AttrType::kFloat4PackedInOneFloat, AttrId::kColorShiftHcl},
       {AttrType::kFloat1PackedInOneUnsignedByte, AttrId::kCustom0},
       {AttrType::kFloat2PackedInThreeUnsignedBytes_XY12,
        AttrId::kSideDerivative},
       {AttrType::kFloat1PackedInOneUnsignedByte, AttrId::kSideLabel},
       {AttrType::kFloat2PackedInThreeUnsignedBytes_XY12,
        AttrId::kForwardDerivative},
       {AttrType::kFloat1PackedInOneUnsignedByte, AttrId::kForwardLabel}},
      MeshFormat::IndexFormat::k32BitUnpacked16BitPacked);
  ASSERT_THAT(format, IsOk());

  std::vector<float> position_x = {1, 3, 5};
  std::vector<float> position_y = {2, 4, 6};
  std::vector<float> color_h = {.1, .2, .3};
  std::vector<float> color_s = {.2, .4, .6};
  std::vector<float> color_l = {.4, .8, 1.2};
  std::vector<float> color_0 = {.8, 1.6, 2.4};
  std::vector<float> custom_attr = {-1, 7, 3};
  std::vector<float> zeros = {0, 0, 0};
  std::vector<uint32_t> triangles = {0, 1, 2};
  absl::StatusOr<Mesh> mesh = Mesh::Create(
      *format,
      {position_x, position_y, zeros, color_h, color_s, color_l, color_0,
       custom_attr, zeros, zeros, zeros, zeros, zeros, zeros},
      triangles);
  ASSERT_THAT(mesh, IsOk());
  CodedMesh coded_mesh;
  EncodeMesh(*mesh, coded_mesh);

  absl::StatusOr<Mesh> decoded = DecodeMesh(coded_mesh);
  ASSERT_THAT(decoded, IsOk());
  ASSERT_THAT(decoded->Format(), MeshFormatEq(*format));
  ASSERT_EQ(decoded->VertexCount(), 3);
  EXPECT_EQ(decoded->TriangleCount(), 1);

  EXPECT_EQ(decoded->VertexCount(), 3);
  EXPECT_EQ(decoded->VertexPosition(0), mesh->VertexPosition(0));
  EXPECT_EQ(decoded->VertexPosition(1), mesh->VertexPosition(1));
  EXPECT_EQ(decoded->VertexPosition(2), mesh->VertexPosition(2));

  EXPECT_EQ(decoded->TriangleCount(), 1);
  EXPECT_THAT(decoded->TriangleIndices(0), ElementsAre(0, 1, 2));

  EXPECT_EQ(decoded->FloatVertexAttribute(0, 2),
            mesh->FloatVertexAttribute(0, 2));
  EXPECT_EQ(decoded->FloatVertexAttribute(1, 2),
            mesh->FloatVertexAttribute(1, 2));
  EXPECT_EQ(decoded->FloatVertexAttribute(2, 2),
            mesh->FloatVertexAttribute(2, 2));
  EXPECT_EQ(decoded->FloatVertexAttribute(0, 3),
            mesh->FloatVertexAttribute(0, 3));
  EXPECT_EQ(decoded->FloatVertexAttribute(1, 3),
            mesh->FloatVertexAttribute(1, 3));
  EXPECT_EQ(decoded->FloatVertexAttribute(2, 3),
            mesh->FloatVertexAttribute(2, 3));
}

TEST(MeshTest, DecodeEmptyMesh) {
  CodedMesh coded_mesh;
  absl::StatusOr<Mesh> mesh = DecodeMesh(coded_mesh);
  ASSERT_THAT(mesh, IsOk());
  EXPECT_THAT(mesh->VertexCount(), 0);
  EXPECT_THAT(mesh->TriangleCount(), 0);
}

TEST(MeshTest, DecodeTriangleMesh) {
  CodedMesh coded_mesh;
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
      &coded_mesh));

  absl::StatusOr<Mesh> mesh = DecodeMesh(coded_mesh);
  ASSERT_THAT(mesh, IsOk());

  ASSERT_EQ(mesh->VertexCount(), 3);
  EXPECT_EQ(mesh->VertexPosition(0), (Point{2.5f, 2.5f}));
  EXPECT_EQ(mesh->VertexPosition(1), (Point{7.5f, 0.0f}));
  EXPECT_EQ(mesh->VertexPosition(2), (Point{10.0f, 2.5f}));

  ASSERT_EQ(mesh->TriangleCount(), 1);
  EXPECT_THAT(mesh->TriangleIndices(0), ElementsAre(0, 1, 2));
}

TEST(MeshTest, DecodeUnpackedMeshSkipsUnrecognizedAttributes) {
  CodedMesh coded_mesh;
  ASSERT_TRUE(TextFormat::ParseFromString(
      R"pb(
        format {
          attribute_types: [
            ATTR_TYPE_FLOAT2_UNPACKED,
            ATTR_TYPE_FLOAT2_UNPACKED,
            ATTR_TYPE_FLOAT1_UNPACKED,
            ATTR_TYPE_UNSPECIFIED
          ]
          attribute_ids: [
            ATTR_ID_POSITION,
            ATTR_ID_UNSPECIFIED,
            ATTR_ID_OPACITY_SHIFT,
            ATTR_ID_CUSTOM0
          ]
          attribute_component_counts: [ 2, 2, 1, 1 ]
        }
        triangle_index { deltas: [ 0, 1, 1 ] }
        x_stroke_space {
          scale: 2.5
          deltas: [ 1, 2, 1 ]
        }
        y_stroke_space {
          scale: 2.5
          deltas: [ 1, -1, 1 ]
        }
        other_attribute_components { deltas: [ 10, 10, 10 ] }
        other_attribute_components { deltas: [ 20, 20, 20 ] }
        other_attribute_components {
          offset: -0.5
          scale: 0.5
          deltas: [ 0, 1, 1 ]
        }
        other_attribute_components { deltas: [ 30, 30, 30 ] }
      )pb",
      &coded_mesh));

  absl::StatusOr<Mesh> decoded = DecodeMesh(coded_mesh);
  ASSERT_THAT(decoded, IsOk());

  absl::StatusOr<MeshFormat> expected_format = MeshFormat::Create(
      {{AttrType::kFloat2Unpacked, AttrId::kPosition},
       {AttrType::kFloat1Unpacked, AttrId::kOpacityShift},
       {AttrType::kFloat2PackedInThreeUnsignedBytes_XY12,
        AttrId::kSideDerivative},
       {AttrType::kFloat1PackedInOneUnsignedByte, AttrId::kSideLabel},
       {AttrType::kFloat2PackedInThreeUnsignedBytes_XY12,
        AttrId::kForwardDerivative},
       {AttrType::kFloat1PackedInOneUnsignedByte, AttrId::kForwardLabel}},
      MeshFormat::IndexFormat::k32BitUnpacked16BitPacked);
  ASSERT_THAT(expected_format, IsOk());
  EXPECT_THAT(decoded->Format(), MeshFormatEq(*expected_format));
  ASSERT_EQ(decoded->VertexCount(), 3u);
  EXPECT_THAT(decoded->FloatVertexAttribute(0, 1).Values(), ElementsAre(-0.5f));
  EXPECT_THAT(decoded->FloatVertexAttribute(1, 1).Values(), ElementsAre(0.0f));
  EXPECT_THAT(decoded->FloatVertexAttribute(2, 1).Values(), ElementsAre(0.5f));
}

TEST(MeshTest, DecodePackedMeshSkipsUnrecognizedAttributes) {
  CodedMesh coded_mesh;
  ASSERT_TRUE(TextFormat::ParseFromString(
      R"pb(
        format {
          attribute_types: [
            ATTR_TYPE_FLOAT2_PACKED_IN_ONE_FLOAT,
            ATTR_TYPE_FLOAT2_PACKED_IN_ONE_FLOAT,
            ATTR_TYPE_FLOAT1_PACKED_IN_ONE_BYTE,
            ATTR_TYPE_UNSPECIFIED,
            ATTR_TYPE_FLOAT2_PACKED_IN_THREE_BYTES_XY12,
            ATTR_TYPE_FLOAT1_PACKED_IN_ONE_BYTE,
            ATTR_TYPE_FLOAT2_PACKED_IN_THREE_BYTES_XY12,
            ATTR_TYPE_FLOAT1_PACKED_IN_ONE_BYTE
          ]
          attribute_ids: [
            ATTR_ID_POSITION,
            ATTR_ID_UNSPECIFIED,
            ATTR_ID_OPACITY_SHIFT,
            ATTR_ID_CUSTOM0,
            ATTR_ID_SIDE_DERIVATIVE,
            ATTR_ID_SIDE_LABEL,
            ATTR_ID_FORWARD_DERIVATIVE,
            ATTR_ID_FORWARD_LABEL
          ]
          attribute_component_counts: [ 2, 2, 1, 1, 2, 1, 2, 1 ]
        }
        triangle_index { deltas: [ 0, 1, 1 ] }
        x_stroke_space {
          scale: 2.5
          deltas: [ 1, 2, 1 ]
        }
        y_stroke_space {
          scale: 2.5
          deltas: [ 1, -1, 1 ]
        }
        other_attribute_components { deltas: [ 10, 10, 10 ] }
        other_attribute_components { deltas: [ 20, 20, 20 ] }
        other_attribute_components {
          offset: -0.5
          scale: 0.5
          deltas: [ 0, 1, 1 ]
        }
        other_attribute_components { deltas: [ 30, 30, 30 ] }
        other_attribute_components { deltas: [ 0, 0, 0 ] }
        other_attribute_components { deltas: [ 0, 0, 0 ] }
        other_attribute_components { deltas: [ 0, 0, 0 ] }
        other_attribute_components { deltas: [ 0, 0, 0 ] }
        other_attribute_components { deltas: [ 0, 0, 0 ] }
        other_attribute_components { deltas: [ 0, 0, 0 ] }
      )pb",
      &coded_mesh));

  absl::StatusOr<Mesh> decoded = DecodeMesh(coded_mesh);
  ASSERT_THAT(decoded, IsOk());

  absl::StatusOr<MeshFormat> expected_format = MeshFormat::Create(
      {{AttrType::kFloat2PackedInOneFloat, AttrId::kPosition},
       {AttrType::kFloat1PackedInOneUnsignedByte, AttrId::kOpacityShift},
       {AttrType::kFloat2PackedInThreeUnsignedBytes_XY12,
        AttrId::kSideDerivative},
       {AttrType::kFloat1PackedInOneUnsignedByte, AttrId::kSideLabel},
       {AttrType::kFloat2PackedInThreeUnsignedBytes_XY12,
        AttrId::kForwardDerivative},
       {AttrType::kFloat1PackedInOneUnsignedByte, AttrId::kForwardLabel}},
      MeshFormat::IndexFormat::k32BitUnpacked16BitPacked);
  ASSERT_THAT(expected_format, IsOk());
  EXPECT_THAT(decoded->Format(), MeshFormatEq(*expected_format));
  ASSERT_EQ(decoded->VertexCount(), 3u);
  EXPECT_THAT(decoded->FloatVertexAttribute(0, 1).Values(), ElementsAre(-0.5f));
  EXPECT_THAT(decoded->FloatVertexAttribute(1, 1).Values(), ElementsAre(0.0f));
  EXPECT_THAT(decoded->FloatVertexAttribute(2, 1).Values(), ElementsAre(0.5f));
}

TEST(MeshTest, DecodeMeshPopulatesMissingRequiredAttributes) {
  absl::StatusOr<MeshFormat> format = MeshFormat::Create(
      {{AttrType::kFloat2PackedInThreeUnsignedBytes_XY12, AttrId::kPosition}},
      MeshFormat::IndexFormat::k32BitUnpacked16BitPacked);
  ASSERT_THAT(format, IsOk());

  absl::StatusOr<Mesh> mesh =
      Mesh::Create(*format, {{0, 10, 0, 10}, {0, 0, 2, 2}}, {0, 1, 2, 2, 1, 3});
  ASSERT_THAT(mesh, IsOk());

  CodedMesh coded_mesh;
  EncodeMesh(*mesh, coded_mesh);

  absl::StatusOr<Mesh> decoded = DecodeMesh(coded_mesh);
  ASSERT_THAT(decoded, IsOk());

  absl::StatusOr<MeshFormat> expected_format = MeshFormat::Create(
      {{AttrType::kFloat2PackedInThreeUnsignedBytes_XY12, AttrId::kPosition},
       {AttrType::kFloat1PackedInOneUnsignedByte, AttrId::kOpacityShift},
       {AttrType::kFloat2PackedInThreeUnsignedBytes_XY12,
        AttrId::kSideDerivative},
       {AttrType::kFloat1PackedInOneUnsignedByte, AttrId::kSideLabel},
       {AttrType::kFloat2PackedInThreeUnsignedBytes_XY12,
        AttrId::kForwardDerivative},
       {AttrType::kFloat1PackedInOneUnsignedByte, AttrId::kForwardLabel}},
      MeshFormat::IndexFormat::k32BitUnpacked16BitPacked);
  ASSERT_THAT(expected_format, IsOk());
  EXPECT_THAT(decoded->Format(), MeshFormatEq(*expected_format));
  ASSERT_GE(decoded->VertexCount(), 4u);
  for (uint32_t v = 0; v < 4; ++v) {
    EXPECT_THAT(decoded->FloatVertexAttribute(v, 1).Values(),
                ElementsAre(FloatNear(0.0f, 1e-2f)));
    EXPECT_TRUE(decoded->FloatVertexAttribute(v, 3)[0] != 0.0f ||
                decoded->FloatVertexAttribute(v, 5)[0] != 0.0f);
  }
}

void DecodeMeshDoesNotCrashOnArbitraryInput(const CodedMesh& coded_mesh) {
  DecodeMesh(coded_mesh).IgnoreError();
}
FUZZ_TEST(MeshTest, DecodeMeshDoesNotCrashOnArbitraryInput);

void EncodePositionOnlyMeshRoundTrip(const MutableMesh& mutable_mesh) {
  // Create a packed mesh from the mutable_mesh.
  absl::StatusOr<absl::InlinedVector<Mesh, 1>> meshes = mutable_mesh.AsMeshes();
  ASSERT_THAT(meshes, IsOkAndHolds(SizeIs(1)));
  const Mesh& mesh = (*meshes)[0];

  // Encode the mesh.
  CodedMesh mesh_proto;
  EncodeMesh(mesh, mesh_proto);

  // The decoded mesh should equal the initial packed mesh.
  EXPECT_THAT(DecodeMesh(mesh_proto), IsOkAndHolds(MeshEq(mesh)));
}
FUZZ_TEST(MeshTest, EncodePositionOnlyMeshRoundTrip)
    .WithDomains(ValidPackableNonEmptyMutableMesh(*MeshFormat::Create(
        {{AttrType::kFloat2PackedInOneFloat, AttrId::kPosition},
         {AttrType::kFloat1PackedInOneUnsignedByte, AttrId::kOpacityShift},
         {AttrType::kFloat2PackedInThreeUnsignedBytes_XY12,
          AttrId::kSideDerivative},
         {AttrType::kFloat1PackedInOneUnsignedByte, AttrId::kSideLabel},
         {AttrType::kFloat2PackedInThreeUnsignedBytes_XY12,
          AttrId::kForwardDerivative},
         {AttrType::kFloat1PackedInOneUnsignedByte, AttrId::kForwardLabel}},
        MeshFormat::IndexFormat::k32BitUnpacked16BitPacked)));

}  // namespace
}  // namespace ink
