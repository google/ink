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

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <variant>

#include "absl/container/flat_hash_set.h"
#include "absl/functional/overload.h"
#include "absl/status/status.h"
#include "absl/status/status_macros.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "absl/time/time.h"
#include "ink/brush/color_function.h"
#include "ink/brush/version.h"
#include "ink/geometry/mesh_format.h"

namespace ink {

uint32_t BrushPaint::MaxTextureLayers() {
  // This value was chosen somewhat arbitrarily. Some of our renderer
  // implementions need to declare the number of textures they will be sampling
  // from, so we need some limit. We can always raise this limit in the future,
  // but lowering it will be harder once clients start relying on being able to
  // have a certain number of texture layers. So for now, the limit is fairly
  // conservative.
  return 4;
}

namespace brush_internal {
namespace {

bool IsValidBrushPaintTextureOrigin(BrushPaint::TextureOrigin origin) {
  switch (origin) {
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

absl::Status ValidateTextureLayer(const BrushPaint::TilingTexture& layer) {
  if (!IsValidBrushPaintTextureOrigin(layer.origin)) {
    return absl::InvalidArgumentError(absl::StrFormat(
        "`BrushPaint::TilingTexture::origin` holds non-enumerator value %d",
        static_cast<int>(layer.origin)));
  }
  if (!IsValidBrushPaintTextureSizeUnit(layer.size_unit)) {
    return absl::InvalidArgumentError(absl::StrFormat(
        "`BrushPaint::TilingTexture::size_unit` holds non-enumerator value %d",
        static_cast<int>(layer.size_unit)));
  }
  if (!IsValidBrushPaintTextureWrap(layer.wrap_x)) {
    return absl::InvalidArgumentError(
        absl::StrFormat("`BrushPaint::TilingTexture::wrap_x` holds "
                        "non-enumerator value %d",
                        static_cast<int>(layer.wrap_x)));
  }
  if (!IsValidBrushPaintTextureWrap(layer.wrap_y)) {
    return absl::InvalidArgumentError(
        absl::StrFormat("`BrushPaint::TilingTexture::wrap_y` holds "
                        "non-enumerator value %d",
                        static_cast<int>(layer.wrap_y)));
  }
  if (layer.size.x <= 0.0f || !std::isfinite(layer.size.x) ||
      layer.size.y <= 0.0f || !std::isfinite(layer.size.y)) {
    return absl::InvalidArgumentError(
        absl::StrFormat("`BrushPaint::TilingTexture::size` must be finite and "
                        "greater than zero. Got %v",
                        layer.size));
  }
  if (!std::isfinite(layer.offset.x) || !std::isfinite(layer.offset.y)) {
    return absl::InvalidArgumentError(absl::StrFormat(
        "`BrushPaint::TilingTexture::offset` must be finite. Got %v",
        layer.offset));
  }
  if (!std::isfinite(layer.rotation.ValueInRadians())) {
    return absl::InvalidArgumentError(
        absl::StrFormat("`BrushPaint::TilingTexture::rotation` must be finite. "
                        "Got %v",
                        layer.rotation));
  }
  if (!IsValidBrushPaintBlendMode(layer.blend_mode)) {
    return absl::InvalidArgumentError(
        absl::StrFormat("`BrushPaint::TilingTexture::blend_mode` holds "
                        "non-enumerator value %d",
                        static_cast<int>(layer.blend_mode)));
  }
  return absl::OkStatus();
}

absl::Status ValidateTextureLayer(const BrushPaint::StampingTexture& layer) {
  if (layer.animation_frames <= 0 || layer.animation_frames > (1 << 24)) {
    return absl::InvalidArgumentError(absl::StrCat(
        "`BrushPaint::StampingTexture::animation_frames` must be in "
        "the interval [1, 2^24] (use 1 to disable animation). Got ",
        layer.animation_frames));
  }
  if (layer.animation_rows <= 0 || layer.animation_rows > (1 << 12)) {
    return absl::InvalidArgumentError(absl::StrCat(
        "`BrushPaint::StampingTexture::animation_rows` must be in the interval "
        "[1, 2^12] (use 1 to disable animation). Got ",
        layer.animation_rows));
  }
  if (layer.animation_columns <= 0 || layer.animation_columns > (1 << 12)) {
    return absl::InvalidArgumentError(absl::StrCat(
        "`BrushPaint::StampingTexture::animation_columns` must be in the "
        "interval [1, 2^12] (use 1 to disable animation). Got ",
        layer.animation_columns));
  }
  // Equivalent to `frames > rows * columns` but in a way that avoids overflow.
  if (layer.animation_frames > layer.animation_rows * layer.animation_columns) {
    return absl::InvalidArgumentError(absl::StrFormat(
        "`BrushPaint::StampingTexture::animation_frames` must be "
        "less than or equal to the product of `animation_rows` "
        "and `animation_columns`. Got %d > %d * %d",
        layer.animation_frames, layer.animation_rows, layer.animation_columns));
  }
  if (layer.animation_duration < absl::ZeroDuration() ||
      layer.animation_duration > absl::Milliseconds(1 << 24) ||
      layer.animation_duration % absl::Milliseconds(1) !=
          absl::ZeroDuration()) {
    return absl::InvalidArgumentError(absl::StrCat(
        "`BrushPaint::StampingTexture::animation_duration` must be "
        "a whole number of milliseconds in the interval [0, 2^24]. Got ",
        layer.animation_duration));
  }
  if (!IsValidBrushPaintBlendMode(layer.blend_mode)) {
    return absl::InvalidArgumentError(
        absl::StrFormat("`BrushPaint::StampingTexture::blend_mode` holds "
                        "non-enumerator value %d",
                        static_cast<int>(layer.blend_mode)));
  }
  return absl::OkStatus();
}

absl::Status ValidateAdjacentTextureLayers(
    const BrushPaint::TextureLayer& layer1,
    const BrushPaint::TextureLayer& layer2) {
  return std::visit(
      absl::Overload(
          [&layer2](const BrushPaint::TilingTexture& tiling1) {
            if (!std::holds_alternative<BrushPaint::TilingTexture>(layer2)) {
              return absl::InvalidArgumentError(
                  "All texture layers must use the same mapping "
                  "mode.");
            }
            return absl::OkStatus();
          },
          [&layer2](const BrushPaint::StampingTexture& stamping1) {
            if (!std::holds_alternative<BrushPaint::StampingTexture>(layer2)) {
              return absl::InvalidArgumentError(
                  "All texture layers must use the same mapping "
                  "mode.");
            }
            const BrushPaint::StampingTexture& stamping2 =
                std::get<BrushPaint::StampingTexture>(layer2);
            if (stamping2.animation_frames != stamping1.animation_frames) {
              return absl::InvalidArgumentError(
                  absl::StrCat("`BrushPaint::StampingTexture::animation_"
                               "frames` must be the same for "
                               "all texture layers. Got `",
                               stamping1.animation_frames, "` and `",
                               stamping2.animation_frames, "`"));
            }
            if (stamping2.animation_rows != stamping1.animation_rows) {
              return absl::InvalidArgumentError(
                  absl::StrCat("`BrushPaint::StampingTexture::animation_"
                               "rows` must be the same for "
                               "all texture layers. Got `",
                               stamping1.animation_rows, "` and `",
                               stamping2.animation_rows, "`"));
            }
            if (stamping2.animation_columns != stamping1.animation_columns) {
              return absl::InvalidArgumentError(
                  absl::StrCat("`BrushPaint::StampingTexture::animation_"
                               "columns` must be the same for "
                               "all texture layers. Got `",
                               stamping1.animation_columns, "` and `",
                               stamping2.animation_columns, "`"));
            }
            if (stamping2.animation_duration != stamping1.animation_duration) {
              return absl::InvalidArgumentError(
                  absl::StrCat("`BrushPaint::StampingTexture::animation_"
                               "duration` must be the same for "
                               "all texture layers. Got `",
                               stamping1.animation_duration, "` and `",
                               stamping2.animation_duration, "`"));
            }
            return absl::OkStatus();
          }),
      layer1);
}

}  // namespace

absl::Status ValidateBrushPaintTextureLayer(
    const BrushPaint::TextureLayer& layer) {
  return std::visit(
      [](const auto& layer) { return ValidateTextureLayer(layer); }, layer);
}

absl::Status ValidateBrushPaintTopLevel(const BrushPaint& paint) {
  const BrushPaint::TextureLayer* previous = nullptr;
  for (const BrushPaint::TextureLayer& layer : paint.texture_layers) {
    if (previous) {
      ABSL_RETURN_IF_ERROR(ValidateAdjacentTextureLayers(*previous, layer));
    }
    previous = &layer;
  }

  if (paint.texture_layers.size() > BrushPaint::MaxTextureLayers()) {
    return absl::InvalidArgumentError(
        absl::StrCat("`BrushPaint` is currently limited to at most ",
                     BrushPaint::MaxTextureLayers(),
                     " `TextureLayer`s, but got ", paint.texture_layers.size(),
                     ". (This limit may be increased in the future.)"));
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
    ABSL_RETURN_IF_ERROR(ValidateBrushPaintTextureLayer(layer));
  }
  for (const ColorFunction& color_function : paint.color_functions) {
    ABSL_RETURN_IF_ERROR(ValidateColorFunction(color_function));
  }
  ABSL_RETURN_IF_ERROR(ValidateBrushPaintTopLevel(paint));
  return absl::OkStatus();
}

namespace {

Version CalculateMinimumRequiredVersion(
    BrushPaint::TextureOrigin texture_origin) {
  switch (texture_origin) {
    case BrushPaint::TextureOrigin::kStrokeSpaceOrigin:
    case BrushPaint::TextureOrigin::kFirstStrokeInput:
    case BrushPaint::TextureOrigin::kLastStrokeInput:
      return Version::k0Jetpack1_0_0();
  }
}

Version CalculateMinimumRequiredVersion(
    BrushPaint::TextureSizeUnit texture_size_unit) {
  switch (texture_size_unit) {
    case BrushPaint::TextureSizeUnit::kBrushSize:
    case BrushPaint::TextureSizeUnit::kStrokeCoordinates:
      return Version::k0Jetpack1_0_0();
  }
}

Version CalculateMinimumRequiredVersion(BrushPaint::TextureWrap texture_wrap) {
  switch (texture_wrap) {
    case BrushPaint::TextureWrap::kRepeat:
    case BrushPaint::TextureWrap::kMirror:
    case BrushPaint::TextureWrap::kClamp:
      return Version::k0Jetpack1_0_0();
  }
}

Version CalculateMinimumRequiredVersion(BrushPaint::BlendMode blend_mode) {
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
      return Version::k0Jetpack1_0_0();
  }
}

Version CalculateMinimumRequiredVersion(BrushPaint::SelfOverlap self_overlap) {
  switch (self_overlap) {
    case BrushPaint::SelfOverlap::kAny:
    case BrushPaint::SelfOverlap::kDiscard:
    case BrushPaint::SelfOverlap::kAccumulate:
      return Version::k0Jetpack1_0_0();
  }
}

Version CalculateMinimumRequiredVersion(
    const BrushPaint::TilingTexture& layer) {
  return std::max({CalculateMinimumRequiredVersion(layer.origin),
                   CalculateMinimumRequiredVersion(layer.size_unit),
                   CalculateMinimumRequiredVersion(layer.wrap_x),
                   CalculateMinimumRequiredVersion(layer.wrap_y),
                   CalculateMinimumRequiredVersion(layer.blend_mode)});
}

Version CalculateMinimumRequiredVersion(
    const BrushPaint::StampingTexture& layer) {
  // LINT.IfChange(animation_defaults)
  if (layer.animation_frames != 1 || layer.animation_rows != 1 ||
      layer.animation_columns != 1 ||
      layer.animation_duration != absl::Milliseconds(1000)) {
    return Version::kDevelopment();
  }
  // LINT.ThenChange(../storage/proto/brush_family.proto:animation_defaults)
  return CalculateMinimumRequiredVersion(layer.blend_mode);
}

Version CalculateMinimumRequiredVersion(const BrushPaint::TextureLayer& layer) {
  return std::visit(
      [](const auto& layer) { return CalculateMinimumRequiredVersion(layer); },
      layer);
}

}  // namespace

Version CalculateMinimumRequiredVersion(const BrushPaint& paint) {
  Version max_version = CalculateMinimumRequiredVersion(paint.self_overlap);
  for (const BrushPaint::TextureLayer& layer : paint.texture_layers) {
    max_version = std::max(max_version, CalculateMinimumRequiredVersion(layer));
  }
  for (const ColorFunction& color_function : paint.color_functions) {
    max_version =
        std::max(max_version, CalculateMinimumRequiredVersion(color_function));
  }
  return max_version;
}

void AddAttributeIdsRequiredByPaint(
    const BrushPaint& paint,
    absl::flat_hash_set<MeshFormat::AttributeId>& attribute_ids) {
  for (const BrushPaint::TextureLayer& layer : paint.texture_layers) {
    if (std::holds_alternative<BrushPaint::StampingTexture>(layer)) {
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

std::string ToFormattedString(const BrushPaint::TilingTexture& layer) {
  return absl::StrCat(
      "TilingTexture{", "client_texture_id=", layer.client_texture_id,
      ", origin=", ToFormattedString(layer.origin),
      ", size_unit=", ToFormattedString(layer.size_unit),
      ", wrap_x=", ToFormattedString(layer.wrap_x),
      ", wrap_y=", ToFormattedString(layer.wrap_y), ", size=", layer.size,
      ", offset=", layer.offset, ", rotation=", layer.rotation,
      ", blend_mode=", ToFormattedString(layer.blend_mode), "}");
}

std::string ToFormattedString(const BrushPaint::StampingTexture& layer) {
  return absl::StrCat(
      "StampingTexture{", "client_texture_id=", layer.client_texture_id,
      ", animation_frames=", layer.animation_frames,
      ", animation_rows=", layer.animation_rows,
      ", animation_columns=", layer.animation_columns,
      ", animation_duration=", layer.animation_duration,
      ", blend_mode=", ToFormattedString(layer.blend_mode), "}");
}

std::string ToFormattedString(const BrushPaint::TextureLayer& layer) {
  return std::visit([](const auto& layer) { return ToFormattedString(layer); },
                    layer);
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

}  // namespace brush_internal
}  // namespace ink
