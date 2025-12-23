// Copyright 2025 Google LLC
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

#ifndef INK_BRUSH_STOCK_BRUSHES_H_
#define INK_BRUSH_STOCK_BRUSHES_H_

#include <string>

#include "ink/brush/brush_behavior.h"
#include "ink/brush/brush_coat.h"
#include "ink/brush/brush_family.h"
#include "ink/brush/brush_paint.h"

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
 * BrushFamily marker = StockBrushes::marker(MarkerVersion::kV1);
 * ```
 *
 * Specific stock brushes may see minor tweaks and bug-fixes when the library is
 * upgraded, but will avoid major changes in behavior.
 */

namespace ink::stock_brushes {

/** Version option for the [marker] stock brush factory function. */
enum class MarkerVersion {
  /** Initial version of a simple, circular fixed-width brush. */
  kV1 = 1,
  /** Whichever version of marker is currently the latest. */
  kLatest = kV1,
};

/** Version option for the [pressurePen] stock brush factory function. */
enum class PressurePenVersion {
  /**
   * Initial version of a pressure- and speed-sensitive brush that is
   * optimized for handwriting with a stylus.
   */
  kV1 = 1,
  /**
   * The latest version of a pressure- and speed-sensitive brush that is
   * optimized for handwriting with a stylus.
   */
  kLatest = kV1,
};

/** Version option for the [highlighter] stock brush factory function. */
enum class HighlighterVersion {
  /**
   * Initial of a chisel-tip brush that is intended for highlighting text in a
   * document (when used with a translucent brush color).
   */
  kV1 = 1,
  /**
   * The latest version of a chisel-tip brush that is intended for highlighting
   * text in a document (when used with a translucent brush color).
   */
  kLatest = kV1,
};

/** Version option for the [dashedLine] stock brush factory function. */
enum class DashedLineVersion {
  /**
   * Initial version of a brush that appears as rounded rectangles with gaps
   * in between them. This may be decorative, or can be used to signify a user
   * interaction like free-form (lasso) selection.
   */
  kV1 = 1,
  /** The latest version of a dashed-line brush. */
  kLatest = kV1,
};

/** Version option for the [emojiHighlighter] stock brush factory function. */
enum class EmojiHighlighterVersion {
  /**
   * Initial version of emoji highlighter, which has a colored streak drawing
   * behind a moving emoji sticker, possibly with a trail of miniature
   * versions of the chosen emoji sparkling behind.
   */
  kV1 = 1,
  /** Whichever version of emoji highlighter is currently the latest. */
  kLatest = kV1,
};

/**
 * Factory function for constructing a simple marker brush.
 *
 * @param version The version of the marker brush to use. By default, uses the
 * latest version.
 */
BrushFamily Marker(const MarkerVersion& version = MarkerVersion::kLatest);

/**
 * Factory function for constructing a pressure- and speed-sensitive brush that
 * is optimized for handwriting with a stylus.
 *
 * @param version The version of the pressure pen brush to use. By default, uses
 * the latest version.
 */
BrushFamily PressurePen(
    const PressurePenVersion& version = PressurePenVersion::kLatest);

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
BrushFamily Highlighter(
    const BrushPaint::SelfOverlap& selfOverlap = BrushPaint::SelfOverlap::kAny,
    const HighlighterVersion& version = HighlighterVersion::kLatest);

/**
 * Factory function for constructing a brush that appears as rounded rectangles
 * with gaps in between them. This may be decorative, or can be used to signify
 * a user interaction like free-form (lasso) selection.
 *
 * @param version The version of the dashed line brush to use. By default, uses
 * the latest version.
 */
BrushFamily DashedLine(
    const DashedLineVersion& version = DashedLineVersion::kLatest);

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
 * background in the implementation of the texture bitmap store passed to the
 * renderer.
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
BrushFamily EmojiHighlighter(
    std::string client_texture_id, bool show_mini_emoji_trail = false,
    const BrushPaint::SelfOverlap& self_overlap = BrushPaint::SelfOverlap::kAny,
    const EmojiHighlighterVersion& version = EmojiHighlighterVersion::kLatest);

/**
 * The scale factor to apply to both X and Y dimensions of the mini emoji
 * brush tip and texture layer size.
 */
constexpr float kEmojiStampScale = 1.5f;
constexpr BrushFamily::InputModel StockInputModel();
BrushBehavior PredictionFadeOutBehavior();

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
BrushCoat MiniEmojiCoat(
    std::string client_texture_id, float tip_scale, float tip_rotation_degrees,
    float tip_particle_gap_distance_scale, float position_offset_range_start,
    float position_offset_range_end, float distance_traveled_range_start,
    float distance_traveled_range_end, float luminosity_range_start,
    float luminosity_range_end);
}  // namespace ink::stock_brushes

#endif  // INK_BRUSH_STOCK_BRUSHES_H_
