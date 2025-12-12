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
#include <vector>

#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "ink/brush/brush_behavior.h"
#include "ink/brush/brush_coat.h"
#include "ink/brush/brush_family.h"
#include "ink/brush/brush_paint.h"

namespace ink {

struct Version {
  absl::string_view name;
  absl::string_view prefix;
  std::string toString() const { return absl::StrCat(prefix, name); }
  explicit Version(absl::string_view n, absl::string_view p)
      : name(n), prefix(p) {}
};
/** Version option for the [marker] stock brush factory function. */
struct MarkerVersion : public Version {
  explicit MarkerVersion(absl::string_view n) : Version(n, "MarkerVersion.") {}
};
namespace marker {
/** Initial version of a simple, circular fixed-width brush. */
const MarkerVersion V1 = MarkerVersion("V1");
/** Whichever version of marker is currently the latest. */
const MarkerVersion LATEST = V1;
};  // namespace marker

/** Version option for the [pressurePen] stock brush factory function. */
struct PressurePenVersion : public Version {
  explicit PressurePenVersion(absl::string_view n)
      : Version(n, "PressurePenVersion.") {}
};
namespace pressure_pen {
/**
 * Initial version of a pressure- and speed-sensitive brush that is
 * optimized for handwriting with a stylus.
 */
const PressurePenVersion V1 = PressurePenVersion("V1");
/**
 * The latest version of a pressure- and speed-sensitive brush that is
 * optimized for handwriting with a stylus.
 */
const PressurePenVersion LATEST = V1;
};  // namespace pressure_pen

/** Version option for the [highlighter] stock brush factory function. */
struct HighlighterVersion : public Version {
  explicit HighlighterVersion(absl::string_view n)
      : Version(n, "HighlighterVersion.") {}
};
namespace highlighter {
/**
 * Initial of a chisel-tip brush that is intended for highlighting text in a
 * document (when used with a translucent brush color).
 */
const HighlighterVersion V1 = HighlighterVersion("V1");
/**
 * The latest version of a chisel-tip brush that is intended for highlighting
 * text in a document (when used with a translucent brush color).
 */
const HighlighterVersion LATEST = V1;
};  // namespace highlighter

/** Version option for the [dashedLine] stock brush factory function. */
struct DashedLineVersion : public Version {
  explicit DashedLineVersion(absl::string_view n)
      : Version(n, "DashedLineVersion.") {}
};
namespace dashed_line {
/**
 * Initial version of a brush that appears as rounded rectangles with gaps
 * in between them. This may be decorative, or can be used to signify a user
 * interaction like free-form (lasso) selection.
 */
const DashedLineVersion V1 = DashedLineVersion("V1");
/** The latest version of a dashed-line brush. */
const DashedLineVersion LATEST = V1;
};  // namespace dashed_line

/** Version option for the [emojiHighlighter] stock brush factory function. */
struct EmojiHighlighterVersion : public Version {
  explicit EmojiHighlighterVersion(absl::string_view n)
      : Version(n, "EmojiHighlighterVersion.") {}
};
namespace emoji_highlighter {
/**
 * Initial version of emoji highlighter, which has a colored streak drawing
 * behind a moving emoji sticker, possibly with a trail of miniature
 * versions of the chosen emoji sparkling behind.
 */
const EmojiHighlighterVersion V1 = EmojiHighlighterVersion("V1");
/** Whichever version of emoji highlighter is currently the latest. */
const EmojiHighlighterVersion LATEST = V1;
};  // namespace emoji_highlighter

namespace stock_brushes {
absl::StatusOr<BrushFamily> marker(
    const MarkerVersion& version = marker::LATEST);

absl::StatusOr<BrushFamily> pressurePen(
    const PressurePenVersion& version = pressure_pen::LATEST);

absl::StatusOr<BrushFamily> highlighter(
    const BrushPaint::SelfOverlap& selfOverlap = BrushPaint::SelfOverlap::kAny,
    const HighlighterVersion& version = highlighter::LATEST);

absl::StatusOr<BrushFamily> dashedLine(
    const DashedLineVersion& version = dashed_line::LATEST);

absl::StatusOr<BrushFamily> emojiHighlighter(
    std::string client_texture_id, bool show_mini_emoji_trail = false,
    const BrushPaint::SelfOverlap& self_overlap = BrushPaint::SelfOverlap::kAny,
    const EmojiHighlighterVersion& version = emoji_highlighter::LATEST);

/**
 * The scale factor to apply to both X and Y dimensions of the mini emoji
 * brush tip and texture layer size.
 */
constexpr float kEmojiStampScale = 1.5f;
BrushFamily::InputModel stockInputModel();
BrushBehavior predictionFadeOutBehavior();
BrushCoat miniEmojiCoat(
    std::string client_texture_id, float tip_scale, float tip_rotation_degrees,
    float tip_particle_gap_distance_scale, float position_offset_range_start,
    float position_offset_range_end, float distance_traveled_range_start,
    float distance_traveled_range_end, float luminosity_range_start,
    float luminosity_range_end);

/** The client texture ID for the background of the version-1 pencil brush. */
static constexpr char kPencilBackgroundUnstableTextureId[] =
    "androidx.ink.brush.StockBrushes.pencil_background_unstable";
absl::StatusOr<BrushFamily> pencilUnstable();

/* For testing purposes, it is convenient to have a named parameter for
 * each stock brush. As stock brushes are added or updated, this function will
 * need to be updated.
 */
using StockBrushesTestParam = std::pair<std::string, BrushFamily>;
std::vector<StockBrushesTestParam> GetParams();
}  // namespace stock_brushes
}  // namespace ink

#endif  // INK_BRUSH_STOCK_BRUSHES_H_
