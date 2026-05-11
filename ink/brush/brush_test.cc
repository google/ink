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

#include "ink/brush/brush.h"

#include <cmath>
#include <limits>
#include <string>
#include <utility>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/status/status_matchers.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "ink/brush/brush_behavior.h"
#include "ink/brush/brush_family.h"
#include "ink/brush/brush_paint.h"
#include "ink/brush/brush_tip.h"
#include "ink/brush/easing_function.h"
#include "ink/brush/type_matchers.h"
#include "ink/color/color.h"
#include "ink/geometry/angle.h"

namespace ink {
namespace {

using ::absl_testing::IsOk;
using ::absl_testing::StatusIs;
using ::testing::Eq;
using ::testing::HasSubstr;
using ::testing::Pointwise;

constexpr absl::string_view kTestTextureId = "test-texture";

BrushFamily CreateTestFamily() {
  absl::StatusOr<BrushFamily> family = BrushFamily::Create(
      {
          .scale = {0.5, 1},
          .corner_rounding = 0.3,
          .rotation = kFullTurn / 8,
          .behaviors = {BrushBehavior{{
              BrushBehavior::SourceNode{
                  .source = BrushBehavior::Source::kNormalizedPressure,
                  .source_out_of_range_behavior =
                      BrushBehavior::OutOfRange::kMirror,
                  .source_value_range = {0.2, 0.4},
              },
              BrushBehavior::ResponseNode{
                  .response_curve = {EasingFunction::Predefined::kEaseInOut},
              },
              BrushBehavior::DampingNode{
                  .damping_source =
                      BrushBehavior::ProgressDomain::kTimeInSeconds,
                  .damping_gap = 0.25,
              },
              BrushBehavior::TargetNode{
                  .target = BrushBehavior::Target::kWidthMultiplier,
                  .target_modifier_range = {0.7, 1.25},
              },
          }}},
      },
      {.texture_layers = {{.client_texture_id = std::string(kTestTextureId),
                           .mapping = BrushPaint::TextureMapping::kStamping,
                           .size_unit = BrushPaint::TextureSizeUnit::kBrushSize,
                           .size = {3, 5},
                           .blend_mode = BrushPaint::BlendMode::kDstIn}}},
      BrushFamily::DefaultInputModel(),
      {.client_brush_family_id = "/brush-family:test-family"});
  ABSL_CHECK_OK(family);
  return *family;
}

TEST(BrushTest, Stringify) {
  absl::StatusOr<BrushFamily> family = BrushFamily::Create(
      BrushTip{.scale = {3, 3}, .corner_rounding = 0},
      {.texture_layers = {{.client_texture_id = std::string(kTestTextureId),
                           .mapping = BrushPaint::TextureMapping::kStamping,
                           .size_unit = BrushPaint::TextureSizeUnit::kBrushSize,
                           .size = {3, 5},
                           .blend_mode = BrushPaint::BlendMode::kDstOut}}},
      BrushFamily::PassthroughModel{},
      {.client_brush_family_id = "big-square"});
  ASSERT_THAT(family, IsOk());
  absl::StatusOr<Brush> brush = Brush::Create(*family, Color::Blue(), 3, .1);
  ASSERT_THAT(brush, IsOk());
  EXPECT_EQ(
      absl::StrCat(*brush),
      "Brush(color=Color({0.000000, 0.000000, 1.000000, 1.000000}, sRGB), "
      "size=3, epsilon=0.1, "
      "family=BrushFamily(coats=[BrushCoat{tip=BrushTip{scale=<3, 3>, "
      "corner_rounding=0}, "
      "paint_preferences={BrushPaint{texture_layers={TextureLayer{client_"
      "texture_id=test-texture, mapping=kStamping, "
      "origin=kStrokeSpaceOrigin, size_unit=kBrushSize, wrap_x=kRepeat, "
      "wrap_y=kRepeat, size=<3, 5>, offset=<0, 0>, rotation=0π, "
      "animation_frames=1, animation_rows=1, animation_columns=1, "
      "animation_duration=1s, blend_mode=kDstOut}}, "
      "self_overlap=kAny}}}], input_model=PassthroughModel, "
      "client_brush_family_id='big-square'))");
}

TEST(BrushTest, Create) {
  BrushFamily family = CreateTestFamily();
  absl::StatusOr<Brush> brush = Brush::Create(family, Color::Blue(), 3, .1);
  ASSERT_THAT(brush, IsOk());

  EXPECT_THAT(brush->GetFamily(), BrushFamilyEq(family));
  EXPECT_THAT(brush->GetCoats(), Pointwise(BrushCoatEq(), family.GetCoats()));
  EXPECT_THAT(brush->GetColor(), Eq(Color::Blue()));
  EXPECT_EQ(brush->GetSize(), 3);
  EXPECT_EQ(brush->GetEpsilon(), 0.1f);
}

TEST(BrushTest, CreateWithInvalidArguments) {
  EXPECT_THAT(
      Brush::Create(BrushFamily(), Color::Blue(), -10, .1),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("`size`")));
  EXPECT_THAT(
      Brush::Create(BrushFamily(), Color::Blue(),
                    std::numeric_limits<float>::infinity(), .1),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("`size`")));
  EXPECT_THAT(
      Brush::Create(BrushFamily(), Color::Blue(), std::nanf(""), .1),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("`size`")));

  EXPECT_THAT(
      Brush::Create(BrushFamily(), Color::Blue(), 3, -2),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("`epsilon`")));
  EXPECT_THAT(
      Brush::Create(BrushFamily(), Color::Blue(), 3,
                    std::numeric_limits<float>::infinity()),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("`epsilon`")));
  EXPECT_THAT(
      Brush::Create(BrushFamily(), Color::Blue(), 3, std::nanf("")),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("`epsilon`")));

  EXPECT_THAT(Brush::Create(BrushFamily(), Color::Blue(), 1, 10),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("greater than or equal")));
}

