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

#ifndef INK_STROKES_BRUSH_BRUSH_BEHAVIOR_H_
#define INK_STROKES_BRUSH_BRUSH_BEHAVIOR_H_

#include <array>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include "absl/status/status.h"
#include "ink/brush/easing_function.h"

namespace ink {

// A behavior describing how stroke input properties should affect the shape and
// color of the brush tip.
//
// The behavior is conceptually a graph made from the various node types defined
// below. Each edge of the graph represents passing a nullable floating point
// value between nodes, and each node in the graph fits into one of the
// following categories:
//   1. Leaf nodes generate an output value without graph inputs. For example,
//      they can create a value from properties of stroke input.
//   2. Filter nodes can conditionally toggle branches of the graph "on" by
//      outputting their input value, or "off" by outputting a null value.
//   3. Operator nodes take in one or more input values and generate an output.
//      For example, by mapping input to output with an easing function.
//   4. Terminal nodes apply one or more input values to chosen properties of
//      the brush tip.
//
// The behavior is specified by a single list of nodes that represents a
// flattened, post-order traversal of the graph. The simplest form of behavior
// consists of two nodes:
//
//                     +--------+      +--------+
//                     | Source | ---> | Target |
//                     +--------+      +--------+
//
// This behavior would be represented by the list {Source, Target}. An example
// of such a behavior defined in C++ would be:
//
//    BrushBehavior behavior = {
//        .nodes = {
//            BrushBehavior::SourceNode{
//                .source = BrushBehavior::Source::kPressure,
//                .source_value_range = {0.2, 0.8},
//            },
//            BrushBehavior::TargetNode{
//                .target = BrushBehavior::Target::kSizeMultiplier,
//                .target_modifier_range = {0.75, 1.25},
//            },
//        }};
//
// A more complex behavior could use two source values for a single target:
//
//             +----------+      +-----+
//             | Source 1 | ---> |     |
//             +----------+      |     |      +--------+
//                               | Max | ---> | Target |
//             +----------+      |     |      +--------+
//             | Source 2 | ---> |     |
//             +----------+      +-----+
//
// This could be represented by the list {Source 1, Source 2, Max, Target}.
//
// For each input in a stroke, `BrushTip::behaviors` are applied as follows:
//   1. A target modifier for each tip property is accumulated from every
//     `BrushBehavior` present on the current `BrushTip`:
//        * Multiple behaviors can affect the same `Target`.
//        * Depending on the `Target`, modifiers from multiple behaviors will
//          stack either additively or multiplicatively, according to the
//          descriptions on that `BrushBehavior::Target`.
//        * Regardless, the order of specified behaviors does not affect the
//          result.
//   2. The modifiers are applied to the shape and color shift values of the
//      tip's state according to the descriptions on each `Target`. The
//      resulting tip property values are then clamped or normalized to within
//      their valid range of values. E.g. the final value of
//      `BrushTip::corner_rounding` will be clamped within [0, 1].
//      Generally:
//        * The affected shape values are those found in `BrushTip` members.
//        * The color shift values remain in the range -100% to +100%. Note that
//          when stored on a vertex, the color shift is encoded such that each
//          channel is in the range [0, 1], where 0.5 represents a 0% shift.
//
// Note that the accumulated tip shape property modifiers may be adjusted by the
// implementation before being applied: The rates of change of shape properties
// may be constrained to keep them from changing too rapidly with respect to
// distance traveled from one input to the next.
struct BrushBehavior {
  // List of input properties along with their units that can act as sources for
  // a `BrushBehavior`.
  //
  // This should match the enum in BrushBehavior.kt and
  // BrushFamilyExtensions.kt.
  //
  // Behaviors that consider properties of the stroke input do not consider
  // alterations to the visible position of that point in the stroke by brush
  // behaviors that modify that position (e.g.
  // Target::kPositionOffsetXInMultiplesOfBrushSize). That is, the position,
  // velocity, and acceleration of the stroke input may not match the
  // visible position, velocity, and acceleration of that point in the drawn
  // stroke. The stroke inputs considered by these behaviors are specifically
  // the "modeled" inputs used to construct the stroke geometry, which may be
  // upsampled, denoised, or otherwise transformed from the raw stroke input
  // (see `BrushFamily::InputModel`).
  //
  enum class Source : int8_t {
    // Stylus or touch pressure with values reported in the range [0, 1].
    kNormalizedPressure,
    // Stylus tilt with values reported in the range [0, π/2] radians.
    kTiltInRadians,
    // Stylus tilt along the x and y axes in the range [-π/2, π/2], with a
    // positive value corresponding to tilt toward the respective positive axis.
    // In order for those values to be reported, both tilt and orientation have
    // to be populated on the StrokeInput.
    kTiltXInRadians,
    kTiltYInRadians,
    // Stylus orientation with values reported in the range [0, 2π).
    kOrientationInRadians,
    // Stylus orientation with values reported in the range (-π, π].
    kOrientationAboutZeroInRadians,
    // Absolute speed of the modeled stroke input in multiples of the brush size
    // per second. Note that this value doesn't take into account brush
    // behaviors that offset the position of the visual tip of the stroke.
    kSpeedInMultiplesOfBrushSizePerSecond,
    // Signed x and y components of the velocity of the modeled stroke input in
    // multiples of the brush size per second. Note that this value doesn't take
    // into account brush behaviors that offset the visible position of that
    // point in the stroke.
    kVelocityXInMultiplesOfBrushSizePerSecond,
    kVelocityYInMultiplesOfBrushSizePerSecond,
    // Angle of the modeled stroke input's current direction of travel in stroke
    // coordinate space, normalized to the range [0, 2π). A value of 0 indicates
    // the direction of the positive x-axis; a value of π/2 indicates the
    // direction of the positive y-axis.
    kDirectionInRadians,
    // Angle of the modeled stroke input's current direction of travel in stroke
    // coordinate space, normalized to the range (-π, π]. A value of 0 indicates
    // the direction of the positive x-axis; a value of π/2 indicates the
    // direction of the positive y-axis.
    kDirectionAboutZeroInRadians,
    // Signed x and y components of the modeled stroke input's current direction
    // of travel in stroke coordinate space, normalized to the range [-1, 1].
    kNormalizedDirectionX,
    kNormalizedDirectionY,
    // Distance traveled by the inputs of the current stroke, starting at 0 at
    // the first input, where one distance unit is equal to the brush size.
    kDistanceTraveledInMultiplesOfBrushSize,
    // Time elapsed from the start of the stroke to the current modeled stroke
    // input. The value remains fixed for any given part of the stroke once
    // drawn.
    kTimeOfInputInSeconds,
    kTimeOfInputInMillis,
    // Distance traveled by the inputs of the current prediction, starting at 0
    // at the last non-predicted input, in multiples of the brush size. Zero for
    // inputs before the predicted portion of the stroke.
    kPredictedDistanceTraveledInMultiplesOfBrushSize,
    // Elapsed time of the prediction, starting at 0 at the last non-predicted
    // input. Zero for inputs before the predicted portion of the stroke.
    kPredictedTimeElapsedInSeconds,
    kPredictedTimeElapsedInMillis,
    // The distance left to be traveled from a given modeled input to the
    // current last modeled input of the stroke in multiples of the brush size.
    // This value changes for each input as the stroke is drawn.
    kDistanceRemainingInMultiplesOfBrushSize,
    // Time elapsed since the modeled stroke input. This continues to increase
    // even after all stroke inputs have completed, and can be used to drive
    // stroke animations. These enumerators are only compatible with a
    // `source_out_of_range_behavior` of `kClamp`, to ensure that the animation
    // will eventually end.
    kTimeSinceInputInSeconds,
    kTimeSinceInputInMillis,
    // Absolute acceleration of the modeled stroke input in multiples of the
    // brush size per second squared. Note that this value doesn't take into
    // account brush behaviors that offset the position of that visible point in
    // the stroke.
    kAccelerationInMultiplesOfBrushSizePerSecondSquared,
    // Signed x and y components of the acceleration of the modeled stroke input
    // in multiples of the brush size per second squared. Note that this value
    // doesn't take into account brush behaviors that offset the position of
    // that visible point in the stroke.
    kAccelerationXInMultiplesOfBrushSizePerSecondSquared,
    kAccelerationYInMultiplesOfBrushSizePerSecondSquared,
    // Signed component of acceleration of the modeled stroke input in the
    // direction of its velocity in multiples of the brush size per second
    // squared. Note that this value doesn't take into account brush behaviors
    // that offset the position of that visible point in the stroke.
    kAccelerationForwardInMultiplesOfBrushSizePerSecondSquared,
    // Signed component of acceleration of the modeled stroke input
    // perpendicular to its velocity, rotated 90 degrees in the direction from
    // the positive x-axis towards the positive y-axis, in multiples of the
    // brush size per second squared. Note that this value doesn't take into
    // account brush behaviors that offset the position of that visible point
    // in the stroke.
    kAccelerationLateralInMultiplesOfBrushSizePerSecondSquared,
    // Absolute speed of the modeled stroke input pointer in centimeters per
    // second.
    kInputSpeedInCentimetersPerSecond,
    // Signed x and y components of the modeled stroke input pointer velocity
    // in centimeters per second.
    kInputVelocityXInCentimetersPerSecond,
    kInputVelocityYInCentimetersPerSecond,
    // Distance in centimeters traveled by the modeled stroke input pointer
    // along the input path from the start of the stroke.
    kInputDistanceTraveledInCentimeters,
    // Distance in centimeters alonge the input path from the real portion of
    // the modeled stroke to this input. Zero for inputs before the predicted
    // portion of the stroke.
    kPredictedInputDistanceTraveledInCentimeters,
    // Absolute acceleration of the modeled stroke input pointer in centimeters
    // per second squared.
    kInputAccelerationInCentimetersPerSecondSquared,
    // Signed x and y components of the acceleration of the modeled stroke input
    // pointer in centimeters per second squared.
    kInputAccelerationXInCentimetersPerSecondSquared,
    kInputAccelerationYInCentimetersPerSecondSquared,
    // Signed component of acceleration of the modeled stroke input pointer in
    // the direction of its velocity in centimeters per second squared.
    kInputAccelerationForwardInCentimetersPerSecondSquared,
    // Signed component of acceleration of the modeled stroke input pointer
    // perpendicular to its velocity, rotated 90 degrees in the direction from
    // the positive x-axis towards the positive y-axis, in centimeters per
    // second squared.
    kInputAccelerationLateralInCentimetersPerSecondSquared,
    // Distance from the current modeled input to the end of the stroke along
    // the input path, as a fraction of the current total length of the stroke.
    // This value changes for each input as inputs are added.
    kDistanceRemainingAsFractionOfStrokeLength,
    // TODO: b/336565152 - Add kInputDistanceRemainingInCentimeters (this will
    // require some refactoring for the code that calculates
    // BrushTipModeler::distance_remaining_behavior_upper_bound_).
  };

