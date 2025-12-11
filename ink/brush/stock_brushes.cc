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

#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "ink/brush/brush_behavior.h"
#include "ink/brush/brush_coat.h"
#include "ink/brush/brush_family.h"
#include "ink/brush/brush_paint.h"
#include "ink/brush/brush_tip.h"
#include "ink/brush/easing_function.h"
#include "ink/geometry/angle.h"

/**
 * Provides a fixed set of stock [BrushFamily] objects that any app can use.
 *
 * All stock brushes are versioned, so apps can store input points and brush
 * specs instead of the pixel result, but be able to regenerate strokes from
 * stored input points that look generally like the strokes originally drawn by
 * the user. Stock brushes are intended to evolve over time.
 *
 * Each successive stock brush version will keep to the spirit of the brush, but
 * the details can change between versions. For example, a new version of the
 * highlighter may introduce a variation on how round the tip is, or what sort
 * of curve maps color to pressure.
 *
 * We generally recommend that applications use the latest brush version
 * available, which is what the factory functions in this class do by default.
 * But for some artistic use-cases, it may be useful to specify a specific stock
 * brush version to minimize visual changes when the Ink dependency is upgraded.
 * For example, the following will always return the initial version of the
 * marker stock brush.
 *
 * ```cpp
 * absl::StatusOr<BrushFamily> marker = StockBrushes::marker(MarkerVersion::V1);
 * ```
 *
 * Or in Kotlin:
 *
 * ```kt
 * val markerBrush = StockBrushes.marker(StockBrushes.MarkerVersion.V1)
 * ```
 *
 * Specific stock brushes may see minor tweaks and bug-fixes when the library is
 * upgraded, but will avoid major changes in behavior.
 *
 */
namespace ink::stock_brushes {

using BinaryOp = BrushBehavior::BinaryOp;
using BinaryOpNode = BrushBehavior::BinaryOpNode;
using ConstantNode = BrushBehavior::ConstantNode;
using DampingSource = BrushBehavior::DampingSource;
using DampingNode = BrushBehavior::DampingNode;
using OutOfRange = BrushBehavior::OutOfRange;
using ResponseNode = BrushBehavior::ResponseNode;
using Source = BrushBehavior::Source;
using SourceNode = BrushBehavior::SourceNode;
using Target = BrushBehavior::Target;
using TargetNode = BrushBehavior::TargetNode;
using ToolTypeFilterNode = BrushBehavior::ToolTypeFilterNode;

BrushBehavior predictionFadeOutBehavior() {
  return BrushBehavior{
      .nodes = {
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
                     .target_modifier_range = {1.0f, 0.3f}}}};
}

BrushFamily::InputModel stockInputModel() {
  return BrushFamily::SlidingWindowModel();
}

/**
 * Factory function for constructing a simple marker brush.
 *
 * @param version The version of the marker brush to use. By default, uses the
 * latest version.
 */
absl::StatusOr<BrushFamily> marker(const MarkerVersion& version) {
  if (version.name == marker::V1.name) {
    return BrushFamily::Create(
        BrushTip{.behaviors = {predictionFadeOutBehavior()}}, BrushPaint{}, "",
        stockInputModel());
  }
  return absl::InvalidArgumentError(
      absl::StrCat("Unsupported marker version: ", version.name));
}

/**
 * Factory function for constructing a pressure- and speed-sensitive brush that
 * is optimized for handwriting with a stylus.
 *
 * @param version The version of the pressure pen brush to use. By default, uses
 * the latest version.
 */
