#include "ink/strokes/internal/brush_tip_extrusion.h"

#include "gtest/gtest.h"
#include "ink/geometry/angle.h"
#include "ink/geometry/internal/circle.h"
#include "ink/strokes/internal/brush_tip_state.h"

namespace ink::strokes_internal {
namespace {

constexpr float kEpsilon = 1e-5;
constexpr float kTravelThreshold = kEpsilon * 0.1;

TEST(BrushTipExtrusionTest, EvaluateTangentQualityTwoNonOverlappingCircles) {
  EXPECT_EQ(
      BrushTipExtrusion::EvaluateTangentQuality(
          {{.position = {0, 0}, .width = 1, .height = 1, .corner_rounding = 1},
           kEpsilon},
          {{.position = {2, 1}, .width = 1, .height = 1, .corner_rounding = 1},
           kEpsilon},
          kTravelThreshold),
      BrushTipExtrusion::TangentQuality::kGoodTangents);
}

TEST(BrushTipExtrusionTest, EvaluateTangentQualityTwoNonOverlappingSquares) {
  EXPECT_EQ(BrushTipExtrusion::EvaluateTangentQuality(
                {{.position = {0, 0}, .width = 1, .height = 1}, kEpsilon},
                {{.position = {2, 1}, .width = 1, .height = 1}, kEpsilon},
                kTravelThreshold),
            BrushTipExtrusion::TangentQuality::kGoodTangents);
}

TEST(BrushTipExtrusionTest, EvaluateTangentQualityTwoNonOverlappingStadiums) {
  EXPECT_EQ(BrushTipExtrusion::EvaluateTangentQuality({{.position = {0, 0},
                                                        .width = 1,
                                                        .height = 1,
                                                        .corner_rounding = 0.5},
                                                       kEpsilon},
                                                      {{.position = {2, 1},
                                                        .width = 1,
                                                        .height = 1,
                                                        .corner_rounding = 0.5},
                                                       kEpsilon},
                                                      kTravelThreshold),
            BrushTipExtrusion::TangentQuality::kGoodTangents);
}

TEST(BrushTipExtrusionTest,
     EvaluateTangentQualityTwoTrapezoidsPointingInOppositeDirections) {
  EXPECT_EQ(
      BrushTipExtrusion::EvaluateTangentQuality({{.position = {-1, 0},
                                                  .width = 1,
                                                  .height = 1,
                                                  .rotation = -kQuarterTurn,
                                                  .pinch = 0.5},
                                                 kTravelThreshold},
                                                {{.position = {1, 0},
                                                  .width = 1,
                                                  .height = 1,
                                                  .rotation = kQuarterTurn,
                                                  .pinch = 0.5},
                                                 kEpsilon},
                                                kTravelThreshold),
      BrushTipExtrusion::TangentQuality::kGoodTangents);
}

TEST(BrushTipExtrusionTest,
     EvaluateTangentQualityTwoRoundedTrapezoidsPointingInOppositeDirections) {
  EXPECT_EQ(
      BrushTipExtrusion::EvaluateTangentQuality({{.position = {-1, 0},
                                                  .width = 1,
                                                  .height = 1,
                                                  .corner_rounding = 0.5,
                                                  .rotation = -kQuarterTurn,
                                                  .pinch = 0.5},
                                                 kTravelThreshold},
                                                {{.position = {1, 0},
                                                  .width = 1,
                                                  .height = 1,
                                                  .corner_rounding = 0.5,
                                                  .rotation = kQuarterTurn,
                                                  .pinch = 0.5},
                                                 kEpsilon},
                                                kTravelThreshold),
      BrushTipExtrusion::TangentQuality::kGoodTangents);
}

TEST(BrushTipExtrusionTest, JoinedShapeTwoPillsAtRightAngles) {
  EXPECT_EQ(
      BrushTipExtrusion::EvaluateTangentQuality(
          {{.position = {0, 0}, .width = 2, .height = 1, .corner_rounding = 1},
           kEpsilon},
          {{.position = {2, 0},
            .width = 2,
            .height = 1,
            .corner_rounding = 1,
            .rotation = kQuarterTurn},
           kEpsilon},
          kTravelThreshold),
      BrushTipExtrusion::TangentQuality::kGoodTangents);
}

TEST(BrushTipExtrusionTest,
     EvaluateTangentQualityCrossedRectanglesFirstShapeNotFullyCovered) {
  EXPECT_EQ(BrushTipExtrusion::EvaluateTangentQuality(
                {{.position = {0, 0}, .width = 3, .height = 1}, kEpsilon},
                {{.position = {0.5, 0}, .width = 1, .height = 2}, kEpsilon},
                kTravelThreshold),
            BrushTipExtrusion::TangentQuality::
                kBadTangentsJoinedShapeDoesNotCoverInputShapes);
}

TEST(BrushTipExtrusionTest,
     EvaluateTangentQualityCrossedRectanglesSecondShapeNotFullyCovered) {
  EXPECT_EQ(BrushTipExtrusion::EvaluateTangentQuality(
                {{.position = {0, 0}, .width = 1, .height = 2}, kEpsilon},
                {{.position = {0.5, 0}, .width = 3, .height = 1}, kEpsilon},
                kTravelThreshold),
            BrushTipExtrusion::TangentQuality::
                kBadTangentsJoinedShapeDoesNotCoverInputShapes);
}

TEST(BrushTipExtrusionTest, EvaluateTangentQualityFirstShapeContainsSecond) {
  EXPECT_EQ(BrushTipExtrusion::EvaluateTangentQuality(
                {{.position = {0, 0}, .width = 4, .height = 4}, kEpsilon},
                {{.position = {0.5, 0}, .width = 1, .height = 1}, kEpsilon},
                kTravelThreshold),
            BrushTipExtrusion::TangentQuality::kNoTangentsFirstContainsSecond);
}

TEST(BrushTipExtrusionTest, EvaluateTangentQualitySecondShapeContainsFirst) {
  EXPECT_EQ(BrushTipExtrusion::EvaluateTangentQuality(
                {{.position = {0.5, 0}, .width = 1, .height = 1}, kEpsilon},
                {{.position = {0, 0}, .width = 4, .height = 4}, kEpsilon},
                kTravelThreshold),
            BrushTipExtrusion::TangentQuality::kNoTangentsSecondContainsFirst);
}

TEST(BrushTipExtrusionTest,
     EvaluateTangentQualityAccountsForFloatingPointPrecision) {
  EXPECT_EQ(
      BrushTipExtrusion::EvaluateTangentQuality(
          {{.position = {1, 2},
            .width = 5,
            .height = 7,
            .rotation = kFullTurn / 8},
           kEpsilon},
          {{.position = {1.1818182, 2.1818182},
            .width = 5,
            .height = 7,
            .rotation = kFullTurn / 8},
           kEpsilon},
          kTravelThreshold),
      // Without the offset to the `RoundedPolygon` in `EvaluateTangentQuality`,
      // this would be `kBadTangentsJoinedShapeDoesNotCoverInputShapes`, as one
      // of the corners of the second shape lies ~1e-7 units outside the joined
      // shape due to floating-point precision loss.
      BrushTipExtrusion::TangentQuality::kGoodTangents);
}

TEST(BrushTipExtrusionTest,
     EvaluateTangentQualityPaddedContainmentCheckNoCrash) {
  // This tests a numerical precision issue that causes a crash in
  // EvaluateTangentQuality, (see e.g. b/523190724). The root of the issue is
  // that in finite precision, uniformly expanding circles does not preserve
  // their containment relationship. In other words, while it's true in exact
  // arithmetic that if two circles don't contain each other then the uniformly
  // expanded circles also don't contain each other, this isn't necessarily true
  // in finite precision arithmetic.

  geometry_internal::Circle c1({21099998.0f, 0.0f}, 30000002.0f);
  geometry_internal::Circle c2({51100000.0f, 0.0f}, 1.0f);

  float pad = 1.0e-6f * 51100002.0f;
  // Inflate c1 and c2 by `pad`
  geometry_internal::Circle padded_c1(c1.Center(), c1.Radius() + pad);
  geometry_internal::Circle padded_c2(c2.Center(), c2.Radius() + pad);

  EXPECT_FALSE(c1.Contains(c2));
  EXPECT_TRUE(padded_c1.Contains(padded_c2));

  // This previously caused EvaluateTangentQuality to crash, due to a check in
  // the RoundedPolygon constructor.
  BrushTipState t1 = {
      .position = {21099998.0f, 0.0f},
      .width = 60000004.0f,
      .height = 60000004.0f,
      .corner_rounding = 1.0f,
  };
  BrushTipState t2 = {
      .position = {51100001.0f, 0.0f},
      .width = 4.0f,
      .height = 2.0f,
      .corner_rounding = 1.0f,
      .rotation = kQuarterTurn / 2.0f,
  };

  // EvaluateTangentQuality should no longer crash on these tips.
  EXPECT_EQ(BrushTipExtrusion::EvaluateTangentQuality(
                {t1, kEpsilon}, {t2, kEpsilon}, kTravelThreshold),
            BrushTipExtrusion::TangentQuality::kGoodTangents);
}

}  // namespace
}  // namespace ink::strokes_internal
