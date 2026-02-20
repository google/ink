// Copyright 2025 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "ink/brush/stock_brushes.h"

#include <string>
#include <vector>

#include "absl/log/absl_check.h"
#include "absl/status/statusor.h"
#include "ink/brush/brush_behavior.h"
#include "ink/brush/brush_coat.h"
#include "ink/brush/brush_family.h"
#include "ink/brush/brush_paint.h"
#include "ink/brush/brush_tip.h"
#include "ink/brush/easing_function.h"
#include "ink/geometry/angle.h"

namespace ink::stock_brushes {

using BinaryOp = ::ink::BrushBehavior::BinaryOp;
using BinaryOpNode = ::ink::BrushBehavior::BinaryOpNode;
using ConstantNode = ::ink::BrushBehavior::ConstantNode;
using DampingNode = ::ink::BrushBehavior::DampingNode;
using Metadata = ::ink::BrushFamily::Metadata;
using OutOfRange = ::ink::BrushBehavior::OutOfRange;
using ProgressDomain = ::ink::BrushBehavior::ProgressDomain;
using ResponseNode = ::ink::BrushBehavior::ResponseNode;
using Source = ::ink::BrushBehavior::Source;
using SourceNode = ::ink::BrushBehavior::SourceNode;
using Target = ::ink::BrushBehavior::Target;
using TargetNode = ::ink::BrushBehavior::TargetNode;
using ToolTypeFilterNode = ::ink::BrushBehavior::ToolTypeFilterNode;

BrushBehavior PredictionFadeOutBehavior() {
  return BrushBehavior{
      .nodes =
          {SourceNode{.source = Source::kPredictedTimeElapsedInSeconds,
                      .source_value_range = {0.0f, 0.024f}},
           // The second branch of the binary op node keeps the opacity
           // fade-out from starting until the predicted inputs have traveled
           // at least 1.5x brush-size.
           SourceNode{
               .source =
                   Source::kPredictedDistanceTraveledInMultiplesOfBrushSize,
               .source_value_range = {1.5f, 2.0f}},
           ResponseNode{
               .response_curve = {EasingFunction::Predefined::kEaseInOut}},
           BinaryOpNode{.operation = BinaryOp::kProduct},
           TargetNode{.target = Target::kOpacityMultiplier,
                      .target_modifier_range = {1.0f, 0.3f}}},
      .developer_comment =
          "Fades out the predicted portion of the stroke, to lessen the visual "
          "impact of a potentially-inaccurate prediction. The fade-out is "
          "based on how far into the future the prediction is (the farther "
          "into the future, the less confident it is); however, the second "
          "branch of the binary op node prevents the fade-out from starting "
          "until and unless the predicted inputs have traveled at least a "
          "certain distance, to prevent a jarringly rapid fade-out for a "
          "short-distance prediction."};
}

constexpr BrushFamily::InputModel StockInputModel() {
  return BrushFamily::SlidingWindowModel();
}

BrushFamily Marker(const MarkerVersion& version) {
  switch (version) {
    case MarkerVersion::kV1: {
      absl::StatusOr<BrushFamily> marker_v1 = BrushFamily::Create(
          BrushTip{.behaviors = {PredictionFadeOutBehavior()}}, BrushPaint{},
          StockInputModel(),
          Metadata{
              .developer_comment =
                  "A felt-tip marker, with a circular tip shape, and no "
                  "dynamic behaviors other than prediction fade-out. This "
                  "serves well as an all-purpose basic brush for drawing "
                  "or handwriting.",
          });
      ABSL_CHECK_OK(marker_v1);
      return *marker_v1;
    }
  }
  ABSL_CHECK(false) << "Unsupported marker version: "
                    << static_cast<int>(version);
}

BrushFamily PressurePen(const PressurePenVersion& version) {
  switch (version) {
    case PressurePenVersion::kV1: {
      BrushBehavior taper_stroke_end_behavior = {
          .nodes = {SourceNode{
                        .source =
                            Source::kDistanceRemainingInMultiplesOfBrushSize,
                        .source_out_of_range_behavior = OutOfRange::kClamp,
                        .source_value_range = {3.0f, 0.0f},
                    },
                    TargetNode{.target = Target::kSizeMultiplier,
                               .target_modifier_range = {1.0f, 0.75f}}},
          .developer_comment =
              "Slightly reduces the brush size near the end of the stroke, "
              "creating a small taper."};
      BrushBehavior normalized_direction_y_to_size_behavior = {
          .nodes = {SourceNode{
                        .source = Source::kNormalizedDirectionY,
                        .source_out_of_range_behavior = OutOfRange::kClamp,
                        .source_value_range = {0.45f, 0.65f},
                    },
                    DampingNode{
                        .damping_source = ProgressDomain::kTimeInSeconds,
                        .damping_gap = .025f,
                    },
                    TargetNode{
                        .target = Target::kSizeMultiplier,
                        .target_modifier_range = {1.0f, 1.17f},
                    }},
          .developer_comment =
              "Slightly increases the brush size when the input is moving "
              "mostly downwards (rather than sideways or upwards)."};
      BrushBehavior acceleration_damped_to_size_behavior = {
          .nodes =
              {SourceNode{
                   .source = Source::
                       kInputAccelerationLateralInCentimetersPerSecondSquared,
                   .source_out_of_range_behavior = OutOfRange::kClamp,
                   .source_value_range = {-80.0f, -230.0f},
               },
               DampingNode{
                   .damping_source = ProgressDomain::kTimeInSeconds,
                   .damping_gap = .025f,
               },
               TargetNode{
                   .target = Target::kSizeMultiplier,
                   .target_modifier_range = {1.0f, 1.25f},
               }},
          .developer_comment =
              "Slightly increases the brush size for negative lateral "
              "acceleration. This tends to make the stroke thicker for "
              "quickly-drawn counterclockwise loops."};
      BrushBehavior stylus_pressure_to_size_behavior = {
          .nodes = {SourceNode{
                        .source = Source::kNormalizedPressure,
                        .source_out_of_range_behavior = OutOfRange::kClamp,
                        .source_value_range = {0.8f, 1.0f},
                    },
                    DampingNode{
                        .damping_source = ProgressDomain::kTimeInSeconds,
                        .damping_gap = .03f,
                    },
                    ToolTypeFilterNode{
                        .enabled_tool_types = {.stylus = true},
                    },
                    TargetNode{
                        .target = Target::kSizeMultiplier,
                        .target_modifier_range = {1.0f, 1.5f},
                    }},
          .developer_comment =
              "Increases the brush size for high stylus pressure values. This "
              "behavior is disabled for non-stylus input types, in particular "
              "for touch inputs, because touch pressure readings tend to be "
              "inaccurate in a way unsuitable for a handwriting-focused "
              "brush."};
      BrushTip tip = {.behaviors = {PredictionFadeOutBehavior(),
                                    taper_stroke_end_behavior,
                                    normalized_direction_y_to_size_behavior,
                                    acceleration_damped_to_size_behavior,
                                    stylus_pressure_to_size_behavior}};
      absl::StatusOr<BrushFamily> pressure_pen_v1 = BrushFamily::Create(
          tip, BrushPaint{}, StockInputModel(),
          Metadata{
              .developer_comment =
                  "A pressure-sensitive pen optimized for handwriting. "
                  "Pressing down harder with the stylus produces a wider "
                  "stroke. The stroke size is also subtly affected by "
                  "acceleration and travel direction.",
          });
      ABSL_CHECK_OK(pressure_pen_v1);
      return *pressure_pen_v1;
    }
  }
  ABSL_CHECK(false) << "Unsupported pressure pen version: "
                    << static_cast<int>(version);
}

BrushFamily Highlighter(const BrushPaint::SelfOverlap& self_overlap,
                        const HighlighterVersion& version) {
  switch (version) {
    case HighlighterVersion::kV1: {
      BrushBehavior increase_opacity_near_stroke_start_behavior = {
          .nodes = {SourceNode{
                        .source =
                            Source::kDistanceTraveledInMultiplesOfBrushSize,
                        .source_out_of_range_behavior = OutOfRange::kClamp,
                        .source_value_range = {0.0f, 3.0f},
                    },
                    DampingNode{
                        .damping_source = ProgressDomain::kTimeInSeconds,
                        .damping_gap = 0.015f,
                    },
                    TargetNode{
                        .target = Target::kOpacityMultiplier,
                        .target_modifier_range = {1.1f, 1.0f},
                    }},
          .developer_comment =
              "Subtly increases the opacity of the highlighter stroke near the "
              "start of the stroke."};
      BrushBehavior increase_opacity_near_stroke_end_behavior = {
          .nodes = {SourceNode{
                        .source =
                            Source::kDistanceRemainingInMultiplesOfBrushSize,
                        .source_out_of_range_behavior = OutOfRange::kClamp,
                        .source_value_range = {0.0f, 3.0f},
                    },
                    DampingNode{
                        .damping_source = ProgressDomain::kTimeInSeconds,
                        .damping_gap = 0.015f,
                    },
                    TargetNode{
                        .target = Target::kOpacityMultiplier,
                        .target_modifier_range = {1.1f, 1.0f},
                    }},
          .developer_comment =
              "Subtly increases the opacity of the highlighter stroke near the "
              "end of the stroke."};
      BrushTip tip = {.scale = {0.25f, 1.0f},
                      .corner_rounding = 1.0f,
                      .rotation = Angle::Degrees(150.0f),
                      .behaviors = {
                          PredictionFadeOutBehavior(),
                          increase_opacity_near_stroke_start_behavior,
                          increase_opacity_near_stroke_end_behavior,
                      }};
      absl::StatusOr<BrushFamily> highlighter_v1 = BrushFamily::Create(
          tip, BrushPaint{.self_overlap = self_overlap}, StockInputModel(),
          Metadata{
              .developer_comment =
                  "A basic highlighter brush, suitable for highlighting text "
                  "in a document. Best used with low-opacity brush colors."});
      ABSL_CHECK_OK(highlighter_v1);
      return *highlighter_v1;
    }
  }
  ABSL_CHECK(false) << "Unsupported highlighter version: "
                    << static_cast<int>(version);
}

BrushFamily DashedLine(const DashedLineVersion& version) {
  switch (version) {
    case DashedLineVersion::kV1: {
      BrushBehavior rotate_particles_to_match_stroke_direction = {
          .nodes = {SourceNode{
                        .source = Source::kDirectionAboutZeroInRadians,
                        .source_out_of_range_behavior = OutOfRange::kClamp,
                        .source_value_range = {-kHalfTurn.ValueInRadians(),
                                               kHalfTurn.ValueInRadians()},
                    },
                    TargetNode{
                        .target = Target::kRotationOffsetInRadians,
                        .target_modifier_range = {-kHalfTurn.ValueInRadians(),
                                                  kHalfTurn.ValueInRadians()},
                    }},
          .developer_comment =
              "Rotates the brush tip (in this case, the particle shape) to "
              "align with the stroke's direction of travel."};
      BrushTip tip = {
          .scale = {2.0f, 1.0f},
          .corner_rounding = 0.45f,
          .particle_gap_distance_scale = 3.0f,
          .behaviors = {PredictionFadeOutBehavior(),
                        rotate_particles_to_match_stroke_direction}};
      absl::StatusOr<BrushFamily> dashed_line_v1 = BrushFamily::Create(
          tip, BrushPaint{}, StockInputModel(),
          Metadata{.developer_comment =
                       "A brush that automatically draws a dashed line, "
                       "suitable for annotations or for a selection tool. This "
                       "version uses (rounded) rectangular particles for the "
                       "dashes, to ensure uniformity."});
      ABSL_CHECK_OK(dashed_line_v1);
      return *dashed_line_v1;
    }
  }
  ABSL_CHECK(false) << "Unsupported dashed line version: "
                    << static_cast<int>(version);
}

BrushCoat MiniEmojiCoat(
    std::string client_texture_id, float tip_scale, float tip_rotation_degrees,
    float tip_particle_gap_distance_scale, float position_offset_range_start,
    float position_offset_range_end, float distance_traveled_range_start,
    float distance_traveled_range_end, float luminosity_range_start,
    float luminosity_range_end) {
  BrushBehavior time_since_input_to_size_behavior = {
      .nodes = {SourceNode{
                    .source = Source::kTimeSinceInputInSeconds,
                    .source_out_of_range_behavior = OutOfRange::kClamp,
                    .source_value_range = {0.0f, 0.7f},
                },
                TargetNode{
                    .target = Target::kSizeMultiplier,
                    .target_modifier_range = {1.0f, 0.0f},
                }},
      .developer_comment =
          "Animates each mini emoji particle to scale down over time, until it "
          "completely disappears."};
  BrushBehavior constant_hue_and_luminosity_offset_behavior = {
      .nodes = {ConstantNode{.value = 0.0f},
                TargetNode{
                    .target = Target::kHueOffsetInRadians,
                    .target_modifier_range =
                        {Angle::Degrees(59.0f).ValueInRadians(),
                         Angle::Degrees(60.0f).ValueInRadians()},
                },
                ConstantNode{.value = 0.0f},
                TargetNode{
                    .target = Target::kLuminosity,
                    .target_modifier_range = {luminosity_range_start,
                                              luminosity_range_end},
                }},
      .developer_comment =
          "Applies a constant hue and luminosity offset to the mini emoji "
          "particles, to help differentiate them visually from the main emoji "
          "stamp."};
  BrushBehavior distance_traveled_to_offset_y_behavior = {
      .nodes = {SourceNode{
                    .source = Source::kDistanceTraveledInMultiplesOfBrushSize,
                    .source_out_of_range_behavior = OutOfRange::kRepeat,
                    .source_value_range = {distance_traveled_range_start,
                                           distance_traveled_range_end},
                },
                TargetNode{
                    .target = Target::kPositionOffsetYInMultiplesOfBrushSize,
                    .target_modifier_range = {position_offset_range_start,
                                              position_offset_range_end},
                }},
      .developer_comment =
          "Applies differing offsets to the vertical positions of each mini "
          "emoji particle along the length of the stroke. This helps to "
          "scatter the particles and make their positions look more random."};
  BrushTip tip = {
      .scale = {tip_scale, tip_scale},
      .corner_rounding = 0.0f,
      .rotation = Angle::Degrees(tip_rotation_degrees),
      .particle_gap_distance_scale = tip_particle_gap_distance_scale,
      .behaviors = {time_since_input_to_size_behavior,
                    constant_hue_and_luminosity_offset_behavior,
                    distance_traveled_to_offset_y_behavior},
  };
  BrushPaint paint = {
      .texture_layers = {{
          .client_texture_id = client_texture_id,
          .mapping = BrushPaint::TextureMapping::kStamping,
          .blend_mode = BrushPaint::BlendMode::kModulate,
      }},
  };
  return BrushCoat{.tip = tip, .paint_preferences = {paint}};
}

BrushFamily EmojiHighlighter(std::string client_texture_id,
                             bool show_mini_emoji_trail,
                             const BrushPaint::SelfOverlap& self_overlap,
                             const EmojiHighlighterVersion& version) {
  switch (version) {
    case EmojiHighlighterVersion::kV1: {
      // Highlighter coat.
      BrushBehavior increase_opacity_near_stroke_start_behavior = {
          .nodes = {SourceNode{
                        .source =
                            Source::kDistanceTraveledInMultiplesOfBrushSize,
                        .source_out_of_range_behavior = OutOfRange::kClamp,
                        .source_value_range = {0.0f, 2.0f},
                    },
                    DampingNode{
                        .damping_source = ProgressDomain::kTimeInSeconds,
                        .damping_gap = 0.01f,
                    },
                    TargetNode{
                        .target = Target::kOpacityMultiplier,
                        .target_modifier_range = {1.2f, 1.0f},
                    }},
          .developer_comment =
              "Subtly increases the opacity of the highlighter stroke near the "
              "start of the stroke."};
      BrushBehavior increase_opacity_near_stroke_end_behavior = {
          .nodes = {SourceNode{
                        .source =
                            Source::kDistanceRemainingInMultiplesOfBrushSize,
                        .source_out_of_range_behavior = OutOfRange::kClamp,
                        .source_value_range = {0.4f, 2.4f},
                    },
                    DampingNode{
                        .damping_source = ProgressDomain::kTimeInSeconds,
                        .damping_gap = 0.01f,
                    },
                    TargetNode{
                        .target = Target::kOpacityMultiplier,
                        .target_modifier_range = {1.2f, 1.0f},
                    }},
          .developer_comment =
              "Subtly increases the opacity of the highlighter stroke near the "
              "end of the stroke."};
      BrushBehavior shrink_stroke_end_behind_emoji_stamp_behavior = {
          .nodes = {SourceNode{
                        .source =
                            Source::kDistanceRemainingInMultiplesOfBrushSize,
                        .source_out_of_range_behavior = OutOfRange::kClamp,
                        .source_value_range = {0.3f, 0.0f},
                    },
                    DampingNode{
                        .damping_source = ProgressDomain::kTimeInSeconds,
                        .damping_gap = 0.01f,
                    },
                    TargetNode{
                        .target = Target::kSizeMultiplier,
                        .target_modifier_range = {1.0f, 0.04f},
                    }},
          .developer_comment =
              "Shrinks the size of the highlighter stroke at the very end of "
              "the stroke, where it is hidden behind the large emoji stamp. "
              "This helps prevent end of the highlighter stroke from peeking "
              "out around the edges of the emoji stamp."};
      BrushTip highlighter_tip = {
          .scale = {1.0f, 1.0f},
          .corner_rounding = 1.0f,
          .behaviors = {PredictionFadeOutBehavior(),
                        increase_opacity_near_stroke_start_behavior,
                        increase_opacity_near_stroke_end_behavior,
                        shrink_stroke_end_behind_emoji_stamp_behavior},
      };
      std::vector<BrushCoat> coats = {BrushCoat{
          .tip = highlighter_tip,
          .paint_preferences = {BrushPaint{.self_overlap = self_overlap}},
      }};
      // Minimoji trail coats.
      if (show_mini_emoji_trail) {
        coats.push_back(MiniEmojiCoat(client_texture_id,
                                      /*tip_scale=*/0.4f,
                                      /*tip_rotation_degrees=*/0.0f,
                                      /*tip_particle_gap_distance_scale=*/1.0f,
                                      /*position_offset_range_start=*/-0.35f,
                                      /*position_offset_range_end=*/0.35f,
                                      /*distance_traveled_range_start=*/0.0f,
                                      /*distance_traveled_range_end=*/0.22f,
                                      /*luminosity_range_start=*/0.48f,
                                      /*luminosity_range_end=*/2.0f));
        coats.push_back(MiniEmojiCoat(client_texture_id,
                                      /*tip_scale=*/0.3f,
                                      /*tip_rotation_degrees=*/-35.0f,
                                      /*tip_particle_gap_distance_scale=*/1.3f,
                                      /*position_offset_range_start=*/-0.4f,
                                      /*position_offset_range_end=*/0.32f,
                                      /*distance_traveled_range_start=*/0.1f,
                                      /*distance_traveled_range_end=*/0.74f,
                                      /*luminosity_range_start=*/0.8f,
                                      /*luminosity_range_end=*/2.0f));
        coats.push_back(MiniEmojiCoat(client_texture_id,
                                      /*tip_scale=*/0.45f,
                                      /*tip_rotation_degrees=*/45.0f,
                                      /*tip_particle_gap_distance_scale=*/1.8f,
                                      /*position_offset_range_start=*/-0.25f,
                                      /*position_offset_range_end=*/0.25f,
                                      /*distance_traveled_range_start=*/0.01f,
                                      /*distance_traveled_range_end=*/0.74f,
                                      /*luminosity_range_start=*/0.8f,
                                      /*luminosity_range_end=*/2.0f));
      }
      // Emoji stamp coat.
      BrushBehavior distance_to_size_behavior = {
          .nodes =
              {
                  SourceNode{
                      .source =
                          Source::kDistanceRemainingInMultiplesOfBrushSize,
                      .source_out_of_range_behavior = OutOfRange::kClamp,
                      .source_value_range = {0.01f, 0.0f},
                  },
                  TargetNode{
                      .target = Target::kSizeMultiplier,
                      .target_modifier_range = {0.0f, 1.0f},
                  },
              },
          .developer_comment =
              "Shrinks the tip size to zero everywhere except at the very end "
              "of the stroke, effectively making this brush coat into a single "
              "stamp that moves with the input as the stroke is being drawn."};
      coats.push_back(BrushCoat{
          .tip =
              {
                  .scale = {kEmojiStampScale, kEmojiStampScale},
                  .corner_rounding = 0.0f,
                  .behaviors = {distance_to_size_behavior},
              },
          .paint_preferences = {{
              .texture_layers = {{
                  .client_texture_id = client_texture_id,
                  .origin = BrushPaint::TextureOrigin::kLastStrokeInput,
                  .size_unit = BrushPaint::TextureSizeUnit::kBrushSize,
                  .wrap_x = BrushPaint::TextureWrap::kClamp,
                  .wrap_y = BrushPaint::TextureWrap::kClamp,
                  .size = {kEmojiStampScale, kEmojiStampScale},
                  .offset = {-0.5f, -0.5f},
                  .blend_mode = BrushPaint::BlendMode::kSrc,
              }},
          }},
      });
      absl::StatusOr<BrushFamily> emoji_highlighter(BrushFamily::Create(
          coats, StockInputModel(),
          Metadata{.developer_comment =
                       "A highlighter brush that sweeps a large emoji stamp "
                       "along the stroke as it's being drawn, leaving a "
                       "highlighter stroke and a trail of temporary animated "
                       "mini emoji particles in its wake. This brush can be "
                       "instantiated with different emoji images to create "
                       "different connotations for the annotation."}));
      ABSL_CHECK_OK(emoji_highlighter);
      return *emoji_highlighter;
    }
  }
  ABSL_CHECK(false) << "Unsupported emoji highlighter version: "
                    << static_cast<int>(version);
}

}  // namespace ink::stock_brushes