absl::StatusOr<BrushFamily> pressurePen(const PressurePenVersion& version) {
  if (version.name == pressure_pen::V1.name) {
    return BrushFamily::Create(
        BrushTip{
            .behaviors =
                {
                    predictionFadeOutBehavior(),
                    BrushBehavior{
                        .nodes =
                            {
                                SourceNode{
                                    .source = Source::
                                        kDistanceRemainingInMultiplesOfBrushSize,  // NOLINT
                                    .source_out_of_range_behavior =
                                        OutOfRange::kClamp,
                                    .source_value_range = {3.0f, 0.0f},
                                },
                                TargetNode{
                                    .target = Target::kSizeMultiplier,
                                    .target_modifier_range = {1.0f, 0.75f}},
                            }},
                    BrushBehavior{
                        .nodes =
                            {
                                SourceNode{
                                    .source = Source::kNormalizedDirectionY,
                                    .source_out_of_range_behavior =
                                        OutOfRange::kClamp,
                                    .source_value_range = {0.45f, 0.65f},
                                },
                                DampingNode{
                                    .damping_source =
                                        DampingSource::kTimeInSeconds,
                                    .damping_gap = .025f,
                                },
                                TargetNode{
                                    .target = Target::kSizeMultiplier,
                                    .target_modifier_range = {1.0f, 1.17f},
                                },
                            }},
                    BrushBehavior{
                        .nodes =
                            {
                                SourceNode{
                                    .source = Source::
                                        kInputAccelerationLateralInCentimetersPerSecondSquared,  // NOLINT
                                    .source_out_of_range_behavior =
                                        OutOfRange::kClamp,
                                    .source_value_range = {-80.0f, -230.0f},
                                },
                                DampingNode{
                                    .damping_source =
                                        DampingSource::kTimeInSeconds,
                                    .damping_gap = .025f,
                                },
                                TargetNode{
                                    .target = Target::kSizeMultiplier,
                                    .target_modifier_range = {1.0f, 1.25f},
                                },
                            }},
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
                                        DampingSource::kTimeInSeconds,
                                    .damping_gap = .03f,
                                },
                                ToolTypeFilterNode{
                                    .enabled_tool_types = {.stylus = true},
                                },
                                TargetNode{
                                    .target = Target::kSizeMultiplier,
                                    .target_modifier_range = {1.0f, 1.5f},
                                },
                            }},
                }},
        BrushPaint{}, "", stockInputModel());
  }
  return absl::InvalidArgumentError(
      absl::StrCat("Unsupported pressure pen version: ", version.name));
}

/**
 * Factory function for constructing a chisel-tip brush that is intended for
 * highlighting text in a document (when used with a translucent brush color).
 *
 * @param selfOverlap Guidance to renderers on how to treat self-overlapping
 * areas of strokes created with this brush. See [BrushPaint::SelfOverlap] for
 * more detail. Consider using [BrushPaint::SelfOverlap::kDiscard] if the visual
 * representation of the stroke must look exactly the same across all Android
 * versions, or if the visual representation must match that of an exported PDF
 * path or SVG object based on strokes authored using this brush.
 * @param version The version of the highlighter brush to use. By default, uses
 * the latest version.
 */
absl::StatusOr<BrushFamily> highlighter(
    const BrushPaint::SelfOverlap& selfOverlap,
    const HighlighterVersion& version) {
  std::set<BrushPaint::SelfOverlap> valid_self_overlaps = {
      BrushPaint::SelfOverlap::kAny, BrushPaint::SelfOverlap::kDiscard,
      BrushPaint::SelfOverlap::kAccumulate};
  if (!valid_self_overlaps.contains(selfOverlap)) {
    return absl::InvalidArgumentError(
        absl::StrCat("Unrecognized SelfOverlap value: ", selfOverlap));
  }
  if (version.name == highlighter::V1.name) {
    return BrushFamily::Create(
        BrushTip{
            .scale = {0.25f, 1.0f},
            .corner_rounding = 0.3f,
            .rotation = Angle::Degrees(150.0f),
            .behaviors =
                {
                    predictionFadeOutBehavior(),
                    BrushBehavior{
                        .nodes =
                            {
                                SourceNode{
                                    .source = Source::
                                        kDistanceRemainingInMultiplesOfBrushSize,  // NOLINT
                                    .source_out_of_range_behavior =
                                        OutOfRange::kClamp,
                                    .source_value_range = {0.0f, 1.0f},
                                },
                                DampingNode{
                                    .damping_source =
                                        DampingSource::kTimeInSeconds,
                                    .damping_gap = 0.015f,
                                },
                                TargetNode{
                                    .target = Target::kCornerRoundingOffset,
                                    .target_modifier_range = {0.3f, 1.0f},
                                },
                            }},
                    BrushBehavior{
                        .nodes =
                            {
                                SourceNode{
                                    .source = Source::
                                        kDistanceTraveledInMultiplesOfBrushSize,
                                    .source_out_of_range_behavior =
                                        OutOfRange::kClamp,
                                    .source_value_range = {0.0f, 1.0f},
                                },
                                DampingNode{
                                    .damping_source =
                                        DampingSource::kTimeInSeconds,
                                    .damping_gap = 0.015f,
                                },
                                TargetNode{
                                    .target = Target::kCornerRoundingOffset,
                                    .target_modifier_range = {0.3f, 1.0f},
                                },
                            }},
                    BrushBehavior{
                        .nodes =
                            {
                                SourceNode{
                                    .source = Source::
                                        kDistanceTraveledInMultiplesOfBrushSize,
                                    .source_out_of_range_behavior =
                                        OutOfRange::kClamp,
                                    .source_value_range = {0.0f, 3.0f},
                                },
                                DampingNode{
                                    .damping_source =
                                        DampingSource::kTimeInSeconds,
                                    .damping_gap = 0.015f,
                                },
                                TargetNode{
                                    .target = Target::kOpacityMultiplier,
                                    .target_modifier_range = {1.1f, 1.0f},
                                },
                            }},
                    BrushBehavior{
                        .nodes =
                            {
                                SourceNode{
                                    .source = Source::
                                        kDistanceRemainingInMultiplesOfBrushSize,  // NOLINT
                                    .source_out_of_range_behavior =
                                        OutOfRange::kClamp,
                                    .source_value_range = {0.0f, 3.0f},
                                },
                                DampingNode{
                                    .damping_source =
                                        DampingSource::kTimeInSeconds,
                                    .damping_gap = 0.015f,
                                },
                                TargetNode{
                                    .target = Target::kOpacityMultiplier,
                                    .target_modifier_range = {1.1f, 1.0f},
                                },
                            }},
                }},
        BrushPaint{.self_overlap = selfOverlap}, "", stockInputModel());
  }
  return absl::InvalidArgumentError(
      absl::StrCat("Unsupported highlighter version: ", version.name));
}