  // List of tip properties that can be modified by a `BrushBehavior`.
  //
  // This should match the enums in BrushBehavior.kt and
  // BrushFamilyExtensions.kt.
  enum class Target : int8_t {
    // `kWidthMultiplier` and `kHeightMultiplier` scale the brush-tip size along
    // one dimension, starting from the values calculated using
    // `BrushTip::scale`, while `kSizeMultiplier` is a convenience target that
    // affects both width and height at once. The final brush size is clamped to
    // a maximum of twice the base size along each dimension. If multiple
    // behaviors have one of these targets, they stack multiplicatively (thus
    // allowing one behavior to scale the size down to zero over time, "winning"
    // over all other size-modifying behaviors).
    kWidthMultiplier,
    kHeightMultiplier,
    kSizeMultiplier,
    // Adds the target modifier to `BrushTip::slant`. The final brush slant
    // value is clamped to [-π/2, π/2]. If multiple behaviors have this target,
    // they stack additively.
    kSlantOffsetInRadians,
    // Adds the target modifier to `BrushTip::pinch`. The final brush pinch
    // value is clamped to [0, 1]. If multiple behaviors have this target, they
    // stack additively.
    kPinchOffset,
    // Adds the target modifier to `BrushTip::rotation`. The final brush
    // rotation angle is effectively normalized (mod 2π). If multiple behaviors
    // have this target, they stack additively.
    kRotationOffsetInRadians,
    // Adds the target modifier to `BrushTip::corner_rounding`. The final brush
    // corner rounding value is clamped to [0, 1]. If multiple behaviors have
    // this target, they stack additively.
    kCornerRoundingOffset,
    // Adds the target modifier to the brush tip x or y position in multiples of
    // the brush size.
    kPositionOffsetXInMultiplesOfBrushSize,
    kPositionOffsetYInMultiplesOfBrushSize,
    // Moves the brush tip by the target modifier times the brush size in the
    // direction of the modeled stroke input's velocity (the opposite direction
    // if the value is negative).
    kPositionOffsetForwardInMultiplesOfBrushSize,
    // Moves the brush tip by the target modifier times the brush size
    // perpendicular to the modeled stroke input's velocity, rotated 90 degrees
    // in the direction from the positive x-axis to the positive y-axis.
    kPositionOffsetLateralInMultiplesOfBrushSize,
    // Adds the target modifier to the initial texture animation progress value
    // of the current particle (which is relevant only for strokes with an
    // animated texture). The final progress offset is not clamped, but is
    // effectively normalized (mod 1). If multiple behaviors have this target,
    // they stack additively.
    kTextureAnimationProgressOffset,