TEST(BrushTest, CopyAndMove) {
  BrushFamily family = CreateTestFamily();

  {
    absl::StatusOr<Brush> brush = Brush::Create(family, Color::Blue(), 3, .1);
    ASSERT_THAT(brush, IsOk());

    Brush copied_brush = *brush;
    EXPECT_THAT(copied_brush.GetFamily(), BrushFamilyEq(brush->GetFamily()));
    EXPECT_THAT(copied_brush.GetColor(), Eq(brush->GetColor()));
    EXPECT_EQ(copied_brush.GetSize(), brush->GetSize());
    EXPECT_EQ(copied_brush.GetEpsilon(), brush->GetEpsilon());
  }
  {
    absl::StatusOr<Brush> brush = Brush::Create(family, Color::Blue(), 3, .1);
    ASSERT_THAT(brush, IsOk());

    Brush copied_brush;
    copied_brush = *brush;
    EXPECT_THAT(copied_brush.GetFamily(), BrushFamilyEq(brush->GetFamily()));
    EXPECT_THAT(copied_brush.GetColor(), Eq(brush->GetColor()));
    EXPECT_EQ(copied_brush.GetSize(), brush->GetSize());
    EXPECT_EQ(copied_brush.GetEpsilon(), brush->GetEpsilon());
  }
  {
    absl::StatusOr<Brush> brush = Brush::Create(family, Color::Blue(), 3, .1);
    ASSERT_THAT(brush, IsOk());

    Brush copied_brush = *brush;
    Brush moved_brush = *std::move(brush);
    EXPECT_THAT(moved_brush.GetFamily(),
                BrushFamilyEq(copied_brush.GetFamily()));
    EXPECT_THAT(moved_brush.GetColor(), Eq(copied_brush.GetColor()));
    EXPECT_EQ(moved_brush.GetSize(), copied_brush.GetSize());
    EXPECT_EQ(moved_brush.GetEpsilon(), copied_brush.GetEpsilon());
  }
  {
    absl::StatusOr<Brush> brush = Brush::Create(family, Color::Blue(), 3, .1);
    ASSERT_THAT(brush, IsOk());

    Brush copied_brush = *brush;
    Brush moved_brush;
    moved_brush = *std::move(brush);
    EXPECT_THAT(moved_brush.GetFamily(),
                BrushFamilyEq(copied_brush.GetFamily()));
    EXPECT_THAT(moved_brush.GetColor(), Eq(copied_brush.GetColor()));
    EXPECT_EQ(moved_brush.GetSize(), copied_brush.GetSize());
    EXPECT_EQ(moved_brush.GetEpsilon(), copied_brush.GetEpsilon());
  }
}

