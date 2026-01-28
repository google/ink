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
#include <string>
#include <variant>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "ink/brush/brush_coat.h"
#include "ink/brush/brush_paint.h"
#include "ink/brush/brush_tip.h"
#include "ink/types/duration.h"

namespace ink {

// A `BrushFamily` combines one or more `BrushCoat`s with an optional
// client-specified string ID, which exists for the convenience of higher level
// serialization and asset management APIs.
class BrushFamily {
 public:
  // LINT.IfChange(input_model_types)

  // Spring-based input modeler. Stored in the `InputModel` variant below to
  // allow future input models to be added without changing the shape of
  // existing strokes.
  struct SpringModel {};

  // A naive model that passes through raw inputs mostly unchanged.  This is an
  // experimental configuration which may be adjusted or removed later.
  struct ExperimentalNaiveModel {};

  // Averages nearby inputs together within a sliding time window. To be valid,
  // the window size must be finite and strictly positive, and the upsampling
  // period must be strictly positive (but may be infinte, to completely disable
  // upsampling). This is an experimental configuration which may be adjusted or
  // removed later.
  struct SlidingWindowModel {
    // The duration over which to average together nearby raw inputs. Typically
    // this should be somewhere in the 1 ms to 100 ms range.
    Duration32 window_size = Duration32::Millis(20);
    // The maximum duration between modeled inputs; if raw inputs are spaced
    // more than this far apart in time, then additional modeled inputs will be
    // inserted between them. Set this to `Duration32::Infinite()` to disable
    // upsampling.
    Duration32 upsampling_period = Duration32::Seconds(1.0 / 180.0);
  };

  // Specifies a model for turning a sequence of raw hardware inputs (e.g. from
  // a stylus, touchscreen, or mouse) into a sequence of smoothed, modeled
  // inputs. Raw hardware inputs tend to be noisy, and must be smoothed before
  // being passed into a brush's behaviors and extruded into a mesh in order to
  // get a good-looking stroke.
  using InputModel =
      std::variant<SpringModel, ExperimentalNaiveModel, SlidingWindowModel>;

  // LINT.ThenChange(../strokes/internal/stroke_input_modeler_test.cc:input_model_types)

  // Metadata for a `BrushFamily` that can be used by client apps or developers,
  // but is not used internally by Ink.
  //
  // All metadata strings should be UTF-8.
  struct Metadata {
    // An optional ID string for this brush family, specified by (and meaningful
    // to) the client that originally created it. For example, this could be a
    // URI, or UUID, or a simple word. No particular structure or meaning should
    // be ascribed to this string by clients for brushes not created by that
    // client.
    std::string client_brush_family_id;
    // A multi-line, human-readable string with a description of the brush and
    // how it works, with the intended audience being designers/developers who
    // are editing the brush definition. This string is not generally intended
    // to be displayed to end users.
    std::string developer_comment;

    bool operator==(const Metadata&) const = default;
  };

  // Returns the default `InputModel` that will be used by
  // `BrushFamily::Create()` when none is specified.
  static InputModel DefaultInputModel();

  // Returns the maximum number of `BrushCoat`s that a `BrushFamily` is allowed
  // to have. Note that this limit may increase in the future.
  static uint32_t MaxBrushCoats();

  // Creates a `BrushFamily` with the given `tip`, `paint`, and optional
  // string `client_brush_family_id`.
  //
  // Performs validation of the `tip` and `paint`.
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
  //    - size components must be finite and greater than 0.
  //    - offset components must be in interval [0, 1].
  //    - rotation component must be finite.
  //    - size_jitter components must be smaller than or equal to their size
  //      counterparts.
  //    - offset_jitter must be in interval [0, 1].
  //    - rotation_jitter must be finite.
  //    - opacity must be in interval [0, 1].
  //    - animation_frames must be greater than 0.
  //    - For each TextureKeyframe the following must hold:
  //      ~ progress has to be in interval [0, 1].
  //      ~ size components, if present, must be finite and greater than 0.
  //      ~ offset components, if present, must be in interval [0, 1].
  //      ~ rotation, if present, must finite.
  //      ~ opacity, if present, must be in interval [0, 1].
  //  * All texture layers must have the same animation_frames value.
  //  * For now, all texture layers must use the same `TextureMapping` value.
  //    TODO: b/375203215 - Relax this requirement once we are able to mix
  //    rendering tiling and winding textures in a single `BrushPaint`.
  //  * Every enum property must be equal to one of the named enumerators for
  //    that property's type.
  static absl::StatusOr<BrushFamily> Create(
      const BrushTip& tip, const BrushPaint& paint,
      const InputModel& input_model = DefaultInputModel(),
      const Metadata& metadata = Metadata{});
  static absl::StatusOr<BrushFamily> Create(
      absl::Span<const BrushCoat> coats,
      const InputModel& input_model = DefaultInputModel(),
      const Metadata& metadata = Metadata{});

  // Constructs a brush-family with default tip and paint, default input model,
  // and empty metadata.
  BrushFamily() = default;

  BrushFamily(const BrushFamily&) = default;
  BrushFamily(BrushFamily&&) = default;
  BrushFamily& operator=(const BrushFamily&) = default;
  BrushFamily& operator=(BrushFamily&&) = default;

  absl::Span<const BrushCoat> GetCoats() const;
  const InputModel& GetInputModel() const;

  // Returns the metadata fields for this brush family. This metadata is
  // considered when comparing `BrushFamily` objects for equality, but is not
  // otherwise used internally by Ink.
  const Metadata& GetMetadata() const;

  template <typename Sink>
  friend void AbslStringify(Sink& sink, const BrushFamily& family) {
    sink.Append(family.ToFormattedString());
  }

 private:
  BrushFamily(absl::Span<const BrushCoat> coats, const InputModel& input_model,
              const Metadata& metadata);

  // Implementation helper for AbslStringify.
  std::string ToFormattedString() const;

  std::vector<BrushCoat> coats_ = {BrushCoat{.tip = BrushTip{}}};
  InputModel input_model_ = DefaultInputModel();
  Metadata metadata_;
};

namespace brush_internal {

absl::Status ValidateInputModel(const BrushFamily::InputModel& model);

std::string ToFormattedString(const BrushFamily::InputModel& model);

}  // namespace brush_internal

template <typename Sink>
void AbslStringify(Sink& sink, const BrushFamily::InputModel& model) {
  sink.Append(brush_internal::ToFormattedString(model));
}

// ---------------------------------------------------------------------------
//                     Implementation details below

inline absl::Span<const BrushCoat> BrushFamily::GetCoats() const {
  return coats_;
}

inline const BrushFamily::InputModel& BrushFamily::GetInputModel() const {
  return input_model_;
}

inline const BrushFamily::Metadata& BrushFamily::GetMetadata() const {
  return metadata_;
}

}  // namespace ink

#endif  // INK_STROKES_BRUSH_BRUSH_FAMILY_H_
