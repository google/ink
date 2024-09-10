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

#ifndef INK_STROKES_BRUSH_BRUSH_FAMILY_H_
#define INK_STROKES_BRUSH_BRUSH_FAMILY_H_

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "ink/brush/brush_coat.h"
#include "ink/brush/brush_paint.h"
#include "ink/brush/brush_tip.h"
#include "ink/types/uri.h"

namespace ink {

// A `BrushFamily` combines one or more `BrushCoat`s and an optional URI to
// describe a family of brushes.
//
// The URI exists for the convenience of higher level serialization and asset
// management APIs. Aside from being checked for valid formatting and being
// forwarded on copy or move, the URI will not*** be used by Ink APIs that
// consume a `BrushFamily`.
//
// *** During the implementation phase of `BrushTip` and `BrushPaint`, the URI
// will be inspected by stroke mesh creation to enable a set of stock Ink
// brushes. This behavior will be replaced when a helper library is able to fill
// out the specification of the stock brushes using the new tip and paint types.
class BrushFamily {
 public:
  // The legacy spring-based input modeler, provided for backwards compatibility
  // with existing Ink clients.
  struct SpringModelV1 {};

  // The newer (and recommended) spring-based input modeler.
  struct SpringModelV2 {};

  // Specifies a model for turning a sequence of raw hardware inputs (e.g. from
  // a stylus, touchscreen, or mouse) into a sequence of smoothed, modeled
  // inputs. Raw hardware inputs tend to be noisy, and must be smoothed before
  // being passed into a brush's behaviors and extruded into a mesh in order to
  // get a good-looking stroke.
  using InputModel = std::variant<SpringModelV1, SpringModelV2>;

  // Returns the default `InputModel` that will be used by
  // `BrushFamily::Create()` when none is specified.
  static InputModel DefaultInputModel();

  // Returns the maximum number of `BrushCoat`s that a `BrushFamily` is allowed
  // to have. Note that this limit may increase in the future.
  static uint32_t MaxBrushCoats();

  // Creates a `BrushFamily` with the given `tips`, `paint`, and an optional
  // `uri`.
  //
  // Performs validation of the `tips`, `paint`, and the `uri` if present.
  // Returns an error if argument validation fails.
  //
  // For a tip to be valid the following must hold:
  //   * Every numeric, `Angle` or `Duration32` property must be finite and
  //     within valid bounds that may be specified in `BrushTip`,
  //     `BrushBehavior`, and `EasingFunction`.
  //   * Every enum property must be equal to one of the named enumerators for
  //     that property's type.
  //
  // For a brush paint to be valid the following must hold:
  //  * For each texture_layer the following must hold:
  //    - The color texture URI must be a texture asset type.
  //    - size components must be finite and greater than 0.
  //    - offset components must be in interval [0, 1].
  //    - rotation component must be finite.
  //    - size_jitter components must be smaller than or equal to their size
  //      counterparts.
  //    - offset_jitter must be in interval [0, 1].
  //    - rotation jitter must be a finite.
  //    - opacity must be in interval [0, 1].
  //    - For each TextureKeyframe the following must hold:
  //      ~ progress has to be in interval [0, 1].
  //      ~ size components, if present, must be finite and greater than 0.
  //      ~ offset components, if present, must be in interval [0, 1].
  //      ~ rotation, if present, must finite.
  //      ~ opacity, if present, must be in interval [0, 1].
  //  * Every enum property must be equal to one of the named enumerators for
  //    that property's type.
  //
  // For a non-nullopt URI to be valid the following must hold:
  //   * The asset type must be "brush-family".
  //
  // If the `BrushFamily` has a URI set, other Ink code will implicitly assume
  // that (1) the tip/paint data is canonical for that URI, and (2) the client
  // application has a `BrushProvider` implementation that can map from that URI
  // to the canonical tip/paint data. In particular, when serializing brush data
  // to proto, if a URI is set, then only the URI will be stored, and the
  // tip/paint data will be omitted.
  static absl::StatusOr<BrushFamily> Create(
      const BrushTip& tip, const BrushPaint& paint,
      std::optional<Uri> uri = std::nullopt,
      const InputModel& input_model = DefaultInputModel());
  static absl::StatusOr<BrushFamily> Create(
      absl::Span<const BrushCoat> coats, std::optional<Uri> uri = std::nullopt,
      const InputModel& input_model = DefaultInputModel());
  // Same as above, except that the URI is parsed from the given URI string (and
  // an error is returned if the URI string fails to parse). An empty URI string
  // is treated as a nullopt URI.
  static absl::StatusOr<BrushFamily> Create(
      const BrushTip& tip, const BrushPaint& paint,
      absl::string_view uri_string,
      const InputModel& input_model = DefaultInputModel());
  static absl::StatusOr<BrushFamily> Create(
      absl::Span<const BrushCoat> coats, absl::string_view uri_string,
      const InputModel& input_model = DefaultInputModel());

  // Constructs a brush-family with default tip and paint, and no URI.
  BrushFamily() = default;

  BrushFamily(const BrushFamily&) = default;
  BrushFamily(BrushFamily&&) = default;
  BrushFamily& operator=(const BrushFamily&) = default;
  BrushFamily& operator=(BrushFamily&&) = default;

  absl::Span<const BrushCoat> GetCoats() const;
  const std::optional<Uri>& GetUri() const;
  const InputModel& GetInputModel() const;

  template <typename Sink>
  friend void AbslStringify(Sink& sink, const BrushFamily& family) {
    sink.Append(family.ToFormattedString());
  }

 private:
  BrushFamily(std::vector<BrushCoat> coats, std::optional<Uri> uri,
              const InputModel& input_model);

  // Implementation helper for AbslStringify.
  std::string ToFormattedString() const;

  std::vector<BrushCoat> coats_ = {BrushCoat{.tips = {BrushTip{}}}};
  std::optional<Uri> uri_;
  InputModel input_model_;
};

// ---------------------------------------------------------------------------
//                     Implementation details below

inline absl::Span<const BrushCoat> BrushFamily::GetCoats() const {
  return coats_;
}

inline const std::optional<Uri>& BrushFamily::GetUri() const { return uri_; }

inline const BrushFamily::InputModel& BrushFamily::GetInputModel() const {
  return input_model_;
}

}  // namespace ink

#endif  // INK_STROKES_BRUSH_BRUSH_FAMILY_H_