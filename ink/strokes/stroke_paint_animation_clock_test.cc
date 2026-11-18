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

#include "ink/strokes/stroke_paint_animation_clock.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status_matchers.h"
#include "absl/status/statusor.h"
#include "absl/time/time.h"
#include "ink/brush/brush_coat.h"
#include "ink/brush/brush_family.h"
#include "ink/brush/brush_paint.h"
#include "ink/brush/brush_tip.h"
#include "ink/types/duration.h"

namespace ink {
namespace {

using ::absl_testing::IsOk;

TEST(StrokePaintAnimationClockTest,
     CalculateBasePaintAnimationPhaseForNewStrokeNonAnimated) {
  EXPECT_EQ(CalculateBasePaintAnimationPhaseForNewStroke(
                /*clock_state_millis=*/12345,
                /*brush_family_loop_duration_millis=*/0),
            0.0f);
  EXPECT_EQ(CalculateBasePaintAnimationPhaseForNewStroke(
                /*clock_state=*/absl::Seconds(12.345),
                /*brush_family_loop_duration=*/absl::ZeroDuration()),
            0.0f);
}

TEST(StrokePaintAnimationClockTest,
     CalculateBasePaintAnimationPhaseForNewStrokeExpectedValues) {
  EXPECT_EQ(CalculateBasePaintAnimationPhaseForNewStroke(
                /*clock_state_millis=*/1250,
                /*brush_family_loop_duration_millis=*/1000),
            0.75f);
  EXPECT_EQ(CalculateBasePaintAnimationPhaseForNewStroke(
                /*clock_state_millis=*/1000,
                /*brush_family_loop_duration_millis=*/1000),
            0.0f);
  EXPECT_EQ(CalculateBasePaintAnimationPhaseForNewStroke(
                /*clock_state_millis=*/2000,
                /*brush_family_loop_duration_millis=*/1000),
            0.0f);
  EXPECT_EQ(CalculateBasePaintAnimationPhaseForNewStroke(
                /*clock_state_millis=*/2500,
                /*brush_family_loop_duration_millis=*/1000),
            0.5f);
  EXPECT_EQ(CalculateBasePaintAnimationPhaseForNewStroke(
                /*clock_state=*/absl::Seconds(1.25),
                /*brush_family_loop_duration=*/absl::Seconds(1)),
            0.75f);
}

TEST(StrokePaintAnimationClockTest,
     CalculateCurrentAnimationPhaseForBrushPaintNonAnimated) {
  EXPECT_EQ(CalculateCurrentAnimationPhaseForBrushPaint(
                /*clock_state_millis=*/12345,
                /*brush_family_loop_duration_millis=*/1000,
                /*brush_paint_loop_duration_millis=*/0,
                /*stroke_base_paint_animation_phase=*/0.25f),
            0.0f);
}

TEST(StrokePaintAnimationClockTest,
     CalculateCurrentAnimationPhaseForBrushPaintExpectedValues) {
  EXPECT_EQ(CalculateCurrentAnimationPhaseForBrushPaint(
                /*clock_state_millis=*/1500,
                /*brush_family_loop_duration_millis=*/1000,
                /*brush_paint_loop_duration_millis=*/500,
                /*stroke_base_paint_animation_phase=*/0.25f),
            0.5f);
  EXPECT_EQ(CalculateCurrentAnimationPhaseForBrushPaint(
                /*clock_state_millis=*/1500,
                /*brush_family_loop_duration_millis=*/1000,
                /*brush_paint_loop_duration_millis=*/500,
                /*stroke_base_paint_animation_phase=*/0.5f),
            0.0f);
  EXPECT_EQ(CalculateCurrentAnimationPhaseForBrushPaint(
                /*clock_state_millis=*/-125,
                /*brush_family_loop_duration_millis=*/1000,
                /*brush_paint_loop_duration_millis=*/500,
                /*stroke_base_paint_animation_phase=*/0.75f),
            0.25f);

  BrushPaint paint_500ms{
      .texture_layers = {BrushPaint::StampingTexture{
          .client_texture_id = "test-texture-1",
          .animation_frames = 8,
          .animation_rows = 4,
          .animation_columns = 2,
          .animation_duration = absl::Milliseconds(500),
          .animation_repeat_mode = BrushPaint::AnimationRepeatMode::kRestart,
      }},
  };
  BrushPaint paint_1000ms{
      .texture_layers = {BrushPaint::StampingTexture{
          .client_texture_id = "test-texture-2",
          .animation_frames = 8,
          .animation_rows = 4,
          .animation_columns = 2,
          .animation_duration = absl::Milliseconds(1000),
          .animation_repeat_mode = BrushPaint::AnimationRepeatMode::kRestart,
      }},
  };
  BrushTip particle_tip{.particle_gap_duration = Duration32::Seconds(0.05f)};
  absl::StatusOr<BrushFamily> family = BrushFamily::Create(
      {BrushCoat{.tip = particle_tip, .paint_preferences = {paint_500ms}},
       BrushCoat{.tip = particle_tip, .paint_preferences = {paint_1000ms}}});
  ASSERT_THAT(family, IsOk());
  EXPECT_EQ(CalculateCurrentAnimationPhaseForBrushPaint(
                /*clock_state=*/absl::Seconds(1.5), *family, paint_500ms,
                /*stroke_base_paint_animation_phase=*/0.25f),
            0.5f);
}

}  // namespace
}  // namespace ink