    // The following are targets for tip color adjustments, including opacity.
    // Renderers can apply them to the brush color when a stroke is drawn to
    // contribute to the local color of each part of the stroke.
    //
    // TODO: b/344839538 - Rename and re-document kLuminosity once we decide how
    // that target should stack.

    // Shifts the hue of the base brush color.  A positive offset shifts around
    // the hue wheel from red towards orange, while a negative offset shifts the
    // other way, from red towards violet. The final hue offset is not clamped,
    // but is effectively normalized (mod 2π). If multiple behaviors have this
    // target, they stack additively.
    kHueOffsetInRadians,
    // Scales the saturation of the base brush color.  If multiple behaviors
    // have one of these targets, they stack multiplicatively.  The final
    // saturation multiplier is clamped to [0, 2].
    kSaturationMultiplier,
    // Target the luminosity of the color. An offset of +/-100% corresponds to
    // changing the luminosity by up to +/-100%.
    kLuminosity,
    // Scales the opacity of the base brush color.  If multiple behaviors have
    // one of these targets, they stack multiplicatively.  The final opacity
    // multiplier is clamped to [0, 2].
    kOpacityMultiplier,
  };

  // List of vector tip properties that can be modified by a `BrushBehavior`.
  //
  // This should match the enums in BrushBehavior.kt and
  // BrushFamilyExtensions.kt.
  enum class PolarTarget : int8_t {
    // Adds the vector to the brush tip's absolute x/y position in stroke space,
    // where the angle input is measured in radians and the magnitude input is
    // measured in units equal to the brush size. An angle of zero indicates an
    // offset in the direction of the positive x-axis in stroke space; an angle
    // of π/2 indicates the direction of the positive y-axis in stroke space.
    kPositionOffsetAbsoluteInRadiansAndMultiplesOfBrushSize,
    // Adds the vector to the brush tip's forward/lateral position relative to
    // the current direction of input travel, where the angle input is measured
    // in radians and the magnitude input is measured in units equal to the
    // brush size. An angle of zero indicates a forward offset in the current
    // direction of input travel, while an angle of π indicates a backwards
    // offset. Meanwhile, if the x- and y-axes of stroke space were rotated so
    // that the positive x-axis points in the direction of stroke travel, then
    // an angle of π/2 would indicate a lateral offset towards the positive
    // y-axis, and an angle of -π/2 would indicate a lateral offset towards the
    // negative y-axis.
    kPositionOffsetRelativeInRadiansAndMultiplesOfBrushSize,
  };

