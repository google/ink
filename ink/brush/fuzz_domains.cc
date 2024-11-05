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

#include "ink/brush/fuzz_domains.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <utility>
#include <vector>

#include "fuzztest/fuzztest.h"
#include "absl/log/absl_check.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "ink/brush/brush.h"
#include "ink/brush/brush_behavior.h"
#include "ink/brush/brush_coat.h"
#include "ink/brush/brush_family.h"
#include "ink/brush/brush_paint.h"
#include "ink/brush/brush_tip.h"
#include "ink/brush/easing_function.h"
#include "ink/color/color.h"
#include "ink/color/fuzz_domains.h"
#include "ink/geometry/angle.h"
#include "ink/geometry/fuzz_domains.h"
#include "ink/geometry/point.h"
#include "ink/geometry/vec.h"
#include "ink/types/fuzz_domains.h"
#include "ink/types/uri.h"

namespace ink {
namespace {

using fuzztest::Arbitrary;
using fuzztest::ArrayOf;
using fuzztest::Domain;
using fuzztest::ElementOf;
using fuzztest::Filter;
using fuzztest::Finite;
using fuzztest::FlatMap;
using fuzztest::InRange;
using fuzztest::InRegexp;
using fuzztest::Just;
using fuzztest::Map;
using fuzztest::NonNegative;
using fuzztest::OneOf;
using fuzztest::OptionalOf;
using fuzztest::PairOf;
using fuzztest::Positive;
using fuzztest::StructOf;
using fuzztest::UniqueElementsContainerOf;
using fuzztest::VariantOf;
using fuzztest::VectorOf;

Domain<float> FiniteNonNegativeFloat() {
  return Filter([](float value) { return std::isfinite(value); },
                NonNegative<float>());
}

Domain<float> FinitePositiveFloat() {
  return Filter([](float value) { return std::isfinite(value); },
                Positive<float>());
}

Domain<std::pair<float, float>> PairOfFinitePositiveAscendingFloats() {
  auto a_and_b = [](float a) {
    return PairOf(InRange(a, std::numeric_limits<float>::max()), Just(a));
  };
  return FlatMap(a_and_b, FinitePositiveFloat());
}

Domain<std::array<float, 2>> ArrayOfTwoFiniteDistinctFloats() {
  return Map(
      [](const std::vector<float>& floats) {
        return std::array<float, 2>{floats[0], floats[1]};
      },
      UniqueElementsContainerOf<std::vector<float>>(Finite<float>())
          .WithSize(2));
}

Domain<BrushBehavior::EnabledToolTypes> ValidBrushBehaviorEnabledToolTypes() {
  return Filter(
      [](BrushBehavior::EnabledToolTypes enabled) {
        // To be valid, the EnabledToolTypes must set at least one tool type to
        // true.
        return enabled.HasAnyTypes();
      },
      StructOf<BrushBehavior::EnabledToolTypes>(
          Arbitrary<bool>(), Arbitrary<bool>(), Arbitrary<bool>(),
          Arbitrary<bool>()));
}

// LINT.IfChange(binary_op)
Domain<BrushBehavior::BinaryOp> ArbitraryBrushBehaviorBinaryOp() {
  return ElementOf({
      BrushBehavior::BinaryOp::kProduct,
      BrushBehavior::BinaryOp::kSum,
  });
}
// LINT.ThenChange(brush_behavior.h:binary_op)

// LINT.IfChange(damping_source)
Domain<BrushBehavior::DampingSource> ArbitraryBrushBehaviorDampingSource() {
  return ElementOf({
      BrushBehavior::DampingSource::kDistanceInCentimeters,
      BrushBehavior::DampingSource::kDistanceInMultiplesOfBrushSize,
      BrushBehavior::DampingSource::kTimeInSeconds,
  });
}
// LINT.ThenChange(brush_behavior.h:damping_source)

// LINT.IfChange(interpolation)
Domain<BrushBehavior::Interpolation> ArbitraryBrushBehaviorInterpolation() {
  return ElementOf({
      BrushBehavior::Interpolation::kLerp,
      BrushBehavior::Interpolation::kInverseLerp,
  });
}
// LINT.ThenChange(brush_behavior.h:interpolation)

// LINT.IfChange(optional_input_property)
Domain<BrushBehavior::OptionalInputProperty>
ArbitraryBrushBehaviorOptionalInputProperty() {
  return ElementOf({
      BrushBehavior::OptionalInputProperty::kPressure,
      BrushBehavior::OptionalInputProperty::kTilt,
      BrushBehavior::OptionalInputProperty::kOrientation,
      BrushBehavior::OptionalInputProperty::kTiltXAndY,
  });
}
// LINT.ThenChange(brush_behavior.h:optional_input_property)

// LINT.IfChange(out_of_range)
Domain<BrushBehavior::OutOfRange> ArbitraryBrushBehaviorOutOfRange() {
  return ElementOf({
      BrushBehavior::OutOfRange::kClamp,
      BrushBehavior::OutOfRange::kRepeat,
      BrushBehavior::OutOfRange::kMirror,
  });
}
Domain<BrushBehavior::OutOfRange> ValidBrushBehaviorOutOfRangeForSource(
    BrushBehavior::Source source) {
  switch (source) {
    case BrushBehavior::Source::kTimeSinceInputInSeconds:
    case BrushBehavior::Source::kTimeSinceInputInMillis:
      return Just(BrushBehavior::OutOfRange::kClamp);
    default:
      return ArbitraryBrushBehaviorOutOfRange();
  }
}
// LINT.ThenChange(brush_behavior.h:out_of_range)

// LINT.IfChange(source)
Domain<BrushBehavior::Source> ArbitraryBrushBehaviorSource() {
  return ElementOf({
      BrushBehavior::Source::kNormalizedPressure,
      BrushBehavior::Source::kTiltInRadians,
      BrushBehavior::Source::kTiltXInRadians,
      BrushBehavior::Source::kTiltYInRadians,
      BrushBehavior::Source::kOrientationInRadians,
      BrushBehavior::Source::kOrientationAboutZeroInRadians,
      BrushBehavior::Source::kSpeedInMultiplesOfBrushSizePerSecond,
      BrushBehavior::Source::kVelocityXInMultiplesOfBrushSizePerSecond,
      BrushBehavior::Source::kVelocityYInMultiplesOfBrushSizePerSecond,
      BrushBehavior::Source::kDirectionInRadians,
      BrushBehavior::Source::kDirectionAboutZeroInRadians,
      BrushBehavior::Source::kNormalizedDirectionX,
      BrushBehavior::Source::kNormalizedDirectionY,
      BrushBehavior::Source::kDistanceTraveledInMultiplesOfBrushSize,
      BrushBehavior::Source::kTimeOfInputInSeconds,
      BrushBehavior::Source::kTimeOfInputInMillis,
      BrushBehavior::Source::kPredictedDistanceTraveledInMultiplesOfBrushSize,
      BrushBehavior::Source::kPredictedTimeElapsedInSeconds,
      BrushBehavior::Source::kPredictedTimeElapsedInMillis,
      BrushBehavior::Source::kDistanceRemainingInMultiplesOfBrushSize,
      BrushBehavior::Source::kTimeSinceInputInSeconds,
      BrushBehavior::Source::kTimeSinceInputInMillis,
      BrushBehavior::Source::
          kAccelerationInMultiplesOfBrushSizePerSecondSquared,
      BrushBehavior::Source::
          kAccelerationXInMultiplesOfBrushSizePerSecondSquared,
      BrushBehavior::Source::
          kAccelerationYInMultiplesOfBrushSizePerSecondSquared,
      BrushBehavior::Source::
          kAccelerationForwardInMultiplesOfBrushSizePerSecondSquared,
      BrushBehavior::Source::
          kAccelerationLateralInMultiplesOfBrushSizePerSecondSquared,
      BrushBehavior::Source::kInputSpeedInCentimetersPerSecond,
      BrushBehavior::Source::kInputVelocityXInCentimetersPerSecond,
      BrushBehavior::Source::kInputVelocityYInCentimetersPerSecond,
      BrushBehavior::Source::kInputDistanceTraveledInCentimeters,
      BrushBehavior::Source::kPredictedInputDistanceTraveledInCentimeters,
      BrushBehavior::Source::kInputAccelerationInCentimetersPerSecondSquared,
      BrushBehavior::Source::kInputAccelerationXInCentimetersPerSecondSquared,
      BrushBehavior::Source::kInputAccelerationYInCentimetersPerSecondSquared,
      BrushBehavior::Source::
          kInputAccelerationForwardInCentimetersPerSecondSquared,
      BrushBehavior::Source::
          kInputAccelerationLateralInCentimetersPerSecondSquared,
  });
}
// LINT.ThenChange(brush_behavior.h:source)

// LINT.IfChange(target)
Domain<BrushBehavior::Target> ArbitraryBrushBehaviorTarget() {
  return ElementOf({
      BrushBehavior::Target::kWidthMultiplier,
      BrushBehavior::Target::kHeightMultiplier,
      BrushBehavior::Target::kSizeMultiplier,
      BrushBehavior::Target::kSlantOffsetInRadians,
      BrushBehavior::Target::kPinchOffset,
      BrushBehavior::Target::kRotationOffsetInRadians,
      BrushBehavior::Target::kCornerRoundingOffset,
      BrushBehavior::Target::kPositionOffsetXInMultiplesOfBrushSize,
      BrushBehavior::Target::kPositionOffsetYInMultiplesOfBrushSize,
      BrushBehavior::Target::kPositionOffsetForwardInMultiplesOfBrushSize,
      BrushBehavior::Target::kPositionOffsetLateralInMultiplesOfBrushSize,
      BrushBehavior::Target::kHueOffsetInRadians,
      BrushBehavior::Target::kSaturationMultiplier,
      BrushBehavior::Target::kLuminosity,
      BrushBehavior::Target::kOpacityMultiplier,
  });
}
// LINT.ThenChange(brush_behavior.h:target)

// LINT.IfChange(predefined)
Domain<EasingFunction::Predefined> ArbitraryEasingFunctionPredefined() {
  return ElementOf({
      EasingFunction::Predefined::kLinear,
      EasingFunction::Predefined::kEase,
      EasingFunction::Predefined::kEaseIn,
      EasingFunction::Predefined::kEaseOut,
      EasingFunction::Predefined::kEaseInOut,
      EasingFunction::Predefined::kStepStart,
      EasingFunction::Predefined::kStepEnd,
  });
}
// LINT.ThenChange(easing_function.h:predefined)

Domain<EasingFunction::CubicBezier> ValidEasingFunctionCubicBezier() {
  return StructOf<EasingFunction::CubicBezier>(
      InRange<float>(0.f, 1.f), Finite<float>(), InRange<float>(0.f, 1.f),
      Finite<float>());
}

Domain<EasingFunction::Linear> ValidEasingFunctionLinear() {
  return StructOf<EasingFunction::Linear>(Map(
      [](std::vector<Point> points) {
        std::stable_sort(points.begin(), points.end(),
                         [](Point lhs, Point rhs) { return lhs.x < rhs.x; });
        return points;
      },
      VectorOf(StructOf<Point>(InRange(0.f, 1.f), Finite<float>()))));
}

Domain<EasingFunction::Steps> ValidEasingFunctionSteps() {
  return Map(
      [](int32_t step_count, EasingFunction::StepPosition step_position) {
        if (step_position == EasingFunction::StepPosition::kJumpNone &&
            step_count < 2) {
          step_count++;
        }
        return EasingFunction::Steps{.step_count = step_count,
                                     .step_position = step_position};
      },
      InRange<int32_t>(1, std::numeric_limits<int32_t>::max()),
      // LINT.IfChange(step_position)
      ElementOf({EasingFunction::StepPosition::kJumpEnd,
                 EasingFunction::StepPosition::kJumpStart,
                 EasingFunction::StepPosition::kJumpNone,
                 EasingFunction::StepPosition::kJumpBoth}));
  // LINT.ThenChange(easing_function.h:step_position)
}

Domain<BrushBehavior::SourceNode> ValidBrushBehaviorSourceNode() {
  return FlatMap(
      [](BrushBehavior::Source source) {
        return StructOf<BrushBehavior::SourceNode>(
            Just(source), ValidBrushBehaviorOutOfRangeForSource(source),
            ArrayOfTwoFiniteDistinctFloats());
      },
      ArbitraryBrushBehaviorSource());
}

Domain<BrushBehavior::ConstantNode> ValidBrushBehaviorConstantNode() {
  return StructOf<BrushBehavior::ConstantNode>(Finite<float>());
}

Domain<BrushBehavior::FallbackFilterNode>
ValidBrushBehaviorFallbackFilterNode() {
  return StructOf<BrushBehavior::FallbackFilterNode>(
      ArbitraryBrushBehaviorOptionalInputProperty());
}

Domain<BrushBehavior::ToolTypeFilterNode>
ValidBrushBehaviorToolTypeFilterNode() {
  return StructOf<BrushBehavior::ToolTypeFilterNode>(
      ValidBrushBehaviorEnabledToolTypes());
}

Domain<BrushBehavior::DampingNode> ValidBrushBehaviorDampingNode() {
  return StructOf<BrushBehavior::DampingNode>(
      ArbitraryBrushBehaviorDampingSource(), FiniteNonNegativeFloat());
}

Domain<BrushBehavior::ResponseNode> ValidBrushBehaviorResponseNode() {
  return StructOf<BrushBehavior::ResponseNode>(ValidEasingFunction());
}

Domain<BrushBehavior::BinaryOpNode> ValidBrushBehaviorBinaryOpNode() {
  return StructOf<BrushBehavior::BinaryOpNode>(
      ArbitraryBrushBehaviorBinaryOp());
}

Domain<BrushBehavior::InterpolationNode> ValidBrushBehaviorInterpolationNode() {
  return StructOf<BrushBehavior::InterpolationNode>(
      ArbitraryBrushBehaviorInterpolation());
}

Domain<BrushBehavior::TargetNode> ValidBrushBehaviorTargetNode() {
  return StructOf<BrushBehavior::TargetNode>(ArbitraryBrushBehaviorTarget(),
                                             ArrayOfTwoFiniteDistinctFloats());
}

// Casts a domain over a specific node type (e.g. `SourceNode`) into a domain
// over `Node`.
template <typename T>
Domain<BrushBehavior::Node> BrushBehaviorNodeOf(Domain<T> domain) {
  return Map([](const BrushBehavior::Node& node) { return node; }, domain);
}

// A domain over all valid behavior node subtrees that consist of a single leaf
// node.
Domain<std::vector<BrushBehavior::Node>> ValidBrushBehaviorNodeLeaf() {
  return Map(
      [](const BrushBehavior::Node& node) {
        return std::vector<BrushBehavior::Node>{node};
      },
      OneOf(BrushBehaviorNodeOf(ValidBrushBehaviorSourceNode()),
            BrushBehaviorNodeOf(ValidBrushBehaviorConstantNode())));
}

// A domain over all valid behavior node subtrees (i.e. with a value node at the
// root) with the specified maximum depth (which must be strictly positive).
Domain<std::vector<BrushBehavior::Node>>
ValidBrushBehaviorNodeSubtreeWithMaxDepth(int max_depth) {
  ABSL_CHECK_GT(max_depth, 0);
  // If `max_depth` is 1, the subtree can only be a leaf node.
  if (max_depth == 1) return ValidBrushBehaviorNodeLeaf();

  // Once fuzztest provides a mechanism for recursion limits on recursive
  // domains (b/230901925), we should use that instead.
  Domain<std::vector<BrushBehavior::Node>> smaller_subtree =
      ValidBrushBehaviorNodeSubtreeWithMaxDepth(max_depth - 1);

  return OneOf(
      // A subtree could just be a leaf node:
      ValidBrushBehaviorNodeLeaf(),
      // Or it could be a unary node with a smaller subtree as an input:
      Map(
          [](const std::vector<BrushBehavior::Node>& input,
             const BrushBehavior::Node& node) {
            std::vector<BrushBehavior::Node> result = input;
            result.push_back(node);
            return result;
          },
          smaller_subtree,
          OneOf(BrushBehaviorNodeOf(ValidBrushBehaviorFallbackFilterNode()),
                BrushBehaviorNodeOf(ValidBrushBehaviorToolTypeFilterNode()),
                BrushBehaviorNodeOf(ValidBrushBehaviorDampingNode()),
                BrushBehaviorNodeOf(ValidBrushBehaviorResponseNode()))),
      // Or it could be a binary node with two smaller subtrees as inputs:
      Map(
          [](const std::vector<BrushBehavior::Node>& input1,
             const std::vector<BrushBehavior::Node>& input2,
             const BrushBehavior::BinaryOpNode& node) {
            std::vector<BrushBehavior::Node> result = input1;
            result.insert(result.end(), input2.begin(), input2.end());
            result.push_back(node);
            return result;
          },
          smaller_subtree, smaller_subtree, ValidBrushBehaviorBinaryOpNode()));
}

// A domain over all valid behavior node trees (i.e. with a target node at the
// root).
Domain<std::vector<BrushBehavior::Node>> ValidBrushBehaviorNodeTree() {
  return Map(
      [](const std::vector<BrushBehavior::Node>& input,
         const BrushBehavior::TargetNode& node) {
        std::vector<BrushBehavior::Node> result = input;
        result.push_back(node);
        return result;
      },
      // Arbitrarily limit the tree depth to prevent resource exhaustion.
      ValidBrushBehaviorNodeSubtreeWithMaxDepth(5),
      ValidBrushBehaviorTargetNode());
}

// A domain over all valid behavior node forests (i.e. containing zero or more
// complete trees, each with a target node at the root).
Domain<std::vector<BrushBehavior::Node>> ValidBrushBehaviorNodeForest() {
  return Map(
      [](const std::vector<std::vector<BrushBehavior::Node>>& trees) {
        std::vector<BrushBehavior::Node> result;
        for (const std::vector<BrushBehavior::Node>& tree : trees) {
          result.insert(result.end(), tree.begin(), tree.end());
        }
        return result;
      },
      VectorOf(ValidBrushBehaviorNodeTree()));
}

}  // namespace

Domain<Brush> ArbitraryBrush() {
  return Map(
      [](const BrushFamily& family, const Color& color,
         std::pair<float, float> size_and_epsilon) {
        return Brush::Create(family, color, size_and_epsilon.first,
                             size_and_epsilon.second)
            .value();
      },
      ArbitraryBrushFamily(), ArbitraryColor(),
      PairOfFinitePositiveAscendingFloats());
}

Domain<Brush> BrushWithoutUri() {
  return Map(
      [](const BrushFamily& family, const Color& color,
         std::pair<float, float> size_and_epsilon) {
        return Brush::Create(family, color, size_and_epsilon.first,
                             size_and_epsilon.second)
            .value();
      },
      BrushFamilyWithoutUri(), ArbitraryColor(),
      PairOfFinitePositiveAscendingFloats());
}

Domain<BrushBehavior> ValidBrushBehavior() {
  return StructOf<BrushBehavior>(ValidBrushBehaviorNodeForest());
}

Domain<BrushBehavior::Node> ValidBrushBehaviorNode() {
  return VariantOf(
      ValidBrushBehaviorSourceNode(), ValidBrushBehaviorConstantNode(),
      ValidBrushBehaviorFallbackFilterNode(),
      ValidBrushBehaviorToolTypeFilterNode(), ValidBrushBehaviorDampingNode(),
      ValidBrushBehaviorResponseNode(), ValidBrushBehaviorBinaryOpNode(),
      ValidBrushBehaviorInterpolationNode(), ValidBrushBehaviorTargetNode());
}

Domain<BrushCoat> ValidBrushCoat() {
  return StructOf<BrushCoat>(
      // TODO: b/285594469 - Once `BrushCoat` permits more than one tip per
      // coat, generate coats with other numbers of tips.
      VectorOf(ValidBrushTip()).WithSize(1), ValidBrushPaint());
}

Domain<BrushFamily> ArbitraryBrushFamily() {
  return Map(
      [](absl::Span<const BrushCoat> coats, const std::optional<Uri>& uri,
         const BrushFamily::InputModel& input_model) {
        return BrushFamily::Create(coats, uri, input_model).value();
      },
      VectorOf(ValidBrushCoat()).WithMaxSize(BrushFamily::MaxBrushCoats()),
      OptionalOf(ValidBrushFamilyUri()), ValidBrushFamilyInputModel());
}

Domain<BrushFamily> BrushFamilyWithoutUri() {
  return Map(
      [](absl::Span<const BrushCoat> coats,
         const BrushFamily::InputModel& input_model) {
        return BrushFamily::Create(coats, std::nullopt, input_model).value();
      },
      VectorOf(ValidBrushCoat()).WithMaxSize(BrushFamily::MaxBrushCoats()),
      ValidBrushFamilyInputModel());
}

Domain<BrushFamily::InputModel> ValidBrushFamilyInputModel() {
  return VariantOf(StructOf<BrushFamily::SpringModelV1>(),
                   StructOf<BrushFamily::SpringModelV2>());
}

Domain<Uri> ValidBrushFamilyUri() {
  return Map(
      [](absl::string_view uri_string) {
        absl::StatusOr<Uri> uri = Uri::Parse(uri_string);
        ABSL_CHECK_OK(uri);
        return *std::move(uri);
      },
      InRegexp(
          "^(ink:|INK:)?(//[a-z-]+)?/(brush-family):[a-z-]+(:[1-9]{1,9})?"));
}

Domain<Uri> ValidColorTextureUri() {
  return Map(
      [](absl::string_view uri_string) {
        absl::StatusOr<Uri> uri = Uri::Parse(uri_string);
        ABSL_CHECK_OK(uri);
        return *std::move(uri);
      },
      InRegexp("^(ink:|INK:)?(//[a-z-]+)?/(texture):[a-z-]+(:[1-9]{1,9})?"));
}

// LINT.IfChange(texture_size_unit)
Domain<BrushPaint::TextureSizeUnit> ArbitraryBrushPaintTextureSizeUnit() {
  return ElementOf({
      BrushPaint::TextureSizeUnit::kBrushSize,
      BrushPaint::TextureSizeUnit::kStrokeSize,
      BrushPaint::TextureSizeUnit::kStrokeCoordinates,
  });
}
// LINT.ThenChange(brush_paint.h:texture_size_unit)

// LINT.IfChange(texture_mapping)
Domain<BrushPaint::TextureMapping> ArbitraryBrushPaintTextureMapping() {
  return ElementOf({
      BrushPaint::TextureMapping::kTiling,
      BrushPaint::TextureMapping::kWinding,
  });
}
// LINT.ThenChange(brush_paint.h:texture_mapping)

// LINT.IfChange(texture_origin)
Domain<BrushPaint::TextureOrigin> ArbitraryBrushPaintTextureOrigin() {
  return ElementOf({
      BrushPaint::TextureOrigin::kStrokeSpaceOrigin,
      BrushPaint::TextureOrigin::kFirstStrokeInput,
      BrushPaint::TextureOrigin::kLastStrokeInput,
  });
}
// LINT.ThenChange(brush_paint.h:texture_origin)

// LINT.IfChange(texture_wrap)
Domain<BrushPaint::TextureWrap> ArbitraryBrushPaintTextureWrap() {
  return ElementOf({
      BrushPaint::TextureWrap::kRepeat,
      BrushPaint::TextureWrap::kMirror,
      BrushPaint::TextureWrap::kClamp,
  });
}
// LINT.ThenChange(brush_paint.h:texture_wrap)

// LINT.IfChange(blend_mode)
Domain<BrushPaint::BlendMode> ArbitraryBrushPaintBlendMode() {
  return ElementOf({
      BrushPaint::BlendMode::kModulate,
      BrushPaint::BlendMode::kDstIn,
      BrushPaint::BlendMode::kDstOut,
      BrushPaint::BlendMode::kSrcAtop,
      BrushPaint::BlendMode::kSrcIn,
      BrushPaint::BlendMode::kSrcOver,
      BrushPaint::BlendMode::kDstOver,
      BrushPaint::BlendMode::kSrc,
      BrushPaint::BlendMode::kDst,
      BrushPaint::BlendMode::kSrcOut,
      BrushPaint::BlendMode::kDstAtop,
      BrushPaint::BlendMode::kXor,
  });
}
// LINT.ThenChange(brush_paint.h:blend_mode)

Domain<BrushPaint::TextureKeyframe> ValidBrushPaintTextureKeyframe() {
  return StructOf<BrushPaint::TextureKeyframe>(
      InRange<float>(0.f, 1.f),
      OptionalOf(StructOf<Vec>(FinitePositiveFloat(), FinitePositiveFloat())),
      OptionalOf(
          StructOf<Vec>(InRange<float>(0.f, 1.f), InRange<float>(0.f, 1.f))),
      OptionalOf(FiniteAngle()), OptionalOf(InRange(0.f, 1.f)));
}

fuzztest::Domain<BrushPaint::TextureLayer>
ValidBrushPaintTextureLayerWithMapping(BrushPaint::TextureMapping mapping) {
  auto texture_layer = [mapping](Vec size) {
    return StructOf<BrushPaint::TextureLayer>(
        ValidColorTextureUri(), Just(mapping),
        ArbitraryBrushPaintTextureOrigin(),
        ArbitraryBrushPaintTextureSizeUnit(), ArbitraryBrushPaintTextureWrap(),
        ArbitraryBrushPaintTextureWrap(), Just(size),
        StructOf<Vec>(InRange<float>(0.f, 1.f), InRange<float>(0.f, 1.f)),
        FiniteAngle(),
        StructOf<Vec>(InRange<float>(0.f, size.x), InRange<float>(0.f, size.y)),
        StructOf<Vec>(InRange<float>(0.f, 1.f), InRange<float>(0.f, 1.f)),
        FiniteAngle(), InRange(0.f, 1.f),
        VectorOf(ValidBrushPaintTextureKeyframe()),
        ArbitraryBrushPaintBlendMode());
  };
  return FlatMap(texture_layer,
                 StructOf<Vec>(FinitePositiveFloat(), FinitePositiveFloat()));
}

fuzztest::Domain<BrushPaint> ValidBrushPaint() {
  return FlatMap(
      [](BrushPaint::TextureMapping mapping) {
        return fuzztest::StructOf<BrushPaint>(
            VectorOf(ValidBrushPaintTextureLayerWithMapping(mapping)));
      },
      ArbitraryBrushPaintTextureMapping());
}

Domain<BrushTip> ValidBrushTip() {
  return StructOf<BrushTip>(
      // To be valid, the tip scale components must each be finite and
      // non-negative, and cannot both be zero.
      Filter([](Vec scale) { return scale != Vec(); },
             StructOf<Vec>(FiniteNonNegativeFloat(), FiniteNonNegativeFloat())),
      InRange<float>(0.f, 1.f), AngleInRange(-kQuarterTurn, kQuarterTurn),
      InRange<float>(0.f, 1.f), FiniteAngle(), InRange<float>(0.f, 2.f),
      FiniteNonNegativeFloat(), FiniteNonNegativeDuration32(),
      VectorOf(ValidBrushBehavior()));
}

Domain<EasingFunction> ValidEasingFunction() {
  return StructOf<EasingFunction>(VariantOf(
      ArbitraryEasingFunctionPredefined(), ValidEasingFunctionCubicBezier(),
      ValidEasingFunctionLinear(), ValidEasingFunctionSteps()));
}

}  // namespace ink