/**
 * Factory function for constructing a brush that appears as rounded rectangles
 * with gaps in between them. This may be decorative, or can be used to signify
 * a user interaction like free-form (lasso) selection.
 *
 * @param version The version of the dashed line brush to use. By default, uses
 * the latest version.
 */
absl::StatusOr<BrushFamily> dashedLine(const DashedLineVersion& version) {
  if (version.name == dashed_line::V1.name) {
    return BrushFamily::Create(
        BrushTip{
            .scale = {2.0f, 1.0f},
            .corner_rounding = 0.45f,
            .particle_gap_distance_scale = 3.0f,
            .behaviors =
                {
                    predictionFadeOutBehavior(),
                    BrushBehavior{
                        .nodes =
                            {
                                SourceNode{
                                    .source =
                                        Source::kDirectionAboutZeroInRadians,
                                    .source_out_of_range_behavior =
                                        OutOfRange::kClamp,
                                    .source_value_range =
                                        {-kHalfTurn.ValueInRadians(),
                                         kHalfTurn.ValueInRadians()},
                                },
                                TargetNode{
                                    .target = Target::kRotationOffsetInRadians,
                                    .target_modifier_range =
                                        {-kHalfTurn.ValueInRadians(),
                                         kHalfTurn.ValueInRadians()},
                                },
                            }},
                }},
        BrushPaint{}, "", stockInputModel());
  }
  return absl::InvalidArgumentError(
      absl::StrCat("Unsupported dashed line version: ", version.name));
}

/**
 * A development version of a brush that looks like pencil marks on subtly
 * textured paper.
 *
 * In order to use this brush, the [TextureBitmapStore] provided to your
 * renderer must map the [kPencilUnstableBackgroundTextureId] to a bitmap;
 * otherwise, no texture will be visible. Android callers may want to use
 * [StockTextureBitmapStore] to provide this mapping.
 *
 * The behavior of this [BrushFamily] may change significantly in future
 * releases. Once it has stabilized, it will be renamed to `pencilV1`.
 */
// TODO: b/373587591 - Change this to be consistent with the other brush
// factory functions before release.
absl::StatusOr<BrushFamily> pencilUnstable() {
  return BrushFamily::Create(
      BrushTip{.behaviors = {predictionFadeOutBehavior()}},
      BrushPaint{
          .texture_layers = {BrushPaint::TextureLayer{
              .client_texture_id = kPencilBackgroundUnstableTextureId,
              .mapping = BrushPaint::TextureMapping::kTiling,
              .size_unit = BrushPaint::TextureSizeUnit::kStrokeCoordinates,
              .size = {512.0f, 512.0f},
          }}},
      "", stockInputModel());
}