  // The desired behavior when an input value is outside the bounds of
  // `source_value_range`.
  //
  // This should match the enum in BrushBehavior.kt and
  // BrushFamilyExtensions.kt.
  enum class OutOfRange : int8_t {
    // Values outside the range will be clamped to not exceed the bounds.
    kClamp,
    // Values will be shifted by an integer multiple of the range size so that
    // they fall within the bounds.
    //
    // In this case, the range will be treated as a half-open interval, with a
    // value exactly at `source_value_range[1]` being treated as though it was
    // `source_value_range[0]`.
    kRepeat,
    // Similar to `kRepeat`, but every other repetition of the bounds will be
    // mirrored, as though the two elements of `source_value_range` were
    // swapped. This means the range does not need to be treated as a half-open
    // interval like in the case of `kRepeat`.
    kMirror,
  };

  // Flags allowing behaviors to be active for a limited subset of tool types.
  struct EnabledToolTypes {
    bool unknown = false;
    bool mouse = false;
    bool touch = false;
    bool stylus = false;

    bool HasAnyTypes() const;
    bool HasAllTypes() const;

    friend bool operator==(const EnabledToolTypes&,
                           const EnabledToolTypes&) = default;
  };

  static constexpr EnabledToolTypes kAllToolTypes = {
      .unknown = true, .mouse = true, .touch = true, .stylus = true};

