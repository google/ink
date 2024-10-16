#include "ink/geometry/internal/polyline_processing.h"

#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "ink/geometry/point.h"
#include "ink/geometry/segment.h"
#include "ink/geometry/type_matchers.h"

namespace ink::geometry_internal {

namespace {

PolylineData CreateWalkingPolylineData() {
  std::vector<Point> walking_points = {
      Point{3, 3},   Point{3, 10}, Point{3, 20}, Point{10, 20},
      Point{10, 15}, Point{5, 15}, Point{5, 0},  Point{2, 0}};
  // Individual Segment lengths: 7, 10, 7, 5, 5, 15, 3
  return CreateNewPolylineData(walking_points);
}

TEST(PolylineProcessingTest, WalkDistanceReturnsCorrectValuesAtEndPoints) {
  PolylineData walking_polyline = CreateWalkingPolylineData();
  EXPECT_EQ(WalkDistance(walking_polyline, 0, 0.0f, false), 0.0f);
  EXPECT_EQ(WalkDistance(walking_polyline, 6, 1.0f, true), 0.0f);
  EXPECT_EQ(WalkDistance(walking_polyline, 0, 0.0f, true), 52.0f);
  EXPECT_EQ(WalkDistance(walking_polyline, 6, 1.0f, false), 52.0f);
}

TEST(PolylineProcessingTest, WalkDistanceReturnsCorrectValuesAtMidPoints) {
  PolylineData walking_polyline = CreateWalkingPolylineData();
  EXPECT_EQ(WalkDistance(walking_polyline, 0, 0.5f, false), 3.5f);
  EXPECT_EQ(WalkDistance(walking_polyline, 0, 0.5f, true), 48.5f);

  EXPECT_EQ(WalkDistance(walking_polyline, 1, 0.4f, false), 11.0f);
  EXPECT_EQ(WalkDistance(walking_polyline, 1, 0.4f, true), 41.0f);

  EXPECT_EQ(WalkDistance(walking_polyline, 5, 0.6f, false), 43.0f);
  EXPECT_EQ(WalkDistance(walking_polyline, 5, 0.6f, true), 9.0f);

  EXPECT_EQ(WalkDistance(walking_polyline, 6, 0.5f, false), 50.5f);
  EXPECT_EQ(WalkDistance(walking_polyline, 6, 0.5f, true), 1.5f);
}

TEST(PolylineProcessingTest,
     WalkDistanceReturnsEqualValuesForEquivalentIndices) {
  PolylineData walking_polyline = CreateWalkingPolylineData();
  EXPECT_EQ(WalkDistance(walking_polyline, 2, 1.0f, false), 24.0f);
  EXPECT_EQ(WalkDistance(walking_polyline, 2, 1.0f, true), 28.0f);

  EXPECT_EQ(WalkDistance(walking_polyline, 3, 0.0f, false), 24.0f);
  EXPECT_EQ(WalkDistance(walking_polyline, 3, 0.0f, true), 28.0f);
}

TEST(PolylineProcessingTest, WalkDistanceReturnsEqualValuesAtMidpoint) {
  PolylineData walking_polyline = CreateWalkingPolylineData();
  EXPECT_EQ(WalkDistance(walking_polyline, 3, 0.4f, false), 26.0f);
  EXPECT_EQ(WalkDistance(walking_polyline, 3, 0.4f, true), 26.0f);
}

TEST(PolylineProcessingTest,
     CreateNewPolylineDataCreatesCorrectSegmentBundles) {
  PolylineData walking_polyline = CreateWalkingPolylineData();
  EXPECT_EQ(walking_polyline.segments.size(), 7);

  EXPECT_EQ(walking_polyline.segments[0].index, 0);
  EXPECT_EQ(walking_polyline.segments[0].length, 7);
  EXPECT_THAT(walking_polyline.segments[0].segment,
              SegmentEq(Segment{{3, 3}, {3, 10}}));

  EXPECT_EQ(walking_polyline.segments[3].index, 3);
  EXPECT_EQ(walking_polyline.segments[3].length, 5);
  EXPECT_THAT(walking_polyline.segments[3].segment,
              SegmentEq(Segment{{10, 20}, {10, 15}}));

  EXPECT_EQ(walking_polyline.segments[5].index, 5);
  EXPECT_EQ(walking_polyline.segments[5].length, 15);
  EXPECT_THAT(walking_polyline.segments[5].segment,
              SegmentEq(Segment{{5, 15}, {5, 0}}));

  EXPECT_EQ(walking_polyline.segments[6].index, 6);
  EXPECT_EQ(walking_polyline.segments[6].length, 3);
  EXPECT_THAT(walking_polyline.segments[6].segment,
              SegmentEq(Segment{{5, 0}, {2, 0}}));
}

}  // namespace
}  // namespace ink::geometry_internal