/**
 * A brush coat that looks like a mini emoji.
 *
 * @param clientTextureId the client texture ID of the emoji to appear in the
 * coat.
 * @param tipScale the scale factor to apply to both X and Y dimensions of the
 * mini emoji
 * @param tipRotationDegrees the rotation to apply to the mini emoji
 * @param tipParticleGapDistanceScale the scale factor to apply to the particle
 * gap distance
 * @param positionOffsetRangeStart the start of the range for the position
 * offset behavior
 * @param positionOffsetRangeEnd the end of the range for the position offset
 * behavior
 * @param distanceTraveledRangeStart the start of the range for the distance
 * traveled behavior
 * @param distanceTraveledRangeEnd the end of the range for the distance
 * traveled behavior
 * @param luminosityRangeStart the start of the range for the luminosity
 * behavior
 * @param luminosityRangeEnd the end of the range for the luminosity behavior
 */
BrushCoat miniEmojiCoat(
    std::string client_texture_id, float tip_scale, float tip_rotation_degrees,
    float tip_particle_gap_distance_scale, float position_offset_range_start,
    float position_offset_range_end, float distance_traveled_range_start,
    float distance_traveled_range_end, float luminosity_range_start,
    float luminosity_range_end) {
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
                      BrushBehavior{
                          .nodes =
                              {
                                  SourceNode{
                                      .source = Source::
                                          kDistanceTraveledInMultiplesOfBrushSize,  // NOLINT
                                      .source_out_of_range_behavior =
                                          OutOfRange::kRepeat,
                                      .source_value_range =
                                          {distance_traveled_range_start,
                                           distance_traveled_range_end},
                                  },
                                  ResponseNode{
                                      .response_curve =
                                          {.parameters = EasingFunction::
                                               Predefined::kLinear},
                                  },
                                  TargetNode{
                                      .target = Target::
                                          kPositionOffsetYInMultiplesOfBrushSize,  // NOLINT
                                      .target_modifier_range =
                                          {position_offset_range_start,
                                           position_offset_range_end},
                                  },
                              },
                      },
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

/**
 * Factory function for constructing an emoji highlighter brush.
 *
 * In order to use this brush, the [TextureBitmapStore] provided to your
 * renderer must map the [clientTextureId] to a bitmap; otherwise, no texture
 * will be visible. The emoji bitmap should be a square, though the image can
 * have a transparent background for emoji shapes that aren't square.
 *
 * @param clientTextureId The client texture ID of the emoji to appear at the
 * end of the stroke. This ID should map to a square bitmap with a transparent
 * background in the implementation of
 *   [com.google.inputmethod.ink.brush.TextureBitmapStore] passed to
 *   [com.google.inputmethod.ink.rendering.android.canvas.CanvasStrokeRenderer.create].
 * @param showMiniEmojiTrail Whether to show a trail of miniature emojis
 * disappearing from the stroke as it is drawn. Note that this will only render
 * properly starting with Android U, and before Android U it is recommended to
 * set this to false.
 * @param selfOverlap Guidance to renderers on how to treat self-overlapping
 * areas of strokes created with this brush. See [BrushPaint::SelfOverlap] for
 * more detail. Consider using [BrushPaint::SelfOverlap::kDiscard] if the visual
 * representation of the stroke must look exactly the same across all Android
 * versions, or if the visual representation must match that of an exported PDF
 * path or SVG object based on strokes authored using this brush.
 * @param version The version of the emoji highlighter to use. By default, uses
 * the latest version of the emoji highlighter brush tip and behavior.
 */
absl::StatusOr<BrushFamily> emojiHighlighter(
    std::string client_texture_id, bool show_mini_emoji_trail,
    const BrushPaint::SelfOverlap& self_overlap,
    const EmojiHighlighterVersion& version) {
  if (version.name == emoji_highlighter::V1.name) {
    // Highlighter coat.
    BrushTip highlighter_tip = BrushTip{
        .scale = {1.0f, 1.0f},
        .corner_rounding = 1.0f,
        .behaviors =
            {
                BrushBehavior{
                    .nodes =
                        {
                            SourceNode{
                                .source = Source::
                                    kPredictedDistanceTraveledInMultiplesOfBrushSize,  // NOLINT
                                .source_out_of_range_behavior =
                                    OutOfRange::kClamp,
                                .source_value_range = {1.5f, 2.0f},
                            },
                            ResponseNode{
                                .response_curve =
                                    {.parameters = EasingFunction::Predefined::
                                         kEaseInOut},
                            },
                            SourceNode{
                                .source = Source::kPredictedTimeElapsedInMillis,
                                .source_out_of_range_behavior =
                                    OutOfRange::kClamp,
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
                },
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
                                .damping_source = DampingSource::kTimeInSeconds,
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
                                .damping_source = DampingSource::kTimeInSeconds,
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
                                .damping_source = DampingSource::kTimeInSeconds,
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
      coats.push_back(miniEmojiCoat(client_texture_id,
                                    /*tip_scale=*/0.4f,
                                    /*tip_rotation_degrees=*/0.0f,
                                    /*tip_particle_gap_distance_scale=*/1.0f,
                                    /*position_offset_range_start=*/-0.35f,
                                    /*position_offset_range_end=*/0.35f,
                                    /*distance_traveled_range_start=*/0.0f,
                                    /*distance_traveled_range_end=*/0.22f,
                                    /*luminosity_range_start=*/0.48f,
                                    /*luminosity_range_end=*/2.0f));
      coats.push_back(miniEmojiCoat(client_texture_id,
                                    /*tip_scale=*/0.3f,
                                    /*tip_rotation_degrees=*/-35.0f,
                                    /*tip_particle_gap_distance_scale=*/1.3f,
                                    /*position_offset_range_start=*/-0.4f,
                                    /*position_offset_range_end=*/0.32f,
                                    /*distance_traveled_range_start=*/0.1f,
                                    /*distance_traveled_range_end=*/0.74f,
                                    /*luminosity_range_start=*/0.8f,
                                    /*luminosity_range_end=*/2.0f));
      coats.push_back(miniEmojiCoat(client_texture_id,
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
    coats.push_back(BrushCoat{
        .tip =
            BrushTip{
                .scale = {kEmojiStampScale, kEmojiStampScale},
                .corner_rounding = 0.0f,
                .behaviors =
                    {
                        BrushBehavior{
                            .nodes =
                                {
                                    SourceNode{
                                        .source = Source::
                                            kDistanceRemainingInMultiplesOfBrushSize,  // NOLINT
                                        .source_out_of_range_behavior =
                                            OutOfRange::kClamp,
                                        .source_value_range = {0.01f, 0.0f},
                                    },
                                    TargetNode{
                                        .target = Target::kSizeMultiplier,
                                        .target_modifier_range = {0.0f, 1.0f},
                                    },
                                },
                        },
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
    return BrushFamily::Create(coats, "", stockInputModel());
  }
  return absl::InvalidArgumentError(
      absl::StrCat("Unsupported emoji highlighter version: ", version.name));
}

std::vector<stock_brushes::StockBrushesTestParam> GetParams() {
  std::vector<StockBrushesTestParam> families;
  absl::StatusOr<BrushFamily> f = marker(marker::V1);
  ABSL_CHECK_OK(f);
  families.emplace_back("marker_1", *f);
  f = pressurePen(pressure_pen::V1);
  ABSL_CHECK_OK(f);
  families.emplace_back("pressure_pen_1", *f);
  f = highlighter(BrushPaint::SelfOverlap::kAny, highlighter::V1);
  ABSL_CHECK_OK(f);
  families.emplace_back("highlighter_1", *f);
  f = dashedLine(dashed_line::V1);
  ABSL_CHECK_OK(f);
  families.emplace_back("dashed_line_1", *f);
  f = pencilUnstable();
  ABSL_CHECK_OK(f);
  families.emplace_back("pencil_1", *f);
  f = emojiHighlighter("emoji_heart", true, BrushPaint::SelfOverlap::kAny,
                       emoji_highlighter::V1);
  ABSL_CHECK_OK(f);
  families.emplace_back("heart_emoji_highlighter_1", *f);
  f = emojiHighlighter("emoji_heart", false, BrushPaint::SelfOverlap::kAny,
                       emoji_highlighter::V1);
  ABSL_CHECK_OK(f);
  families.emplace_back("heart_emoji_highlighter_no_trail_1", *f);
  return families;
}
}  // namespace ink::stock_brushes