  // List of input properties that might not be reported by `StrokeInput`.
  //
  // This should match the enums in BrushBehavior.kt and
  // BrushFamilyExtensions.kt.
  enum OptionalInputProperty : int8_t {
    kPressure,
    kTilt,
    kOrientation,
    // Tilt-x and tilt-y require both tilt and orientation to be reported.
    kTiltXAndY,
  };

  // A binary operation for combining two values in a `BinaryOpNode`.
  // LINT.IfChange(binary_op)
  enum class BinaryOp {
    kProduct,  // A * B, or null if either is null
    kSum,      // A + B, or null if either is null
  };
  // LINT.ThenChange(
  //   fuzz_domains.cc:binary_op,
  //   ../storage/proto/brush_family.proto:binary_op,
  // )

  // Dimensions and units for measuring distance/time along the length/duration
  // of a stroke.
  // LINT.IfChange(progress_domain)
  enum class ProgressDomain {
    // Progress in input distance traveled since the start of the stroke,
    // measured in centimeters. If the input data does not indicate the
    // relationship between stroke units and physical units (e.g. as may be the
    // case for programmatically-generated inputs), then special handling will
    // be applied based on the node type.
    kDistanceInCentimeters,
    // Progress in input distance traveled since the start of the stroke,
    // measured in multiples of the brush size.
    kDistanceInMultiplesOfBrushSize,
    // Progress in input time since the start of the stroke, measured in
    // seconds.
    kTimeInSeconds,
  };
  // LINT.ThenChange(
  //   fuzz_domains.cc:progress_domain,
  //   ../storage/proto/brush_family.proto:progress_domain,
  // )

  // An interpolation function for combining three values in an
  // `InterpolationNode`.
  // LINT.IfChange(interpolation)
  enum class Interpolation {
    // Linear interpolation. Uses parameter A to interpolate between B (when
    // A=0) and C (when A=1).
    kLerp,
    // Inverse linear interpolation. Outputs 0 when A=B and 1 when A=C,
    // interpolating linearly in between. Outputs null if B=C.
    kInverseLerp,
  };
  // LINT.ThenChange(
  //   fuzz_domains.cc:interpolation,
  //   ../storage/proto/brush_family.proto:interpolation,
  // )

