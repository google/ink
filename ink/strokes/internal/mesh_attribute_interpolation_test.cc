// Copyright 2026 Google LLC
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

#include "ink/strokes/internal/mesh_attribute_interpolation.h"

#include <array>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status_matchers.h"
#include "absl/status/statusor.h"
#include "ink/geometry/mesh_format.h"
#include "ink/geometry/mutable_mesh.h"
#include "ink/geometry/point.h"
#include "ink/types/small_array.h"

namespace ink::strokes_internal {
namespace {

using ::absl_testing::IsOk;
using ::testing::ElementsAre;
using ::testing::FloatNear;

using AttributeId = MeshFormat::AttributeId;
using AttributeType = MeshFormat::AttributeType;
using IndexFormat = MeshFormat::IndexFormat;

constexpr float kFloatTolerance = 1e-6f;

TEST(MeshAttributeInterpolationTest, InterpolatesLinearly) {
  absl::StatusOr<MeshFormat> format = MeshFormat::Create(
      {{AttributeType::kFloat2Unpacked, AttributeId::kPosition},
       {AttributeType::kFloat2Unpacked, AttributeId::kCustom0}},
      IndexFormat::k32BitUnpacked16BitPacked);
  ASSERT_THAT(format, IsOk());
  MutableMesh mesh(*format);
  mesh.AppendVertex({0, 0});
  mesh.AppendVertex({4, 0});
  mesh.SetFloatVertexAttribute(0, 1, {0.0f, 10.0f});
  mesh.SetFloatVertexAttribute(1, 1, {4.0f, 20.0f});

  mesh.AppendVertex({1, 0});
  SetInterpolatedAttributes(mesh, 2,
                            std::array{GetInterpolationAttributes(mesh, 0),
                                       GetInterpolationAttributes(mesh, 1)},
                            std::array{0.75f, 0.25f});

  SmallArray<float, 4> value = mesh.FloatVertexAttribute(2, 1);
  EXPECT_THAT(value.Values(), ElementsAre(FloatNear(1.0f, kFloatTolerance),
                                          FloatNear(12.5f, kFloatTolerance)));
}

TEST(MeshAttributeInterpolationTest, LeavesPositionAndLabelsUnchanged) {
  absl::StatusOr<MeshFormat> format = MeshFormat::Create(
      {{AttributeType::kFloat2Unpacked, AttributeId::kPosition},
       {AttributeType::kFloat1Unpacked, AttributeId::kSideLabel},
       {AttributeType::kFloat1Unpacked, AttributeId::kForwardLabel}},
      IndexFormat::k32BitUnpacked16BitPacked);
  ASSERT_THAT(format, IsOk());
  MutableMesh mesh(*format);
  mesh.AppendVertex({0, 0});
  mesh.SetFloatVertexAttribute(0, 1, {-127.0f});
  mesh.SetFloatVertexAttribute(0, 2, {127.0f});
  mesh.AppendVertex({1, 1});
  mesh.SetFloatVertexAttribute(1, 1, {50.0f});
  mesh.SetFloatVertexAttribute(1, 2, {-50.0f});

  SetInterpolatedAttributes(mesh, 1,
                            std::array{GetInterpolationAttributes(mesh, 0)},
                            std::array{1.0f});

  EXPECT_EQ(mesh.VertexPosition(1), (Point{1, 1}));
  SmallArray<float, 4> side = mesh.FloatVertexAttribute(1, 1);
  SmallArray<float, 4> forward = mesh.FloatVertexAttribute(1, 2);
  EXPECT_THAT(side.Values(), ElementsAre(50.0f));
  EXPECT_THAT(forward.Values(), ElementsAre(-50.0f));
}

TEST(MeshAttributeInterpolationTest, InterpolatesHclShiftInLinearSpace) {
  absl::StatusOr<MeshFormat> format = MeshFormat::Create(
      {{AttributeType::kFloat2Unpacked, AttributeId::kPosition},
       {AttributeType::kFloat3Unpacked, AttributeId::kColorShiftHcl}},
      IndexFormat::k32BitUnpacked16BitPacked);
  ASSERT_THAT(format, IsOk());
  MutableMesh mesh(*format);
  mesh.AppendVertex({0, 10});
  mesh.AppendVertex({10, 10});
  mesh.AppendVertex({5, 2});
  mesh.SetFloatVertexAttribute(0, 1, {0.9f, 0.5f, 0.4f});
  mesh.SetFloatVertexAttribute(1, 1, {0.8f, 0.5f, 0.8f});
  mesh.SetFloatVertexAttribute(2, 1, {0.1f, 0.4f, 0.2f});

  mesh.AppendVertex({5, 8});
  SetInterpolatedAttributes(mesh, 3,
                            std::array{GetInterpolationAttributes(mesh, 0),
                                       GetInterpolationAttributes(mesh, 1),
                                       GetInterpolationAttributes(mesh, 2)},
                            std::array{3.0f / 8, 3.0f / 8, 2.0f / 8});

  SmallArray<float, 4> hcl_shift = mesh.FloatVertexAttribute(3, 1);
  EXPECT_THAT(hcl_shift.Values(),
              ElementsAre(
                  // Hue and chroma shift are linearly interpolated after
                  // mapping from polar coordinates to cartesian coordinates.
                  FloatNear(-0.099683f, kFloatTolerance),
                  FloatNear(0.125730f, kFloatTolerance),
                  // Lightness shift interpolates linearly.
                  FloatNear(0.5f, kFloatTolerance)));
}

}  // namespace
}  // namespace ink::strokes_internal
