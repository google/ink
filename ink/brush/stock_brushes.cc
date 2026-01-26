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

#include <set>
#include <string>
#include <vector>

#include "absl/base/no_destructor.h"
#include "absl/log/absl_check.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "ink/brush/brush_behavior.h"
#include "ink/brush/brush_coat.h"
#include "ink/brush/brush_family.h"
#include "ink/brush/brush_paint.h"
#include "ink/brush/brush_tip.h"
#include "ink/brush/easing_function.h"
#include "ink/geometry/angle.h"

namespace ink::stock_brushes {

using BinaryOp = BrushBehavior::BinaryOp;
using BinaryOpNode = BrushBehavior::BinaryOpNode;
using ConstantNode = BrushBehavior::ConstantNode;
using DampingNode = BrushBehavior::DampingNode;
using OutOfRange = BrushBehavior::OutOfRange;
using ProgressDomain = BrushBehavior::ProgressDomain;
using ResponseNode = BrushBehavior::ResponseNode;
using Source = BrushBehavior::Source;
using SourceNode = BrushBehavior::SourceNode;
using Target = BrushBehavior::Target;
using TargetNode = BrushBehavior::TargetNode;
using ToolTypeFilterNode = BrushBehavior::ToolTypeFilterNode;

BrushBehavior PredictionFadeOutBehavior() {
  static const absl::NoDestructor<BrushBehavior> behavior(
      {.nodes = {
           SourceNode{.source = Source::kPredictedTimeElapsedInMillis,
                      .source_value_range = {0.0f, 24.0f}},
           // The second branch of the binary op node keeps the opacity fade-out
           // from starting until the predicted inputs have traveled at
           // least 1.5x brush-size.
           SourceNode{
               .source =
                   Source::kPredictedDistanceTraveledInMultiplesOfBrushSize,
               .source_value_range = {1.5f, 2.0f}},
           ResponseNode{
               .response_curve = {EasingFunction::Predefined::kEaseInOut}},
           BinaryOpNode{.operation = BinaryOp::kProduct},
           TargetNode{.target = Target::kOpacityMultiplier,
                      .target_modifier_range = {1.0f, 0.3f}}}});
  return *behavior;
}

constexpr BrushFamily::InputModel StockInputModel() {
  return BrushFamily::SlidingWindowModel();
}

BrushFamily Marker(const MarkerVersion& version) {
  switch (version) {
    case MarkerVersion::kV1: {
      static const absl::NoDestructor<absl::StatusOr<BrushFamily>> marker_v1(
          BrushFamily::Create(
              BrushTip{.behaviors = {PredictionFadeOutBehavior()}},
              BrushPaint{}, StockInputModel()));
      ABSL_CHECK_OK(*marker_v1);
      return **marker_v1;
    }
    default:
      ABSL_CHECK(false) << "Unsupported marker version: "
                        << static_cast<int>(version);
  }
}

BrushFamily PressurePen(const PressurePenVersion& version) {
  switch (version) {
    case PressurePenVersion::kV1: {
      static const absl::NoDestructor<BrushBehavior>
          distance_remaining_to_size_behavior(BrushBehavior{
              .nodes = {
                  SourceNode{
                      .source =
                          Source::kDistanceRemainingInMultiplesOfBrushSize,
                      .source_out_of_range_behavior = OutOfRange::kClamp,
                      .source_value_range = {3.0f, 0.0f},
                  },
                  TargetNode{.target = Target::kSizeMultiplier,
                             .target_modifier_range = {1.0f, 0.75f}},
              }});
      static const absl::NoDestructor<SourceNode> acceleration_source_node(
          SourceNode{
              .source = Source::
                  kInputAccelerationLateralInCentimetersPerSecondSquared,
              .source_out_of_range_behavior = OutOfRange::kClamp,
              .source_value_range = {-80.0f, -230.0f},
          });
      static const absl::NoDestructor<BrushBehavior>
          acceleration_damped_to_size_behavior(BrushBehavior{
              .nodes = {
                  *acceleration_source_node,
                  DampingNode{
                      .damping_source = ProgressDomain::kTimeInSeconds,
                      .damping_gap = .025f,
                  },
                  TargetNode{
                      .target = Target::kSizeMultiplier,
                      .target_modifier_range = {1.0f, 1.25f},
                  },
              }});
      static const absl::NoDestructor<absl::StatusOr<BrushFamily>>
          pressure_pen_v1(BrushFamily::Create(
              BrushTip{
                  .behaviors =
                      {
                          PredictionFadeOutBehavior(),
                          *distance_remaining_to_size_behavior,
                          BrushBehavior{
                              .nodes =
                                  {
                                      SourceNode{
                                          .source =
                                              Source::kNormalizedDirectionY,
                                          .source_out_of_range_behavior =
                                              OutOfRange::kClamp,
                                          .source_value_range = {0.45f, 0.65f},
                                      },
                                      DampingNode{
                                          .damping_source =
                                              ProgressDomain::kTimeInSeconds,
                                          .damping_gap = .025f,
                                      },
                                      TargetNode{
                                          .target = Target::kSizeMultiplier,
                                          .target_modifier_range = {1.0f,
                                                                    1.17f},
                                      },
                                  }},
                          *acceleration_damped_to_size_behavior,
                          BrushBehavior{
                              .nodes =
                                  {
                                      SourceNode{
                                          .source = Source::kNormalizedPressure,
                                          .source_out_of_range_behavior =
                                              OutOfRange::kClamp,
                                          .source_value_range = {0.8f, 1.0f},
                                      },
                                      DampingNode{
                                          .damping_source =
                                              ProgressDomain::kTimeInSeconds,
                                          .damping_gap = .03f,
                                      },
                                      ToolTypeFilterNode{
                                          .enabled_tool_types = {.stylus =
                                                                     true},
                                      },
                                      TargetNode{
                                          .target = Target::kSizeMultiplier,
                                          .target_modifier_range = {1.0f, 1.5f},
                                      },
                                  }},
                      }},
              BrushPaint{}, StockInputModel()));
      ABSL_CHECK_OK(*pressure_pen_v1);
      return **pressure_pen_v1;
    }
    default:
      ABSL_CHECK(false) << "Unsupported pressure pen version: "
                        << static_cast<int>(version);
  }
}

BrushFamily Highlighter(const BrushPaint::SelfOverlap& selfOverlap,
                        const HighlighterVersion& version) {
  static const absl::NoDestructor<std::set<BrushPaint::SelfOverlap>>
      valid_self_overlaps({BrushPaint::SelfOverlap::kAny,
                           BrushPaint::SelfOverlap::kDiscard,
                           BrushPaint::SelfOverlap::kAccumulate});
  if (!valid_self_overlaps->contains(selfOverlap)) {
    ABSL_CHECK(false) << "Unsupported highlighter self overlap: "
                      << static_cast<int>(selfOverlap);
  }
  switch (version) {
    case HighlighterVersion::kV1: {
      static const absl::NoDestructor<BrushBehavior>
          distance_remaining_damped_to_corner_rounding_behavior(BrushBehavior{
              .nodes = {
                  SourceNode{
                      .source =
                          Source::kDistanceRemainingInMultiplesOfBrushSize,
                      .source_out_of_range_behavior = OutOfRange::kClamp,
                      .source_value_range = {0.0f, 1.0f},
                  },
                  DampingNode{
                      .damping_source = ProgressDomain::kTimeInSeconds,
                      .damping_gap = 0.015f,
                  },
                  TargetNode{
                      .target = Target::kCornerRoundingOffset,
                      .target_modifier_range = {0.3f, 1.0f},
                  },
              }});
      static const absl::NoDestructor<BrushBehavior>
          distance_traveled_damped_to_corner_rounding_behavior(BrushBehavior{
              .nodes = {
                  SourceNode{
                      .source = Source::kDistanceTraveledInMultiplesOfBrushSize,
                      .source_out_of_range_behavior = OutOfRange::kClamp,
                      .source_value_range = {0.0f, 1.0f},
                  },
                  DampingNode{
                      .damping_source = ProgressDomain::kTimeInSeconds,
                      .damping_gap = 0.015f,
                  },
                  TargetNode{
                      .target = Target::kCornerRoundingOffset,
                      .target_modifier_range = {0.3f, 1.0f},
                  },
              }});
      static const absl::NoDestructor<BrushBehavior>
          distance_traveled_damped_to_opacity_behavior(BrushBehavior{
              .nodes = {
                  SourceNode{
                      .source = Source::kDistanceTraveledInMultiplesOfBrushSize,
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
                  },
              }});
      static const absl::NoDestructor<BrushBehavior>
          distance_remaining_damped_to_opacity_behavior(BrushBehavior{
              .nodes = {
                  SourceNode{
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
                  },
              }});
      static const absl::NoDestructor<BrushTip> tip(
          BrushTip{.scale = {0.25f, 1.0f},
                   .corner_rounding = 0.3f,
                   .rotation = Angle::Degrees(150.0f),
                   .behaviors = {
                       PredictionFadeOutBehavior(),
                       *distance_remaining_damped_to_corner_rounding_behavior,
                       *distance_traveled_damped_to_corner_rounding_behavior,
                       *distance_traveled_damped_to_opacity_behavior,
                       *distance_remaining_damped_to_opacity_behavior,
                   }});
      switch (selfOverlap) {
        case BrushPaint::SelfOverlap::kAny: {
          static const absl::NoDestructor<absl::StatusOr<BrushFamily>>
              highlighter_any_v1(BrushFamily::Create(
                  *tip,
                  BrushPaint{.self_overlap = BrushPaint::SelfOverlap::kAny},
                  StockInputModel()));
          ABSL_CHECK_OK(*highlighter_any_v1);
          return **highlighter_any_v1;
        }
        case BrushPaint::SelfOverlap::kDiscard: {
          static const absl::NoDestructor<absl::StatusOr<BrushFamily>>
              highlighter_discard_v1(BrushFamily::Create(
                  *tip,
                  BrushPaint{.self_overlap = BrushPaint::SelfOverlap::kDiscard},
                  StockInputModel()));
          ABSL_CHECK_OK(*highlighter_discard_v1);
          return **highlighter_discard_v1;
        }
        case BrushPaint::SelfOverlap::kAccumulate: {
          static const absl::NoDestructor<absl::StatusOr<BrushFamily>>
              highlighter_accumulate_v1(BrushFamily::Create(
                  *tip,
                  BrushPaint{.self_overlap =
                                 BrushPaint::SelfOverlap::kAccumulate},
                  StockInputModel()));
          ABSL_CHECK_OK(*highlighter_accumulate_v1);
          return **highlighter_accumulate_v1;
        }
        default:
          // The check above should make this unreachable, but this way we
          // ensure this code is modified whenever the check above is modified.
          ABSL_CHECK(false) << "Unsupported SelfOverlap value: "
                            << static_cast<int>(selfOverlap);
      }
    }
    default:
      ABSL_CHECK(false) << "Unsupported highlighter version: "
                        << static_cast<int>(version);
  }
}

BrushFamily DashedLine(const DashedLineVersion& version) {
  switch (version) {
    case DashedLineVersion::kV1: {
      static const absl::NoDestructor<absl::StatusOr<BrushFamily>>
          dashed_line_v1(BrushFamily::Create(
              BrushTip{
                  .scale = {2.0f, 1.0f},
                  .corner_rounding = 0.45f,
                  .particle_gap_distance_scale = 3.0f,
                  .behaviors =
                      {
                          PredictionFadeOutBehavior(),
                          BrushBehavior{
                              .nodes =
                                  {
                                      SourceNode{
                                          .source = Source::
                                              kDirectionAboutZeroInRadians,
                                          .source_out_of_range_behavior =
                                              OutOfRange::kClamp,
                                          .source_value_range =
                                              {-kHalfTurn.ValueInRadians(),
                                               kHalfTurn.ValueInRadians()},
                                      },
                                      TargetNode{
                                          .target =
                                              Target::kRotationOffsetInRadians,
                                          .target_modifier_range =
                                              {-kHalfTurn.ValueInRadians(),
                                               kHalfTurn.ValueInRadians()},
                                      },
                                  }},
                      }},
              BrushPaint{}, StockInputModel()));
      ABSL_CHECK_OK(*dashed_line_v1);
      return **dashed_line_v1;
    }
    default:
      ABSL_CHECK(false) << "Unsupported dashed line version: "
                        << static_cast<int>(version);
  }
}

BrushCoat MiniEmojiCoat(
    std::string client_texture_id, float tip_scale, float tip_rotation_degrees,
    float tip_particle_gap_distance_scale, float position_offset_range_start,
    float position_offset_range_end, float distance_traveled_range_start,
    float distance_traveled_range_end, float luminosity_range_start,
    float luminosity_range_end) {
  const BrushBehavior distance_traveled_eased_to_offset_y_behavior =
      BrushBehavior{
          .nodes =
              {
                  SourceNode{
                      .source = Source::kDistanceTraveledInMultiplesOfBrushSize,
                      .source_out_of_range_behavior = OutOfRange::kRepeat,
                      .source_value_range = {distance_traveled_range_start,
                                             distance_traveled_range_end},
                  },
                  ResponseNode{
                      .response_curve =
                          {.parameters = EasingFunction::Predefined::kLinear},
                  },
                  TargetNode{
                      .target = Target::kPositionOffsetYInMultiplesOfBrushSize,
                      .target_modifier_range = {position_offset_range_start,
                                                position_offset_range_end},
                  },
              },
      };
  return BrushCoat{
      .tip =
          BrushTip{
              .scale = {tip_scale, tip_scale},
              .corner_rounding = 0.0f,
              .rotation = Angle::Degrees(tip_rotation_degrees),
              .particle_gap_distance_scale = tip_particle_gap_distance_scale,
              .behaviors =
                  {
                      BrushBehavior{
                          .nodes =
                              {
                                  SourceNode{
                                      .source =
                                          Source::kTimeSinceInputInSeconds,
                                      .source_out_of_range_behavior =
                                          OutOfRange::kClamp,
                                      .source_value_range = {0.0f, 0.7f},
                                  },
                                  TargetNode{
                                      .target = Target::kSizeMultiplier,
                                      .target_modifier_range = {1.0f, 0.0f},
                                  },
                              },
                      },
                      BrushBehavior{
                          .nodes =
                              {
                                  ConstantNode{.value = 0.0f},
                                  TargetNode{
                                      .target = Target::kHueOffsetInRadians,
                                      .target_modifier_range =
                                          {Angle::Degrees(59.0f)
                                               .ValueInRadians(),
                                           Angle::Degrees(60.0f)
                                               .ValueInRadians()},
                                  },
                                  ConstantNode{.value = 0.0f},
                                  TargetNode{
                                      .target = Target::kLuminosity,
                                      .target_modifier_range =
                                          {luminosity_range_start,
                                           luminosity_range_end},
                                  },
                              },
                      },
                      distance_traveled_eased_to_offset_y_behavior,
                  },
          },
      .paint_preferences =
          {
              BrushPaint{
                  .texture_layers =
                      {
                          BrushPaint::TextureLayer{
                              .client_texture_id = client_texture_id,
                              .mapping = BrushPaint::TextureMapping::kStamping,
                              .size_unit =
                                  BrushPaint::TextureSizeUnit::kStrokeSize,
                              .size = {1.0f, 1.0f},
                              .opacity = 0.4f,
                              .blend_mode = BrushPaint::BlendMode::kModulate,
                          },
                      },
              },
          },
  };
}

BrushFamily EmojiHighlighter(std::string client_texture_id,
                             bool show_mini_emoji_trail,
                             const BrushPaint::SelfOverlap& self_overlap,
                             const EmojiHighlighterVersion& version) {
  switch (version) {
    case EmojiHighlighterVersion::kV1: {
      const BrushBehavior distance_eased_product_time_to_opacity_behavior =
          BrushBehavior{
              .nodes =
                  {
                      SourceNode{
                          .source = Source::
                              kPredictedDistanceTraveledInMultiplesOfBrushSize,
                          .source_out_of_range_behavior = OutOfRange::kClamp,
                          .source_value_range = {1.5f, 2.0f},
                      },
                      ResponseNode{
                          .response_curve =
                              {.parameters =
                                   EasingFunction::Predefined::kEaseInOut},
                      },
                      SourceNode{
                          .source = Source::kPredictedTimeElapsedInMillis,
                          .source_out_of_range_behavior = OutOfRange::kClamp,
                          .source_value_range = {0.0f, 24.0f},
                      },
                      BinaryOpNode{
                          .operation = BinaryOp::kProduct,
                      },
                      TargetNode{
                          .target = Target::kOpacityMultiplier,
                          .target_modifier_range = {1.0f, 0.3f},
                      },
                  },
          };
      // Highlighter coat.
      BrushTip highlighter_tip = BrushTip{
          .scale = {1.0f, 1.0f},
          .corner_rounding = 1.0f,
          .behaviors =
              {
                  distance_eased_product_time_to_opacity_behavior,
                  BrushBehavior{
                      .nodes =
                          {
                              SourceNode{
                                  .source = Source::
                                      kDistanceTraveledInMultiplesOfBrushSize,
                                  .source_out_of_range_behavior =
                                      OutOfRange::kClamp,
                                  .source_value_range = {0.0f, 2.0f},
                              },
                              DampingNode{
                                  .damping_source =
                                      ProgressDomain::kTimeInSeconds,
                                  .damping_gap = 0.01f,
                              },
                              TargetNode{
                                  .target = Target::kOpacityMultiplier,
                                  .target_modifier_range = {1.2f, 1.0f},
                              },
                          },
                  },
                  BrushBehavior{
                      .nodes =
                          {
                              SourceNode{
                                  .source = Source::
                                      kDistanceRemainingInMultiplesOfBrushSize,
                                  .source_out_of_range_behavior =
                                      OutOfRange::kClamp,
                                  .source_value_range = {0.4f, 2.4f},
                              },
                              DampingNode{
                                  .damping_source =
                                      ProgressDomain::kTimeInSeconds,
                                  .damping_gap = 0.01f,
                              },
                              TargetNode{
                                  .target = Target::kOpacityMultiplier,
                                  .target_modifier_range = {1.2f, 1.0f},
                              },
                          },
                  },
                  BrushBehavior{
                      .nodes =
                          {
                              SourceNode{
                                  .source = Source::
                                      kDistanceRemainingInMultiplesOfBrushSize,
                                  .source_out_of_range_behavior =
                                      OutOfRange::kClamp,
                                  .source_value_range = {0.3f, 0.0f},
                              },
                              DampingNode{
                                  .damping_source =
                                      ProgressDomain::kTimeInSeconds,
                                  .damping_gap = 0.01f,
                              },
                              TargetNode{
                                  .target = Target::kSizeMultiplier,
                                  .target_modifier_range = {1.0f, 0.04f},
                              },
                          },
                  },
              },
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
      const BrushBehavior distance_to_size_behavior = BrushBehavior{
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
      };
      // Emoji stamp coat.
      coats.push_back(BrushCoat{
          .tip =
              BrushTip{
                  .scale = {kEmojiStampScale, kEmojiStampScale},
                  .corner_rounding = 0.0f,
                  .behaviors =
                      {
                          distance_to_size_behavior,
                      },
              },
          .paint_preferences = {BrushPaint{
              .texture_layers =
                  {
                      BrushPaint::TextureLayer{
                          .client_texture_id = client_texture_id,
                          .origin = BrushPaint::TextureOrigin::kLastStrokeInput,
                          .size_unit = BrushPaint::TextureSizeUnit::kBrushSize,
                          .wrap_x = BrushPaint::TextureWrap::kClamp,
                          .wrap_y = BrushPaint::TextureWrap::kClamp,
                          .size = {kEmojiStampScale, kEmojiStampScale},
                          .offset = {-0.5f, -0.5f},
                          .blend_mode = BrushPaint::BlendMode::kSrc,
                      },
                  },
          }},
      });
      absl::StatusOr<BrushFamily> emoji_highlighter(
          BrushFamily::Create(coats, StockInputModel()));
      ABSL_CHECK_OK(emoji_highlighter);
      return *emoji_highlighter;
    }
    default:
      ABSL_CHECK(false) << "Unsupported emoji highlighter version: "
                        << static_cast<int>(version);
  }
}
}  // namespace ink::stock_brushes