  ////////////////////////
  /// LEAF VALUE NODES ///
  ////////////////////////

  // Value node for getting data from the stroke input batch.
  // Inputs: 0
  // Output: The value of the source after inverse-lerping from the specified
  //     value range and applying the specified out-of-range behavior, or null
  //     if the source value is indeterminate (e.g. because the stroke input
  //     batch is missing that property).
  // To be valid:
  //   - `source` must be a valid `Source` enumerator.
  //   - `source_out_of_range_behavior` must be a valid `OutOfRange` enumerator.
  //   - The endpoints of `source_value_range` must be finite and distinct.
  struct SourceNode {
    Source source;
    OutOfRange source_out_of_range_behavior = OutOfRange::kClamp;
    std::array<float, 2> source_value_range;

    friend bool operator==(const SourceNode&, const SourceNode&) = default;
  };

  // Value node for producing a constant value.
  // Inputs: 0
  // Output: The specified constant value.
  // To be valid: `value` must be finite.
  struct ConstantNode {
    float value;

    friend bool operator==(const ConstantNode&, const ConstantNode&) = default;
  };

  // Value node for producing a continuous random noise function with values
  // between 0 to 1.
  // Inputs: 0
  // Output: The current random value.
  // To be valid:
  //   - `vary_over` must be a valid `ProgressDomain` enumerator.
  //   - `base_period` must be finite and strictly positive.
  struct NoiseNode {
    uint32_t seed;
    ProgressDomain vary_over;
    float base_period;

    friend bool operator==(const NoiseNode&, const NoiseNode&) = default;
  };

  //////////////////////////
  /// FILTER VALUE NODES ///
  //////////////////////////

  // Value node for filtering out a branch of a behavior graph unless a
  // particular stroke input property is missing.
  // Inputs: 1
  // Output: Null if the specified property is present in the stroke input
  //     batch, otherwise the input value.
  // To be valid:
  //   - `is_fallback_for` must be a valid `OptionalInputProperty` enumerator.
  struct FallbackFilterNode {
    OptionalInputProperty is_fallback_for;

    friend bool operator==(const FallbackFilterNode&,
                           const FallbackFilterNode&) = default;
  };

  // Value node for filtering out a branch of a behavior graph unless this
  // stroke's tool type is in the specified set.
  // Inputs: 1
  // Output: Null if this stroke's tool type is not in the specified set,
  //     otherwise the input value.
  // To be valid: At least one tool type must be enabled.
  struct ToolTypeFilterNode {
    EnabledToolTypes enabled_tool_types;

    friend bool operator==(const ToolTypeFilterNode&,
                           const ToolTypeFilterNode&) = default;
  };

  ////////////////////////////
  /// OPERATOR VALUE NODES ///
  ////////////////////////////

  // Value node for damping changes in an input value, causing the output value
  // to slowly follow changes in the input value over a specified time or
  // distance.
  // Inputs: 1
  // Output: The damped input value. If the input value becomes null, this node
  //     continues to emit its previous output value.  If the input value starts
  //     out null, the output value is null until the first non-null input.
  // To be valid:
  //   - `damping_source` must be a valid `ProgressDomain` enumerator.
  //   - `damping_gap` must be finite and non-negative.
  struct DampingNode {
    // If `damping_source` is `kDistanceInCentimeters` but the input data does
    // not indicate the relationship between stroke units and physical units
    // (e.g. as may be the case for programmatically-generated inputs), then no
    // damping will be performed (i.e. the `damping_gap` will be treated as
    // zero).
    ProgressDomain damping_source;
    float damping_gap;

    friend bool operator==(const DampingNode&, const DampingNode&) = default;
  };

  // Value node for mapping a value through a response curve.
  // Inputs: 1

  // Output: The result of the easing function when applied to the input value,
  //     or null if the input value is null.
  // To be valid: `function` must be a valid `EasingFunction`.
  struct ResponseNode {
    EasingFunction response_curve;

