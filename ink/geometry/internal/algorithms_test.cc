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

#include "ink/geometry/internal/algorithms.h"

#include <optional>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/container/inlined_vector.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/types/span.h"
#include "ink/geometry/affine_transform.h"
#include "ink/geometry/angle.h"
#include "ink/geometry/envelope.h"
#include "ink/geometry/mesh.h"
#include "ink/geometry/mesh_test_helpers.h"
#include "ink/geometry/mutable_mesh.h"
#include "ink/geometry/point.h"
#include "ink/geometry/rect.h"
#include "ink/geometry/segment.h"
#include "ink/geometry/triangle.h"
#include "ink/geometry/type_matchers.h"
#include "ink/geometry/vec.h"

namespace ink {
namespace geometry_internal {
namespace {

using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::FloatEq;
using ::testing::FloatNear;
using ::testing::Optional;
using ::testing::Pair;

TEST(GetBarycentricCoordinatesTest, NonDegenerateTriangle) {
  Triangle triangle{.p0 = {1, 2}, .p1 = {4, 2}, .p2 = {1, 8}};

  EXPECT_THAT(GetBarycentricCoordinates(triangle, triangle.p0),
              Optional(ElementsAre(FloatNear(1, 1e-6), FloatNear(0, 1e-6),
                                   FloatNear(0, 1e-6))));

  EXPECT_THAT(GetBarycentricCoordinates(triangle, triangle.p1),
              Optional(ElementsAre(FloatNear(0, 1e-6), FloatNear(1, 1e-6),
                                   FloatNear(0, 1e-6))));

  EXPECT_THAT(GetBarycentricCoordinates(triangle, triangle.p2),
              Optional(ElementsAre(FloatNear(0, 1e-6), FloatNear(0, 1e-6),
                                   FloatNear(1, 1e-6))));

  EXPECT_THAT(GetBarycentricCoordinates(triangle, {2.5, 2}),
              Optional(ElementsAre(FloatNear(0.5, 1e-6), FloatNear(0.5, 1e-6),
                                   FloatNear(0, 1e-6))));

  EXPECT_THAT(GetBarycentricCoordinates(triangle, {2.5, 5}),
              Optional(ElementsAre(FloatNear(0, 1e-6), FloatNear(0.5, 1e-6),
                                   FloatNear(0.5, 1e-6))));

  EXPECT_THAT(
      GetBarycentricCoordinates(triangle, {2, 4}),
      Optional(ElementsAre(FloatNear(1. / 3, 1e-6), FloatNear(1. / 3, 1e-6),
                           FloatNear(1. / 3, 1e-6))));

  EXPECT_THAT(GetBarycentricCoordinates(triangle, {1.75, 3.5}),
              Optional(ElementsAre(FloatNear(0.5, 1e-6), FloatNear(0.25, 1e-6),
                                   FloatNear(0.25, 1e-6))));

  EXPECT_THAT(GetBarycentricCoordinates(triangle, {3.25, 9.5}),
              Optional(ElementsAre(FloatNear(-1, 1e-6), FloatNear(0.75, 1e-6),
                                   FloatNear(1.25, 1e-6))));
}

TEST(GetBarycentricCoordinatesTest, DegenerateTriangle) {
  EXPECT_THAT(GetBarycentricCoordinates(
                  {.p0 = {1, 2}, .p1 = {1, 2}, .p2 = {1, 8}}, {0, 0}),
              Eq(std::nullopt));
  EXPECT_THAT(GetBarycentricCoordinates(
                  {.p0 = {1, 2}, .p1 = {4, 2}, .p2 = {1, 2}}, {0, 0}),
              Eq(std::nullopt));
  EXPECT_THAT(GetBarycentricCoordinates(
                  {.p0 = {1, 2}, .p1 = {4, 2}, .p2 = {4, 2}}, {0, 0}),
              Eq(std::nullopt));
  EXPECT_THAT(GetBarycentricCoordinates(
                  {.p0 = {1, 2}, .p1 = {1, 2}, .p2 = {1, 2}}, {0, 0}),
              Eq(std::nullopt));
  EXPECT_THAT(GetBarycentricCoordinates(
                  {.p0 = {1, 2}, .p1 = {4, 2}, .p2 = {8, 2}}, {0, 0}),
              Eq(std::nullopt));
  EXPECT_THAT(GetBarycentricCoordinates(
                  {.p0 = {1, 2}, .p1 = {5, 8}, .p2 = {3, 5}}, {0, 0}),
              Eq(std::nullopt));
}

TEST(CalculateEnvelopeTest, EmptyMesh) {
  MutableMesh mesh;
  EXPECT_TRUE(CalculateEnvelope(mesh).IsEmpty());
}

TEST(CalculateEnvelopeTest, NonEmptyMesh) {
  MutableMesh mesh;
  mesh.AppendVertex({1, 2});
  mesh.AppendVertex({3, -4});
  mesh.AppendVertex({-5, 0});
  mesh.AppendVertex({7, -1});
  EXPECT_THAT(
      CalculateEnvelope(mesh).AsRect(),
      Optional(RectNear(Rect::FromTwoPoints({-5, -4}, {7, 2}), 0.0001)));
}

TEST(VectorFromPointToSegmentProjectionTest, PointAwayFromDegenerateSegment) {
  EXPECT_EQ(VectorFromPointToSegmentProjection(
                {0, 0}, {.start = {5, 7}, .end = {5, 7}}),
            std::nullopt);
}

TEST(VectorFromPointToSegmentProjectionTest, PointOnDegenerateSegment) {
  EXPECT_EQ(VectorFromPointToSegmentProjection(
                {-1, -2}, {.start = {-1, -2}, .end = {-1, -2}}),
            std::nullopt);
}

TEST(VectorFromPointToSegmentProjectionTest, PointOnNonDegenerateSegmentLine) {
  Segment segment = {.start = {2, 3}, .end = {4, -1}};

  EXPECT_THAT(VectorFromPointToSegmentProjection(segment.Lerp(-1), segment),
              Optional(VecEq({0, 0})));
  EXPECT_THAT(VectorFromPointToSegmentProjection(segment.Lerp(0), segment),
              Optional(VecEq({0, 0})));
  EXPECT_THAT(VectorFromPointToSegmentProjection(segment.Lerp(0.25), segment),
              Optional(VecEq({0, 0})));
  EXPECT_THAT(VectorFromPointToSegmentProjection(segment.Lerp(1), segment),
              Optional(VecEq({0, 0})));
  EXPECT_THAT(VectorFromPointToSegmentProjection(segment.Lerp(2), segment),
              Optional(VecEq({0, 0})));
}

TEST(VectorFromPointToSegmentProjectionTest,
     PointAndSegmentFormNonDegenerateTriangle) {
  Segment segment = {.start = {0, -3}, .end = {4, -1}};

  Point point = {5, 3};
  EXPECT_THAT(
      VectorFromPointToSegmentProjection(point, segment),
      Optional(VecNear(segment.Lerp(*segment.Project(point)) - point, 0.0001)));
  point = {2, -6};
  EXPECT_THAT(
      VectorFromPointToSegmentProjection(point, segment),
      Optional(VecNear(segment.Lerp(*segment.Project(point)) - point, 0.0001)));
}

TEST(SegmentIntersectionRatioTest, NoIntersection) {
  // These segments cover all three non-intersection cases: skew but not
  // touching, parallel but offset, and parallel on the same line but
  // non-overlapping. They're arranged like so (the name is the start point, the
  // 'x' is the end point):
  //           c
  //          /
  //         x
  //
  //   x   x
  //  /   /
  // d   a   b-x
  Segment a{{0, 0}, {1, 1}};
  Segment b{{2, 0}, {3, 0}};
  Segment c{{3, 3}, {2, 2}};
  Segment d{{-2, 0}, {-1, 1}};

  EXPECT_EQ(SegmentIntersectionRatio(a, b), std::nullopt);
  EXPECT_EQ(SegmentIntersectionRatio(b, a), std::nullopt);
  EXPECT_EQ(SegmentIntersectionRatio(a, c), std::nullopt);
  EXPECT_EQ(SegmentIntersectionRatio(c, a), std::nullopt);
  EXPECT_EQ(SegmentIntersectionRatio(a, d), std::nullopt);
  EXPECT_EQ(SegmentIntersectionRatio(d, a), std::nullopt);
  EXPECT_EQ(SegmentIntersectionRatio(b, c), std::nullopt);
  EXPECT_EQ(SegmentIntersectionRatio(c, b), std::nullopt);
  EXPECT_EQ(SegmentIntersectionRatio(b, d), std::nullopt);
  EXPECT_EQ(SegmentIntersectionRatio(d, b), std::nullopt);
  EXPECT_EQ(SegmentIntersectionRatio(c, d), std::nullopt);
  EXPECT_EQ(SegmentIntersectionRatio(d, c), std::nullopt);
}

TEST(SegmentIntersectionRatioTest, NoIntersectionCollinear) {
  Segment a{{0, 0}, {4, 0}};
  Segment b{{5, 0}, {6, 0}};
  // Cover the wholly-before and wholly-after logic.
  EXPECT_EQ(SegmentIntersectionRatio(a, b), std::nullopt);
  EXPECT_EQ(SegmentIntersectionRatio(b, a), std::nullopt);
}

TEST(SegmentIntersectionRatioTest, SimpleIntersection) {
  // These segments all intersect, and are arranged like so (the name is the
  // start point, the 'x' is the end point):
  //   x
  //   |
  // c |
  //  \|
  //   X
  //   |\
  //   | \
  //   |  \
  // a-+---X-----x
  //   |    \
  //   b     x
  Segment a{{0, 0}, {6, 0}};
  Segment b{{1, -1}, {1, 4}};
  Segment c{{0, 3}, {4, -1}};

  EXPECT_THAT(SegmentIntersectionRatio(a, b),
              Optional(Pair(FloatEq(1. / 6), FloatEq(0.2))));
  EXPECT_THAT(SegmentIntersectionRatio(b, a),
              Optional(Pair(FloatEq(0.2), FloatEq(1. / 6))));
  EXPECT_THAT(SegmentIntersectionRatio(a, c),
              Optional(Pair(FloatEq(0.5), FloatEq(0.75))));
  EXPECT_THAT(SegmentIntersectionRatio(c, a),
              Optional(Pair(FloatEq(0.75), FloatEq(0.5))));
  EXPECT_THAT(SegmentIntersectionRatio(b, c),
              Optional(Pair(FloatEq(0.6), FloatEq(0.25))));
  EXPECT_THAT(SegmentIntersectionRatio(c, b),
              Optional(Pair(FloatEq(0.25), FloatEq(0.6))));
}

TEST(SegmentIntersectionRatioTest, EndpointIntersection) {
  // These segments all intersect at their start or end points, and are arranged
  // like so (the name is the start point, the 'x' is the end point):
  //    xx
  //   /  \
  //  /    \
  // b      c
  //  a----x
  Segment a{{0, 0}, {4, 0}};
  Segment b{{0, 0}, {2, 2}};
  Segment c{{4, 0}, {2, 2}};

  EXPECT_THAT(SegmentIntersectionRatio(a, b),
              Optional(Pair(FloatEq(0), FloatEq(0))));
  EXPECT_THAT(SegmentIntersectionRatio(b, a),
              Optional(Pair(FloatEq(0), FloatEq(0))));
  EXPECT_THAT(SegmentIntersectionRatio(a, c),
              Optional(Pair(FloatEq(1), FloatEq(0))));
  EXPECT_THAT(SegmentIntersectionRatio(c, a),
              Optional(Pair(FloatEq(0), FloatEq(1))));
  EXPECT_THAT(SegmentIntersectionRatio(b, c),
              Optional(Pair(FloatEq(1), FloatEq(1))));
  EXPECT_THAT(SegmentIntersectionRatio(c, b),
              Optional(Pair(FloatEq(1), FloatEq(1))));
}

TEST(SegmentIntersectionRatioTest, Overlap) {
  // These segments all overlap along the same line, and cover the five overlap
  // cases:
  // - Aligned segments, one contains the other
  // - Aligned segments, the end of one overlaps the start of the other
  // - Reversed segments, one contains the other
  // - Reversed segments, the start of one overlaps the start of the other
  // - Reversed segments, the end of one overlaps the end of the other
  // By "aligned", we mean that the segments point in the same direction;
  // likewise, "reversed" segments point in opposite directions. Each of these
  // cases can also be inverted, swapping which segment is first or second, to
  // give us ten cases total.
  // The segments all lie on the line y = x/2, and are arranged like so (the
  // name is the start point, the 'x' is the end point):
  // a---------x
  //   x---------b
  //     c---------x
  //       d-x
  Segment a{{0, 0}, {10, 5}};
  Segment b{{12, 6}, {2, 1}};
  Segment c{{4, 2}, {14, 7}};
  Segment d{{6, 3}, {8, 4}};

  EXPECT_THAT(SegmentIntersectionRatio(a, b),
              Optional(Pair(FloatEq(0.2), FloatEq(0.2))));
  EXPECT_THAT(SegmentIntersectionRatio(b, a),
              Optional(Pair(FloatEq(0.2), FloatEq(0.2))));
  EXPECT_THAT(SegmentIntersectionRatio(a, c),
              Optional(Pair(FloatEq(0.4), FloatEq(0))));
  EXPECT_THAT(SegmentIntersectionRatio(c, a),
              Optional(Pair(FloatEq(0), FloatEq(0.4))));
  EXPECT_THAT(SegmentIntersectionRatio(a, d),
              Optional(Pair(FloatEq(0.6), FloatEq(0))));
  EXPECT_THAT(SegmentIntersectionRatio(d, a),
              Optional(Pair(FloatEq(0), FloatEq(0.6))));
  EXPECT_THAT(SegmentIntersectionRatio(b, c),
              Optional(Pair(FloatEq(0), FloatEq(0))));
  EXPECT_THAT(SegmentIntersectionRatio(c, b),
              Optional(Pair(FloatEq(0), FloatEq(0))));
  EXPECT_THAT(SegmentIntersectionRatio(b, d),
              Optional(Pair(FloatEq(0.4), FloatEq(0))));
  EXPECT_THAT(SegmentIntersectionRatio(d, b),
              Optional(Pair(FloatEq(0), FloatEq(0.4))));
  EXPECT_THAT(SegmentIntersectionRatio(c, d),
              Optional(Pair(FloatEq(0.2), FloatEq(0))));
  EXPECT_THAT(SegmentIntersectionRatio(d, c),
              Optional(Pair(FloatEq(0), FloatEq(0.2))));
}

TEST(SegmentIntersectionRatioTest, DegenerateSegments) {
  // Segments 'a', 'b', and 'c' are all degenerate; 'a' and 'b' lie on
  // 'well_formed", but 'c' does not.
  Segment a{{1, 1}, {1, 1}};
  Segment b{{3, 3}, {3, 3}};
  Segment c{{0, 2}, {0, 2}};
  Segment well_formed{{0, 0}, {5, 5}};

  EXPECT_THAT(SegmentIntersectionRatio(a, well_formed),
              Optional(Pair(FloatEq(0), FloatEq(0.2))));
  EXPECT_THAT(SegmentIntersectionRatio(well_formed, a),
              Optional(Pair(FloatEq(0.2), FloatEq(0))));
  EXPECT_THAT(SegmentIntersectionRatio(b, well_formed),
              Optional(Pair(FloatEq(0), FloatEq(0.6))));
  EXPECT_THAT(SegmentIntersectionRatio(well_formed, b),
              Optional(Pair(FloatEq(0.6), FloatEq(0))));
  EXPECT_EQ(SegmentIntersectionRatio(c, well_formed), std::nullopt);
  EXPECT_EQ(SegmentIntersectionRatio(well_formed, c), std::nullopt);
  EXPECT_EQ(SegmentIntersectionRatio(a, b), std::nullopt);
  EXPECT_EQ(SegmentIntersectionRatio(b, a), std::nullopt);
  EXPECT_EQ(SegmentIntersectionRatio(a, c), std::nullopt);
  EXPECT_EQ(SegmentIntersectionRatio(c, a), std::nullopt);
  EXPECT_EQ(SegmentIntersectionRatio(b, c), std::nullopt);
  EXPECT_EQ(SegmentIntersectionRatio(c, b), std::nullopt);
  EXPECT_THAT(SegmentIntersectionRatio(a, a),
              Optional(Pair(FloatEq(0), FloatEq(0))));
}

TEST(UniqueLineIntersectionRatioTest, OneDegenerateSegment) {
  Point a = {0, 2};
  Point b = {-1, 0};
  Point c = {1, 1};
  EXPECT_EQ(UniqueLineIntersectionRatio(Segment{.start = a, .end = b},
                                        Segment{.start = c, .end = c}),
            std::nullopt);
  EXPECT_EQ(UniqueLineIntersectionRatio(Segment{.start = a, .end = a},
                                        Segment{.start = b, .end = c}),
            std::nullopt);
}

TEST(UniqueLineIntersectionRatioTest, TwoDegenerateSegments) {
  Point a = {0, 2};
  Point b = {-1, 0};
  EXPECT_EQ(UniqueLineIntersectionRatio(Segment{.start = a, .end = a},
                                        Segment{.start = b, .end = b}),
            std::nullopt);
}

TEST(UniqueLineIntersectionRatioTest, NonOverlappingParallelSegments) {
  EXPECT_EQ(
      UniqueLineIntersectionRatio(Segment{.start = {0, 1}, .end = {3, 1}},
                                  Segment{.start = {5, 1}, .end = {4, 1}}),
      std::nullopt);
  EXPECT_EQ(
      UniqueLineIntersectionRatio(Segment{.start = {1, 1}, .end = {2, 4}},
                                  Segment{.start = {3, 1}, .end = {4, 4}}),
      std::nullopt);
}

TEST(UniqueLineIntersectionRatioTest, OverlappingParallelSegments) {
  EXPECT_EQ(
      UniqueLineIntersectionRatio(Segment{.start = {3, -1}, .end = {3, 2}},
                                  Segment{.start = {3, 5}, .end = {3, 0}}),
      std::nullopt);
}

TEST(UniqueLineIntersectionRatioTest, IntersectingNonParallelSegments) {
  EXPECT_THAT(
      UniqueLineIntersectionRatio(Segment{.start = {0, 0}, .end = {4, 0}},
                                  Segment{.start = {3, 1}, .end = {3, -3}}),
      Optional(Pair(FloatNear(0.75, 0.0001), FloatNear(0.25, 0.0001))));
}

TEST(UniqueLineIntersectionRatioTest, NonIntersectingNonParallelSegments) {
  EXPECT_THAT(
      UniqueLineIntersectionRatio(Segment{.start = {0, 0}, .end = {-4, 0}},
                                  Segment{.start = {0, 4}, .end = {1, 3}}),
      Optional(Pair(FloatNear(-1, 0.0001), FloatNear(4, 0.0001))));
}

TEST(SegmentIntersectionTest, ReturnsIntersection) {
  Segment a{{0, 0}, {4, 0}};
  Segment b{{0, 0}, {2, 2}};
  EXPECT_THAT(SegmentIntersection(a, b), Optional(PointEq({0, 0})));
}
TEST(SegmentIntersectionTest, ReturnsFirstIntersectionOnFirstSegment) {
  Segment a{{0, 0}, {2, 0}};
  Segment b{{3, 0}, {1, 0}};
  EXPECT_THAT(SegmentIntersection(a, b), Optional(PointEq({1, 0})));
}

TEST(SegmentIntersectionTest, NoIntersection) {
  Segment a{{0, 0}, {4, 0}};
  Segment b{{1, 1}, {2, 2}};
  EXPECT_EQ(SegmentIntersection(a, b), std::nullopt);
}

TEST(CalculateCollapsedSegmentTest, CollapseToPointSingleMesh) {
  absl::StatusOr<absl::InlinedVector<Mesh, 1>> star_meshes =
      MakeStarMutableMesh(6).AsMeshes();
  ASSERT_EQ(star_meshes.status(), absl::OkStatus());

  absl::StatusOr<absl::InlinedVector<Mesh, 1>> line_meshes =
      MakeStraightLineMutableMesh(3).AsMeshes();
  ASSERT_EQ(line_meshes.status(), absl::OkStatus());

  ASSERT_EQ(star_meshes->size(), 1);
  ASSERT_EQ(line_meshes->size(), 1);
  const Mesh& star_mesh = (*star_meshes)[0];
  const Mesh& line_mesh = (*line_meshes)[0];

  // This transform collapses everything to the point (-1, 2).
  EXPECT_THAT(
      CalculateCollapsedSegment({star_mesh}, *star_mesh.Bounds().AsRect(),

                                AffineTransform{0, 0, -1, 0, 0, 2}),
      SegmentEq({{-1, 2}, {-1, 2}}));
  // This transform collapses everything to the point (2, -0.5);
  EXPECT_THAT(
      CalculateCollapsedSegment({line_mesh}, *line_mesh.Bounds().AsRect(),
                                AffineTransform{0, 0, 2, 0, 0, -0.5}),
      SegmentEq({{2, -0.5}, {2, -0.5}}));
}

TEST(CalculateCollapsedSegmentTest, CollapseToPointMultipleMeshes) {
  absl::StatusOr<absl::InlinedVector<Mesh, 1>> star_meshes =
      MakeStarMutableMesh(6).AsMeshes();
  ASSERT_EQ(star_meshes.status(), absl::OkStatus());

  absl::StatusOr<absl::InlinedVector<Mesh, 1>> line_meshes =
      MakeStraightLineMutableMesh(3).AsMeshes();
  ASSERT_EQ(line_meshes.status(), absl::OkStatus());

  ASSERT_EQ(star_meshes->size(), 1);
  ASSERT_EQ(line_meshes->size(), 1);
  std::vector<Mesh> meshes = {(*star_meshes)[0], (*line_meshes)[0]};
  Envelope bounds = meshes[0].Bounds();
  bounds.Add(meshes[1].Bounds());

  // This transform collapses everything to the point (3, 4).
  EXPECT_THAT(CalculateCollapsedSegment(meshes, *bounds.AsRect(),
                                        AffineTransform{0, 0, 3, 0, 0, 4}),
              SegmentEq({{3, 4}, {3, 4}}));
  // This transform collapses everything to the point (-10, 5);
  EXPECT_THAT(CalculateCollapsedSegment(meshes, *bounds.AsRect(),
                                        AffineTransform{0, 0, -10, 0, 0, 5}),
              SegmentEq({{-10, 5}, {-10, 5}}));
}

TEST(CalculateCollapsedSegmentTest, CollapseToSegmentSingleMesh) {
  absl::StatusOr<absl::InlinedVector<Mesh, 1>> star_meshes =
      MakeStarMutableMesh(6).AsMeshes();
  ASSERT_EQ(star_meshes.status(), absl::OkStatus());

  absl::StatusOr<absl::InlinedVector<Mesh, 1>> line_meshes =
      MakeStraightLineMutableMesh(3).AsMeshes();
  ASSERT_EQ(line_meshes.status(), absl::OkStatus());

  ASSERT_EQ(star_meshes->size(), 1);
  ASSERT_EQ(line_meshes->size(), 1);
  const Mesh& star_mesh = (*star_meshes)[0];
  const Mesh& line_mesh = (*line_meshes)[0];

  // This transform collapses everything to the line y = 2x.
  EXPECT_THAT(
      CalculateCollapsedSegment({star_mesh}, *star_mesh.Bounds().AsRect(),
                                AffineTransform{1, 1, 0, 2, 2, 0}),
      SegmentNear({{-1.366, -2.732}, {1.366, 2.732}}, 1e-3));
  EXPECT_THAT(
      CalculateCollapsedSegment({line_mesh}, *line_mesh.Bounds().AsRect(),
                                AffineTransform{1, 1, 0, 2, 2, 0}),
      SegmentEq({{0, 0}, {4, 8}}));
  // This transform collapses the everything to the line y = (x - 10) / 3.
  EXPECT_THAT(
      CalculateCollapsedSegment({star_mesh}, *star_mesh.Bounds().AsRect(),
                                AffineTransform{1.5, 1.5, 1, .5, .5, -3}),
      SegmentNear({{-1.049, -3.683}, {3.049, -2.317}}, 1e-3));
  EXPECT_THAT(
      CalculateCollapsedSegment({line_mesh}, *line_mesh.Bounds().AsRect(),
                                AffineTransform{1.5, 1.5, 1, .5, .5, -3}),
      SegmentEq({{1, -3}, {7, -1}}));
}

TEST(CalculateCollapsedSegmentTest, CollapseToSegmentMultipleMeshes) {
  absl::StatusOr<absl::InlinedVector<Mesh, 1>> star_meshes =
      MakeStarMutableMesh(6).AsMeshes();
  ASSERT_EQ(star_meshes.status(), absl::OkStatus());

  absl::StatusOr<absl::InlinedVector<Mesh, 1>> line_meshes =
      MakeStraightLineMutableMesh(3).AsMeshes();
  ASSERT_EQ(line_meshes.status(), absl::OkStatus());

  ASSERT_EQ(star_meshes->size(), 1);
  ASSERT_EQ(line_meshes->size(), 1);
  std::vector<Mesh> meshes = {(*star_meshes)[0], (*line_meshes)[0]};
  Envelope bounds = meshes[0].Bounds();
  bounds.Add(meshes[1].Bounds());

  // This transform collapses everything to the line y = 1.5x + 1.
  EXPECT_THAT(
      CalculateCollapsedSegment(meshes, *bounds.AsRect(),
                                AffineTransform{.8, .8, 0, 1.2, 1.2, 1}),
      SegmentNear({{-1.093, -0.639}, {3.2, 5.8}}, 1e-3));
  // This transform collapses everything to the line x = 5.
  EXPECT_THAT(CalculateCollapsedSegment(meshes, *bounds.AsRect(),
                                        AffineTransform{0, 0, 5, 1, 1, 0}),
              SegmentNear({{5, -1.366}, {5, 4}}, 1e-3));
}

}  // namespace
}  // namespace geometry_internal
}  // namespace ink
