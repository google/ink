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

#include "ink/brush/brush_family.h"

#include <algorithm>
#include <cstdint>
#include <numeric>
#include <string>
#include <variant>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/status_macros.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/string_view.h"
#include "absl/time/time.h"
#include "absl/types/span.h"
#include "ink/brush/brush_coat.h"
#include "ink/brush/brush_paint.h"
#include "ink/brush/brush_tip.h"
#include "ink/brush/version.h"
#include "ink/types/duration.h"

namespace ink {

BrushFamily::InputModel BrushFamily::DefaultInputModel() {
  return SlidingWindowModel{};
}

uint32_t BrushFamily::MaxBrushCoats() {
  // This value was chosen somewhat arbitrarily. A `PartitionedMesh` can't have
  // more than 2^16 meshes, and each coat creates at least one mesh, so we need
  // *some* limit. We can always raise this limit in the future, but lowering it
  // will be harder once clients start relying on being able to have a certain
  // number of coats. So for now, the limit is fairly conservative.
  return 10;
}

BrushFamily::BrushFamily(absl::Span<const BrushCoat> coats,
                         const InputModel& input_model,
                         const Metadata& metadata,
                         absl::Duration texture_animation_loop_duration)
    : coats_(coats.begin(), coats.end()),
      input_model_(input_model),
      metadata_(metadata),
      texture_animation_loop_duration_(texture_animation_loop_duration) {}

absl::StatusOr<BrushFamily> BrushFamily::Create(const BrushTip& tip,
                                                const BrushPaint& paint,
                                                const InputModel& input_model,
                                                const Metadata& metadata) {
  BrushCoat coat = {.tip = tip, .paint_preferences = {paint}};
  return BrushFamily::Create(absl::MakeConstSpan(&coat, 1), input_model,
                             metadata);
}

absl::StatusOr<BrushFamily> BrushFamily::Create(
    absl::Span<const BrushCoat> coats, const InputModel& input_model,
    const Metadata& metadata) {
  if (coats.size() > MaxBrushCoats()) {
    return absl::InvalidArgumentError(
        absl::StrCat("`BrushFamily` is currently limited to at most ",
                     MaxBrushCoats(), " `BrushCoat`s, but got ", coats.size(),
                     ". (This limit may be increased in the future.)"));
  }
  for (const BrushCoat& coat : coats) {
    ABSL_RETURN_IF_ERROR(brush_internal::ValidateBrushCoat(coat));
  }
  ABSL_RETURN_IF_ERROR(brush_internal::ValidateInputModel(input_model));

  int64_t full_duration_ms = 0;
  for (const BrushCoat& coat : coats) {
    // Only one paint preference for a given coat will be in use at a time, but
    // we can't know ahead of time which one, so we take the LCM across all of
    // them.  This is OK, because (1) even if the LCM turns out to be larger
    // than necessary, it will still form a closed animation loop, and (2) in
    // most practical cases, each paint preferences for a given coat will
    // probably either have the same animation duration, or be a non-animated
    // fallback.
    for (const BrushPaint& paint : coat.paint_preferences) {
      for (const BrushPaint::TextureLayer& texture_layer :
           paint.texture_layers) {
        if (const auto* stamping_texture =
                std::get_if<BrushPaint::StampingTexture>(&texture_layer)) {
          if (stamping_texture->animation_duration == absl::ZeroDuration() ||
              stamping_texture->animation_frames == 1) {
            continue;  // This texture is not animated.
          }
          // Because we've already validated each `BrushCoat`, we know that each
          // texture's `animation_duration` is a whole number of milliseconds,
          // so `ToInt64Milliseconds` isn't losing any precision here.
          int64_t texture_duration_ms =
              absl::ToInt64Milliseconds(stamping_texture->animation_duration);
          if (full_duration_ms == 0) {
            full_duration_ms = texture_duration_ms;
            continue;
          }
          // Because we've already validated each `BrushCoat`, we know that
          // `texture_duration_ms` is at most (1 << 24), and similarly we know
          // that `full_duration_ms` is at most (1 << 24) so far, so their
          // product is at most (1 << 48), and therefore this 64-bit `std::lcm`
          // call can't overflow.
          full_duration_ms = std::lcm(full_duration_ms, texture_duration_ms);
          if (full_duration_ms > (1 << 24)) {
            return absl::InvalidArgumentError(
                "The LCM of all texture animation durations in a `BrushFamily` "
                "must be no more than 2^24 milliseconds");
          }
        }
      }
    }
  }

  return BrushFamily(coats, input_model, metadata,
                     absl::Milliseconds(full_duration_ms));
}

std::string BrushFamily::ToFormattedString() const {
  std::string formatted =
      absl::StrCat("BrushFamily(coats=[", absl::StrJoin(coats_, ", "),
                   "], input_model=", input_model_);
  if (!metadata_.client_brush_family_id.empty()) {
    absl::StrAppend(&formatted, ", client_brush_family_id='",
                    metadata_.client_brush_family_id, "'");
  }
  absl::StrAppend(&formatted, ")");
  return formatted;
}

Version BrushFamily::CalculateMinimumRequiredVersion() const {
  Version max_version = Version::k0Jetpack1_0_0();
  for (const auto& coat : coats_) {
    max_version = std::max(
        max_version, brush_internal::CalculateMinimumRequiredVersion(coat));
  }
  return max_version;
}

bool BrushFamily::HasFallbacks() const {
  return !opaque_decoded_proto_bytes_with_fallbacks_.empty();
}

namespace brush_internal {
namespace {

absl::Status ValidateInputModel(const BrushFamily::PassthroughModel& model) {
  return absl::OkStatus();
}

absl::Status ValidateInputModel(const BrushFamily::SlidingWindowModel& model) {
  if (!model.window_size.IsFinite() ||
      model.window_size <= Duration32::Zero()) {
    return absl::InvalidArgumentError(absl::StrCat(
        "`SlidingWindowModel::window_size` must be finite and positive. Got: ",
        model.window_size));
  }
  if (model.upsampling_period <= Duration32::Zero()) {
    return absl::InvalidArgumentError(absl::StrCat(
        "`SlidingWindowModel::upsampling_period` must be positive. Got: ",
        model.upsampling_period));
  }
  return absl::OkStatus();
}

std::string ToFormattedString(const BrushFamily::PassthroughModel& model) {
  return "PassthroughModel";
}

std::string ToFormattedString(const BrushFamily::SlidingWindowModel& model) {
  return absl::StrCat("SlidingWindowModel(window_size=", model.window_size,
                      ", upsampling_period=", model.upsampling_period, ")");
}

}  // namespace

absl::Status ValidateInputModel(const BrushFamily::InputModel& model) {
  return std::visit([](const auto& model) { return ValidateInputModel(model); },
                    model);
}

std::string ToFormattedString(const BrushFamily::InputModel& model) {
  return std::visit([](const auto& model) { return ToFormattedString(model); },
                    model);
}

}  // namespace brush_internal
}  // namespace ink