    friend bool operator==(const ResponseNode&, const ResponseNode&) = default;
  };

  // Value node for integrating an input value over time or distance.
  // Inputs: 1
  // Output: The integral of the input value since the start of the stroke,
  //     after inverse-lerping from the specified value range and applying the
  //     specified out-of-range behavior. If the input value ever becomes null,
  //     this node acts as though the input value were still equal to its most
  //     recent non-null value. If the input value starts out null, it is
  //     treated as zero until the first non-null input.
  // To be valid:
  //   - `integrate_over` must be a valid `ProgressDomain` enumerator.
  //   - `integral_out_of_range_behavior` must be a valid `OutOfRange`
  //     enumerator.
  //   - The endpoints of `integral_value_range` must be finite and distinct.
  struct IntegralNode {
    ProgressDomain integrate_over;
    OutOfRange integral_out_of_range_behavior;
    std::array<float, 2> integral_value_range;

    friend bool operator==(const IntegralNode&, const IntegralNode&) = default;
  };

  // Value node for combining two other values with a binary operation.
  // Inputs: 2
  // Output: The result of the specified operation on the two input values. See
  //     comments on `BinaryOp` for details on how each operator handles null
  //     input values.
  // To be valid: `operation` must be a valid `BinaryOp` enumerator.
  struct BinaryOpNode {
    BinaryOp operation;

    friend bool operator==(const BinaryOpNode&, const BinaryOpNode&) = default;
  };

  // Value node for interpolating to/from a range of two values.
  // Inputs: 3
  // Output: The result of using the first input value as an interpolation
  //     parameter between the second and third input values, using the
  //     specified interpolation function, or null if any input value is null.
  // To be valid: `interpolation` must be a valid `Interpolation` enumerator.
  struct InterpolationNode {
    Interpolation interpolation;

    friend bool operator==(const InterpolationNode&,
                           const InterpolationNode&) = default;
  };

  //////////////////////
  /// TERMINAL NODES ///
  //////////////////////

  // Terminal node that consumes a single input value to modify a scalar brush
  // tip property.
  // Inputs: 1
  // Effect: Applies a modifier to the specified target equal to the input value
  //     lerped to the specified range. If the input becomes null, the target
  //     continues to apply its previous effect from the most recent non-null
  //     input (if any).
  // To be valid:
  //   - `target` must be a valid `Target` enumerator.
  //   - The endpoints of `target_modifier_range` must be finite and distinct.
  struct TargetNode {
    Target target;
    std::array<float, 2> target_modifier_range;

    friend bool operator==(const TargetNode&, const TargetNode&) = default;
  };

  // Terminal node that consumes two input values (angle and magnitude), forming
  // a polar vector to modify a vector brush tip property.
  // Inputs: 2
  // Effect: Applies a vector modifier to the specified target equal to the
  //     polar vector formed by lerping the first input value to the specified
  //     angle range, and the second input to the specified magnitude range. If
  //     either input becomes null, the target continues to apply its previous
  //     effect from the most recent non-null inputs (if any).
  // To be valid:
  //   - `target` must be a valid `PolarTarget` enumerator.
  //   - The endpoints of `angle_range` and of `magnitude_range` must be finite
  //     and distinct.
  struct PolarTargetNode {
    PolarTarget target;
    std::array<float, 2> angle_range;
    std::array<float, 2> magnitude_range;

    friend bool operator==(const PolarTargetNode&,
                           const PolarTargetNode&) = default;
  };

  // A single node in a behavior's graph.  Each node type is either a "value
  // node" which consumes zero or more input values and produces a single output
  // value, or a "terminal node" which consumes one or more input values and
  // applies some effect to the brush tip (but does not produce any output
  // value).
  using Node = std::variant<SourceNode, ConstantNode, NoiseNode,
                            FallbackFilterNode, ToolTypeFilterNode, DampingNode,
                            ResponseNode, IntegralNode, BinaryOpNode,
                            InterpolationNode, TargetNode, PolarTargetNode>;

