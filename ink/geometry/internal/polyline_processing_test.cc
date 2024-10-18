#include "ink/geometry/internal/polyline_processing.h"

#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "ink/geometry/internal/static_rtree.h"
#include "ink/geometry/point.h"
#include "ink/geometry/rect.h"
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

// function for use in rtree traversals.
auto segment_bounds = [](SegmentBundle segment_data) {
  return Rect::FromTwoPoints(segment_data.segment.start,
                             segment_data.segment.end);
};

PolylineData CreatePolylineAndFindIntersections(std::vector<Point> points) {
  PolylineData output_polyline = CreateNewPolylineData(points);
  ink::geometry_internal::StaticRTree<SegmentBundle> rtree(
      output_polyline.segments, segment_bounds);
  FindFirstAndLastIntersections(rtree, output_polyline);
  return output_polyline;
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

TEST(PolylineProcessingTest, IntersectionsForPerfectlyClosedLoop) {
  std::vector<Point> points = {
      Point{5, 3},   Point{5, 8},   Point{5, 15},  Point{10, 20}, Point{15, 25},
      Point{20, 30}, Point{30, 20}, Point{20, 10}, Point{11, 3},  Point{5, 3}};
  PolylineData polyline = CreatePolylineAndFindIntersections(points);

  EXPECT_EQ(polyline.has_intersection, true);

  EXPECT_EQ(polyline.first_intersection.index_int, 0);
  EXPECT_EQ(polyline.first_intersection.index_fraction, 0.0f);
  EXPECT_THAT(polyline.new_first_point, PointEq(Point{5, 3}));

  EXPECT_EQ(polyline.last_intersection.index_int, 8);
  EXPECT_EQ(polyline.last_intersection.index_fraction, 1.0f);
  EXPECT_THAT(polyline.new_last_point, PointEq(Point{5, 3}));
}

TEST(PolylineProcessingTest, IntersectionsWithExplicitIntersectionPoint) {
  std::vector<Point> points = {
      Point{-8, 25}, Point{-3, 23}, Point{2, 21},  Point{6, 19},  Point{10, 17},
      Point{14, 15}, Point{18, 9},  Point{16, 3},  Point{10, 0},  Point{4, 3},
      Point{2, 9},   Point{6, 15},  Point{10, 17}, Point{14, 19}, Point{19, 22},
      Point{24, 24}, Point{30, 26}};
  PolylineData polyline = CreatePolylineAndFindIntersections(points);

  EXPECT_EQ(polyline.has_intersection, true);

  EXPECT_EQ(polyline.first_intersection.index_int, 3);
  EXPECT_EQ(polyline.first_intersection.index_fraction, 1.0f);
  EXPECT_THAT(polyline.new_first_point, PointEq(Point{10, 17}));

  EXPECT_EQ(polyline.last_intersection.index_int, 12);
  EXPECT_EQ(polyline.last_intersection.index_fraction, 0.0f);
  EXPECT_THAT(polyline.new_last_point, PointEq(Point{10, 17}));
}

TEST(PolylineProcessingTest, IntersectionsWithImplicitIntersectionPoint) {
  std::vector<Point> points = {Point{-8, 25}, Point{-3, 23}, Point{2, 21},
                               Point{6, 19},  Point{14, 15}, Point{18, 9},
                               Point{16, 3},  Point{10, 0},  Point{4, 3},
                               Point{2, 9},   Point{6, 15},  Point{14, 19},
                               Point{19, 22}, Point{24, 24}, Point{30, 26}};
  PolylineData polyline = CreatePolylineAndFindIntersections(points);

  EXPECT_EQ(polyline.has_intersection, true);

  EXPECT_EQ(polyline.first_intersection.index_int, 3);
  EXPECT_EQ(polyline.first_intersection.index_fraction, 0.5f);
  EXPECT_THAT(polyline.new_first_point, PointEq(Point{10, 17}));

  EXPECT_EQ(polyline.last_intersection.index_int, 10);
  EXPECT_EQ(polyline.last_intersection.index_fraction, 0.5f);
  EXPECT_THAT(polyline.new_last_point, PointEq(Point{10, 17}));
}

TEST(PolylineProcessingTest, IntersectionsWithUnconnectedLineReturnsFalse) {
  std::vector<Point> points = {Point{-1, 0}, Point{5, 1},  Point{10, 2},
                               Point{15, 4}, Point{20, 6}, Point{25, 9}};
  PolylineData polyline = CreatePolylineAndFindIntersections(points);

  EXPECT_EQ(polyline.has_intersection, false);
}

TEST(PolylineProcessingTest,
     IntersectionsWithCloseButUnconnectedLineReturnsFalse) {
  std::vector<Point> points = {
      Point{19, 22}, Point{14, 19}, Point{10.05f, 17}, Point{14, 15},
      Point{18, 9},  Point{16, 3},  Point{10, 0},      Point{4, 3},
      Point{2, 9},   Point{6, 15},  Point{9.95f, 17},  Point{6, 19},
      Point{2, 21}};
  PolylineData polyline = CreatePolylineAndFindIntersections(points);

  EXPECT_EQ(polyline.has_intersection, false);
}

TEST(PolylineProcessingTest,
     IntersectionsWithExplictlyOverlappingLineSegments) {
  std::vector<Point> points = {
      Point{18, 21}, Point{14, 19}, Point{12, 17}, Point{8, 17}, Point{6, 15},
      Point{2, 9},   Point{4, 3},   Point{10, 0},  Point{16, 3}, Point{18, 9},
      Point{14, 15}, Point{12, 17}, Point{8, 17},  Point{6, 19}, Point{2, 21}};
  PolylineData polyline = CreatePolylineAndFindIntersections(points);

  EXPECT_EQ(polyline.has_intersection, true);

  EXPECT_EQ(polyline.first_intersection.index_int, 1);
  EXPECT_EQ(polyline.first_intersection.index_fraction, 1.0f);
  EXPECT_THAT(polyline.new_first_point, PointEq(Point{12, 17}));

  EXPECT_EQ(polyline.last_intersection.index_int, 12);
  EXPECT_EQ(polyline.last_intersection.index_fraction, 0.0f);
  EXPECT_THAT(polyline.new_last_point, PointEq(Point{8, 17}));
}

TEST(PolylineProcessingTest,
     IntersectionsWithImplicitlyOverlappingLineSegments) {
  std::vector<Point> points = {
      Point{18, 21}, Point{14, 19}, Point{12, 17}, Point{8, 17}, Point{6, 15},
      Point{2, 9},   Point{4, 3},   Point{10, 0},  Point{16, 3}, Point{18, 9},
      Point{14, 15}, Point{11, 17}, Point{9, 17},  Point{6, 19}, Point{2, 21}};
  PolylineData polyline = CreatePolylineAndFindIntersections(points);

  EXPECT_EQ(polyline.has_intersection, true);

  EXPECT_EQ(polyline.first_intersection.index_int, 2);
  EXPECT_EQ(polyline.first_intersection.index_fraction, 0.25f);
  EXPECT_THAT(polyline.new_first_point, PointEq(Point{11, 17}));

  EXPECT_EQ(polyline.last_intersection.index_int, 12);
  EXPECT_EQ(polyline.last_intersection.index_fraction, 0.0f);
  EXPECT_THAT(polyline.new_last_point, PointEq(Point{9, 17}));
}

TEST(PolylineProcessingTest, IntersectionsWithMultipleIntersections) {
  std::vector<Point> points = {Point{4, 9},    Point{2.5, 9},   Point{1.5, 9},
                               Point{5.5, 15}, Point{10, 17},   Point{14, 15},
                               Point{18, 9},   Point{16, 3},    Point{10, 0},
                               Point{4, 3},    Point{2, 9},     Point{6, 15},
                               Point{10, 17},  Point{14.5, 15}, Point{18.5, 9},
                               Point{17.5, 9}, Point{16, 9}};
  PolylineData polyline = CreatePolylineAndFindIntersections(points);

  EXPECT_EQ(polyline.has_intersection, true);

  EXPECT_EQ(polyline.first_intersection.index_int, 1);
  EXPECT_EQ(polyline.first_intersection.index_fraction, 0.5f);
  EXPECT_THAT(polyline.new_first_point, PointEq(Point{2, 9}));

  EXPECT_EQ(polyline.last_intersection.index_int, 14);
  EXPECT_EQ(polyline.last_intersection.index_fraction, 0.5f);
  EXPECT_THAT(polyline.new_last_point, PointEq(Point{18, 9}));
}

}  // namespace
}  // namespace ink::geometry_internal