TEST(BrushTest, SetNewFamily) {
  BrushFamily start_family = CreateTestFamily();
  auto brush = Brush::Create(start_family, Color::Magenta(), 10, 3);
  ASSERT_THAT(brush, IsOk());

  auto new_family = BrushFamily::Create(
      BrushTip{},
      {.texture_layers = {{.client_texture_id = std::string(kTestTextureId),
                           .mapping = BrushPaint::TextureMapping::kStamping,
                           .size_unit = BrushPaint::TextureSizeUnit::kBrushSize,
                           .size = {3, 5},
                           .blend_mode = BrushPaint::BlendMode::kDstIn}}},
      BrushFamily::DefaultInputModel(),
      {.client_brush_family_id = "/brush-family:new-test-family"});
  ASSERT_THAT(new_family, IsOk());

  EXPECT_THAT(brush->GetFamily(), ::testing::Not(BrushFamilyEq(*new_family)));

  brush->SetFamily(*new_family);
  EXPECT_THAT(brush->GetFamily(), BrushFamilyEq(*new_family));
}

TEST(BrushTest, SetNewColor) {
  Color start_color = Color::Blue();
  auto brush = Brush::Create(CreateTestFamily(), start_color, 10, 3);
  ASSERT_THAT(brush, IsOk());
  ASSERT_THAT(brush->GetColor(), start_color);

  Color new_color = Color::Red();
  brush->SetColor(new_color);
  EXPECT_EQ(brush->GetColor(), new_color);
}

TEST(BrushTest, SetNewSize) {
  float start_size = 5;
  auto brush = Brush::Create(CreateTestFamily(), Color::Blue(), start_size, 3);
  ASSERT_THAT(brush, IsOk());
  ASSERT_THAT(brush->GetSize(), start_size);

  float new_size = 10;
  EXPECT_THAT(brush->SetSize(new_size), IsOk());
  EXPECT_EQ(brush->GetSize(), new_size);
}

TEST(BrushTest, SetInvalidSize) {
  BrushFamily family = CreateTestFamily();
  auto brush = Brush::Create(family, Color::Green(), 30, 1);
  ASSERT_THAT(brush, IsOk());
  Brush brush_before_invalid_arg = *brush;
  EXPECT_THAT(brush->SetSize(-3), StatusIs(absl::StatusCode::kInvalidArgument,
                                           HasSubstr("`size`")));
  EXPECT_THAT(*brush, BrushEq(brush_before_invalid_arg));

  EXPECT_THAT(
      brush->SetSize(std::numeric_limits<float>::infinity()),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("`size`")));
  EXPECT_THAT(*brush, BrushEq(brush_before_invalid_arg));

  EXPECT_THAT(
      brush->SetSize(std::nanf("")),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("`size`")));
  EXPECT_THAT(*brush, BrushEq(brush_before_invalid_arg));

  EXPECT_THAT(brush->SetSize(0.1),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("greater than or equal")));
  EXPECT_THAT(*brush, BrushEq(brush_before_invalid_arg));
}

TEST(BrushTest, SetNewEpsilon) {
  float start_epsilon = 5;
  auto brush = Brush::Create(BrushFamily{}, Color::Blue(), 10, start_epsilon);
  ASSERT_THAT(brush, IsOk());
  ASSERT_THAT(brush->GetEpsilon(), start_epsilon);

  float new_epsilon = 1;
  EXPECT_THAT(brush->SetEpsilon(new_epsilon), IsOk());
  EXPECT_EQ(brush->GetEpsilon(), new_epsilon);
}

TEST(BrushTest, SetInvalidEpsilon) {
  auto brush = Brush::Create(CreateTestFamily(), Color::Green(), 30, 1);
  ASSERT_THAT(brush, IsOk());
  Brush brush_before_invalid_arg = *brush;

  EXPECT_THAT(
      brush->SetEpsilon(-3),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("`epsilon`")));
  EXPECT_THAT(*brush, BrushEq(brush_before_invalid_arg));

  EXPECT_THAT(
      brush->SetEpsilon(std::numeric_limits<float>::infinity()),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("`epsilon`")));
  EXPECT_THAT(*brush, BrushEq(brush_before_invalid_arg));

  EXPECT_THAT(
      brush->SetEpsilon(std::nanf("")),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("`epsilon`")));
  EXPECT_THAT(*brush, BrushEq(brush_before_invalid_arg));

  EXPECT_THAT(brush->SetEpsilon(31),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("greater than or equal")));
  EXPECT_THAT(*brush, BrushEq(brush_before_invalid_arg));
}

}  // namespace
}  // namespace ink