  // A post-order traversal of this behavior's node graph.
  std::vector<Node> nodes;
  // A multi-line, human-readable string with a description of this brush
  // behavior and its purpose within the brush, with the intended audience being
  // designers/developers who are editing the brush definition. This string is
  // not generally intended to be displayed to end users.
  std::string developer_comment;

  friend bool operator==(const BrushBehavior&, const BrushBehavior&) = default;
};

namespace brush_internal {

// Determines whether the given BrushBehavior struct is valid to be used in a
// BrushFamily, and returns an error if not. Validates both the top-level
// structure and the individual nodes.
absl::Status ValidateBrushBehavior(const BrushBehavior& behavior);

// Validates the top-level structure of a BrushBehavior, but not the individual
// nodes. This can be used to validate a BrushBehavior if the nodes are already
// validated.
absl::Status ValidateBrushBehaviorTopLevel(const BrushBehavior& behavior);

absl::Status ValidateBrushBehaviorNode(const BrushBehavior::Node& node);

std::string ToFormattedString(BrushBehavior::Source source);
std::string ToFormattedString(BrushBehavior::Target target);
std::string ToFormattedString(BrushBehavior::PolarTarget target);
std::string ToFormattedString(BrushBehavior::OutOfRange out_of_range);
std::string ToFormattedString(BrushBehavior::EnabledToolTypes enabled);
std::string ToFormattedString(BrushBehavior::OptionalInputProperty input);
std::string ToFormattedString(BrushBehavior::BinaryOp operation);
std::string ToFormattedString(BrushBehavior::ProgressDomain progress_domain);
std::string ToFormattedString(BrushBehavior::Interpolation interpolation);
std::string ToFormattedString(const BrushBehavior::Node& node);
std::string ToFormattedString(const BrushBehavior& behavior);

}  // namespace brush_internal

template <typename Sink>
void AbslStringify(Sink& sink, BrushBehavior::Source source) {
  sink.Append(brush_internal::ToFormattedString(source));
}

template <typename Sink>
void AbslStringify(Sink& sink, BrushBehavior::Target target) {
  sink.Append(brush_internal::ToFormattedString(target));
}

template <typename Sink>
void AbslStringify(Sink& sink, BrushBehavior::PolarTarget target) {
  sink.Append(brush_internal::ToFormattedString(target));
}

template <typename Sink>
void AbslStringify(Sink& sink, BrushBehavior::OutOfRange out_of_range) {
  sink.Append(brush_internal::ToFormattedString(out_of_range));
}

template <typename Sink>
void AbslStringify(Sink& sink, BrushBehavior::EnabledToolTypes enabled) {
  sink.Append(brush_internal::ToFormattedString(enabled));
}

template <typename Sink>
void AbslStringify(Sink& sink, BrushBehavior::OptionalInputProperty input) {
  sink.Append(brush_internal::ToFormattedString(input));
}

template <typename Sink>
void AbslStringify(Sink& sink, BrushBehavior::BinaryOp operation) {
  sink.Append(brush_internal::ToFormattedString(operation));
}

template <typename Sink>
void AbslStringify(Sink& sink, BrushBehavior::ProgressDomain progress_domain) {
  sink.Append(brush_internal::ToFormattedString(progress_domain));
}

template <typename Sink>
void AbslStringify(Sink& sink, BrushBehavior::Interpolation interpolation) {
  sink.Append(brush_internal::ToFormattedString(interpolation));
}

template <typename Sink>
void AbslStringify(Sink& sink, const BrushBehavior::Node& node) {
  sink.Append(brush_internal::ToFormattedString(node));
}

template <typename Sink>
void AbslStringify(Sink& sink, const BrushBehavior& behavior) {
  sink.Append(brush_internal::ToFormattedString(behavior));
}

}  // namespace ink

#endif  // INK_STROKES_BRUSH_BRUSH_BEHAVIOR_H_
