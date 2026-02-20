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

#include "ink/brush/brush_paint.h"

#include <cmath>
#include <string>

#include "absl/container/flat_hash_set.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "absl/time/time.h"
#include "ink/brush/color_function.h"
#include "ink/geometry/mesh_format.h"

namespace ink::brush_internal {
namespace {

bool IsValidBrushPaintTextureMapping(BrushPaint::TextureMapping mapping) {
  switch (mapping) {
    case BrushPaint::TextureMapping::kTiling:
    case BrushPaint::TextureMapping::kStamping:
      return true;
  }
  return false;
}

bool IsValidBrushPaintTextureOrigin(BrushPaint::TextureOrigin mapping) {
  switch (mapping) {
    case BrushPaint::TextureOrigin::kStrokeSpaceOrigin:
    case BrushPaint::TextureOrigin::kFirstStrokeInput:
    case BrushPaint::TextureOrigin::kLastStrokeInput:
      return true;
  }
  return false;
}

bool IsValidBrushPaintTextureSizeUnit(BrushPaint::TextureSizeUnit size_unit) {
  switch (size_unit) {
    case BrushPaint::TextureSizeUnit::kBrushSize:
    case BrushPaint::TextureSizeUnit::kStrokeCoordinates:
      return true;
  }
  return false;
}

bool IsValidBrushPaintTextureWrap(BrushPaint::TextureWrap wrap) {
  switch (wrap) {
    case BrushPaint::TextureWrap::kRepeat:
    case BrushPaint::TextureWrap::kMirror:
    case BrushPaint::TextureWrap::kClamp:
      return true;
  }
  return false;
}

bool IsValidBrushPaintBlendMode(BrushPaint::BlendMode blend_mode) {
  switch (blend_mode) {
    case BrushPaint::BlendMode::kModulate:
    case BrushPaint::BlendMode::kDstIn:
    case BrushPaint::BlendMode::kDstOut:
    case BrushPaint::BlendMode::kSrcAtop:
    case BrushPaint::BlendMode::kSrcIn:
    case BrushPaint::BlendMode::kSrcOver:
    case BrushPaint::BlendMode::kDstOver:
    case BrushPaint::BlendMode::kSrc:
    case BrushPaint::BlendMode::kDst:
    case BrushPaint::BlendMode::kSrcOut:
    case BrushPaint::BlendMode::kDstAtop:
    case BrushPaint::BlendMode::kXor:
      return true;
  }
  return false;
}

bool IsValidBrushPaintSelfOverlap(BrushPaint::SelfOverlap self_overlap) {
  switch (self_overlap) {
    case BrushPaint::SelfOverlap::kAny:
    case BrushPaint::SelfOverlap::kAccumulate:
    case BrushPaint::SelfOverlap::kDiscard:
      return true;
  }
  return false;
}

}  // namespace

absl::Status ValidateBrushPaintTextureLayer(
    const BrushPaint::TextureLayer& layer) {
  if (!IsValidBrushPaintTextureMapping(layer.mapping)) {
    return absl::InvalidArgumentError(absl::StrFormat(
        "`BrushPaint::texture_layers::mapping` holds non-enumerator value %d",
        static_cast<int>(layer.mapping)));
  }
  if (!IsValidBrushPaintTextureOrigin(layer.origin)) {
    return absl::InvalidArgumentError(absl::StrFormat(
        "`BrushPaint::texture_layers::origin` holds non-enumerator value %d",
        static_cast<int>(layer.origin)));
  }
  if (!IsValidBrushPaintTextureSizeUnit(layer.size_unit)) {
    return absl::InvalidArgumentError(absl::StrFormat(
        "`BrushPaint::texture_layers::size_unit` holds non-enumerator value %d",
        static_cast<int>(layer.size_unit)));
  }
  if (!IsValidBrushPaintTextureWrap(layer.wrap_x)) {
    return absl::InvalidArgumentError(
        absl::StrFormat("`BrushPaint::texture_layers::wrap_x` holds "
                        "non-enumerator value %d",
                        static_cast<int>(layer.wrap_x)));
  }
  if (!IsValidBrushPaintTextureWrap(layer.wrap_y)) {
    return absl::InvalidArgumentError(
        absl::StrFormat("`BrushPaint::texture_layers::wrap_y` holds "
                        "non-enumerator value %d",
                        static_cast<int>(layer.wrap_y)));
  }
  if (layer.size.x <= 0.0f || !std::isfinite(layer.size.x) ||
      layer.size.y <= 0.0f || !std::isfinite(layer.size.y)) {
    return absl::InvalidArgumentError(
        absl::StrFormat("`BrushPaint::TextureLayer::size` must be finite and "
                        "greater than zero. Got %v",
                        layer.size));
  }
  if (!std::isfinite(layer.offset.x) || !std::isfinite(layer.offset.y)) {
    return absl::InvalidArgumentError(absl::StrFormat(
        "`BrushPaint::TextureLayer::offset` must be finite. Got %v",
        layer.offset));
  }
  if (!std::isfinite(layer.rotation.ValueInRadians())) {
    return absl::InvalidArgumentError(
        absl::StrFormat("`BrushPaint::TextureLayer::rotation` must be finite. "
                        "Got %v",
                        layer.rotation));
  }
  if (layer.animation_frames <= 0 || layer.animation_frames > (1 << 24)) {
    return absl::InvalidArgumentError(absl::StrCat(
        "`BrushPaint::TextureLayer::animation_frames` must be in "
        "the interval [1, 2^24] (use 1 to disable animation). Got ",
        layer.animation_frames));
  }
  if (layer.animation_rows <= 0 || layer.animation_rows > (1 << 12)) {
    return absl::InvalidArgumentError(absl::StrCat(
        "`BrushPaint::TextureLayer::animation_rows` must be in the interval "
        "[1, 2^12] (use 1 to disable animation). Got ",
        layer.animation_rows));
  }
  if (layer.animation_columns <= 0 || layer.animation_columns > (1 << 12)) {
    return absl::InvalidArgumentError(absl::StrCat(
        "`BrushPaint::TextureLayer::animation_columns` must be in the interval "
        "[1, 2^12] (use 1 to disable animation). Got ",
        layer.animation_columns));
  }
  // Equivalent to `frames > rows * columns` but in a way that avoids overflow.
  if (layer.animation_frames > layer.animation_rows * layer.animation_columns) {
    return absl::InvalidArgumentError(absl::StrFormat(
        "`BrushPaint::TextureLayer::animation_frames` must be "
        "less than or equal to the product of `animation_rows` "
        "and `animation_columns`. Got %d > %d * %d",
        layer.animation_frames, layer.animation_rows, layer.animation_columns));
  }
  if (layer.animation_duration < absl::Milliseconds(1) ||
      layer.animation_duration > absl::Milliseconds(1 << 24) ||
      layer.animation_duration % absl::Milliseconds(1) !=
          absl::ZeroDuration()) {
    return absl::InvalidArgumentError(absl::StrCat(
        "`BrushPaint::TextureLayer::animation_duration` must be "
        "a whole number of milliseconds in the interval [1, 2^24]. Got ",
        layer.animation_duration));
  }
  if (!IsValidBrushPaintBlendMode(layer.blend_mode)) {
    return absl::InvalidArgumentError(
        absl::StrFormat("`BrushPaint::texture_layers::blend_mode` holds "
                        "non-enumerator value %d",
                        static_cast<int>(layer.blend_mode)));
  }
  return absl::OkStatus();
}

absl::Status ValidateBrushPaintTopLevel(const BrushPaint& paint) {
  if (!paint.texture_layers.empty()) {
    int first_animation_frames = paint.texture_layers[0].animation_frames;
    int first_animation_rows = paint.texture_layers[0].animation_rows;
    int first_animation_columns = paint.texture_layers[0].animation_columns;
    absl::Duration first_animation_duration =
        paint.texture_layers[0].animation_duration;
    BrushPaint::TextureMapping first_mapping = paint.texture_layers[0].mapping;
    for (const BrushPaint::TextureLayer& layer : paint.texture_layers) {
      if (layer.animation_frames != first_animation_frames) {
        return absl::InvalidArgumentError(absl::StrCat(
            "`BrushPaint::TextureLayer::animation_frames` must be the same for "
            "all texture layers. Got `",
            first_animation_frames, "` and `", layer.animation_frames, "`"));
      }
      if (layer.animation_rows != first_animation_rows) {
        return absl::InvalidArgumentError(absl::StrCat(
            "`BrushPaint::TextureLayer::animation_rows` must be the same for "
            "all texture layers. Got `",
            first_animation_rows, "` and `", layer.animation_rows, "`"));
      }
      if (layer.animation_columns != first_animation_columns) {
        return absl::InvalidArgumentError(absl::StrCat(
            "`BrushPaint::TextureLayer::animation_columns` must be the same "
            "for all texture layers. Got `",
            first_animation_columns, "` and `", layer.animation_columns, "`"));
      }
      if (layer.animation_duration != first_animation_duration) {
        return absl::InvalidArgumentError(absl::StrCat(
            "`BrushPaint::TextureLayer::animation_duration` must be the same "
            "for all texture layers. Got `",
            first_animation_duration, "` and `", layer.animation_duration,
            "`"));
      }
      // TODO: b/375203215 - Remove the below check once we are able to mix
      // rendering different mapping modes in a single `BrushPaint`.
      if (layer.mapping != first_mapping) {
        return absl::InvalidArgumentError(
            absl::StrCat("`BrushPaint::TextureLayer::mapping` must be the same "
                         "for all texture layers. Got `",
                         first_mapping, "` and `", layer.mapping, "`"));
      }
    }
  }
  if (!IsValidBrushPaintSelfOverlap(paint.self_overlap)) {
    return absl::InvalidArgumentError(
        absl::StrFormat("`BrushPaint::self_overlap` holds "
                        "non-enumerator value %d",
                        static_cast<int>(paint.self_overlap)));
  }
  return absl::OkStatus();
}

absl::Status ValidateBrushPaint(const BrushPaint& paint) {
  for (const BrushPaint::TextureLayer& layer : paint.texture_layers) {
    if (absl::Status status = ValidateBrushPaintTextureLayer(layer);
        !status.ok()) {
      return status;
    }
  }
  for (const ColorFunction& color_function : paint.color_functions) {
    if (absl::Status status = ValidateColorFunction(color_function);
        !status.ok()) {
      return status;
    }
  }
  if (absl::Status status = ValidateBrushPaintTopLevel(paint); !status.ok()) {
    return status;
  }
  return absl::OkStatus();
}

void AddAttributeIdsRequiredByPaint(
    const BrushPaint& paint,
    absl::flat_hash_set<MeshFormat::AttributeId>& attribute_ids) {
  for (const BrushPaint::TextureLayer& layer : paint.texture_layers) {
    if (layer.mapping == BrushPaint::TextureMapping::kStamping) {
      attribute_ids.insert(MeshFormat::AttributeId::kSurfaceUv);
      // This is the only attribute that the paint may end up using, so if we
      // find it then there's no need to check the other layers.
      break;
    }
  }
}

bool AllowsSelfOverlapMode(const BrushPaint& paint,
                           BrushPaint::SelfOverlap self_overlap) {
  return paint.self_overlap == BrushPaint::SelfOverlap::kAny ||
         paint.self_overlap == self_overlap;
}

std::string ToFormattedString(BrushPaint::TextureMapping texture_mapping) {
  switch (texture_mapping) {
    case BrushPaint::TextureMapping::kTiling:
      return "kTiling";
    case BrushPaint::TextureMapping::kStamping:
      return "kStamping";
  }
  return absl::StrCat("TextureMapping(", static_cast<int>(texture_mapping),
                      ")");
}

std::string ToFormattedString(BrushPaint::TextureOrigin texture_origin) {
  switch (texture_origin) {
    case BrushPaint::TextureOrigin::kStrokeSpaceOrigin:
      return "kStrokeSpaceOrigin";
    case BrushPaint::TextureOrigin::kFirstStrokeInput:
      return "kFirstStrokeInput";
    case BrushPaint::TextureOrigin::kLastStrokeInput:
      return "kLastStrokeInput";
  }
  return absl::StrCat("TextureOrigin(", static_cast<int>(texture_origin), ")");
}

std::string ToFormattedString(BrushPaint::TextureSizeUnit texture_size_unit) {
  switch (texture_size_unit) {
    case BrushPaint::TextureSizeUnit::kBrushSize:
      return "kBrushSize";
    case BrushPaint::TextureSizeUnit::kStrokeCoordinates:
      return "kStrokeCoordinates";
  }
  return absl::StrCat("TextureSizeUnit(", static_cast<int>(texture_size_unit),
                      ")");
}

std::string ToFormattedString(BrushPaint::TextureWrap texture_wrap) {
  switch (texture_wrap) {
    case BrushPaint::TextureWrap::kRepeat:
      return "kRepeat";
    case BrushPaint::TextureWrap::kMirror:
      return "kMirror";
    case BrushPaint::TextureWrap::kClamp:
      return "kClamp";
  }
  return absl::StrCat("TextureWrap(", static_cast<int>(texture_wrap), ")");
}

std::string ToFormattedString(BrushPaint::BlendMode blend_mode) {
  switch (blend_mode) {
    case BrushPaint::BlendMode::kModulate:
      return "kModulate";
    case BrushPaint::BlendMode::kDstIn:
      return "kDstIn";
    case BrushPaint::BlendMode::kDstOut:
      return "kDstOut";
    case BrushPaint::BlendMode::kSrcAtop:
      return "kSrcAtop";
    case BrushPaint::BlendMode::kSrcIn:
      return "kSrcIn";
    case BrushPaint::BlendMode::kSrcOver:
      return "kSrcOver";
    case BrushPaint::BlendMode::kDstOver:
      return "kDstOver";
    case BrushPaint::BlendMode::kSrc:
      return "kSrc";
    case BrushPaint::BlendMode::kDst:
      return "kDst";
    case BrushPaint::BlendMode::kSrcOut:
      return "kSrcOut";
    case BrushPaint::BlendMode::kDstAtop:
      return "kDstAtop";
    case BrushPaint::BlendMode::kXor:
      return "kXor";
  }
  return absl::StrCat("BlendMode(", static_cast<int>(blend_mode), ")");
}

std::string ToFormattedString(const BrushPaint::TextureLayer& texture_layer) {
  return absl::StrCat(
      "TextureLayer{", "client_texture_id=", texture_layer.client_texture_id,
      ", mapping=", ToFormattedString(texture_layer.mapping),
      ", origin=", ToFormattedString(texture_layer.origin),
      ", size_unit=", ToFormattedString(texture_layer.size_unit),
      ", wrap_x=", ToFormattedString(texture_layer.wrap_x),
      ", wrap_y=", ToFormattedString(texture_layer.wrap_y),
      ", size=", texture_layer.size, ", offset=", texture_layer.offset,
      ", rotation=", texture_layer.rotation,
      ", animation_frames=", texture_layer.animation_frames,
      ", animation_rows=", texture_layer.animation_rows,
      ", animation_columns=", texture_layer.animation_columns,
      ", animation_duration=", texture_layer.animation_duration,
      ", blend_mode=", ToFormattedString(texture_layer.blend_mode), "}");
}

std::string ToFormattedString(const BrushPaint::SelfOverlap self_overlap) {
  switch (self_overlap) {
    case BrushPaint::SelfOverlap::kAny:
      return "kAny";
    case BrushPaint::SelfOverlap::kDiscard:
      return "kDiscard";
    case BrushPaint::SelfOverlap::kAccumulate:
      return "kAccumulate";
  }
  return absl::StrCat("SelfOverlap(", static_cast<int>(self_overlap), ")");
}

std::string ToFormattedString(const BrushPaint& paint) {
  std::string formatted = "BrushPaint{";
  if (!paint.texture_layers.empty()) {
    absl::StrAppend(&formatted, "texture_layers={",
                    absl::StrJoin(paint.texture_layers, ", "), "}");
  }
  if (!paint.color_functions.empty()) {
    if (!paint.texture_layers.empty()) {
      absl::StrAppend(&formatted, ", ");
    }
    absl::StrAppend(&formatted, "color_functions={",
                    absl::StrJoin(paint.color_functions, ", "), "}");
  }
  if (!paint.texture_layers.empty() || !paint.color_functions.empty()) {
    absl::StrAppend(&formatted, ", ");
  }
  absl::StrAppend(&formatted,
                  "self_overlap=", ToFormattedString(paint.self_overlap), "}");
  return formatted;
}

}  // namespace ink::brush_internal
