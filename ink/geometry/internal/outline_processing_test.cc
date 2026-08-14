// Copyright 2026 Google LLC
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

#include "ink/geometry/internal/outline_processing.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "ink/geometry/internal/test_matchers.h"
#include "ink/geometry/point.h"
#include "ink/geometry/rect.h"
#include "ink/geometry/triangle.h"

namespace ink::geometry_internal {
namespace {

using ::testing::SizeIs;
using ::testing::UnorderedElementsAre;

constexpr float kFloatTolerance = 1e-6f;
constexpr auto FromTwoPoints = Rect::FromTwoPoints;

// Helper function for test: returns true if the point is contained in any of
// the triangles.
bool ContainsPoint(const std::vector<Point>& points,
                   const std::vector<std::array<uint32_t, 3>>& triangles,
                   Point p) {
  return std::any_of(triangles.begin(), triangles.end(),
                     [&points, p](const std::array<uint32_t, 3>& tri) {
                       return Triangle{points[tri[0]], points[tri[1]],
                                       points[tri[2]]}
                           .Contains(p);
                     });
}

// Helper function for test: returns true if all of the indices in the triangles
// are valid indices into the points array.
bool HasValidIndices(const std::vector<Point>& points,
                     const std::vector<std::array<uint32_t, 3>>& triangles) {
  return std::all_of(triangles.begin(), triangles.end(),
                     [&points](const std::array<uint32_t, 3>& tri) {
                       return tri[0] < points.size() &&
                              tri[1] < points.size() && tri[2] < points.size();
                     });
}

TEST(ComputeOutlineTest, BasicTriangle) {
  //         C
  //        / \
  //       /   \
  //      A-----B
  Point A{0, 0}, B{2, 0}, C{1, 1};

  std::vector<std::vector<Point>> loops = {{A, B, C}};

  ShapeOutline outline(loops);

  EXPECT_EQ(outline, ShapeOutline({{{A, C, B}, -1}, {{A, B}, 1}}));
}

TEST(ComputeOutlineTest, SelfIntersection) {
  //       F      C
  //      /  \   /  \
  //     /    \ /    \
  //    /     / \     \
  //   /     D---E     \
  //  /                 \
  // A-------------------B
  Point A{-5, -2}, B{5, -2}, C{2, 4}, D{-2, 0}, E{2, 0}, F{-2, 4};
  Point X1{0, 2};

  std::vector<std::vector<Point>> loops = {{A, B, C, D, E, F}};

  ShapeOutline outline(loops);

  EXPECT_EQ(outline, ShapeOutline({{{A, F, X1, C, B}, -1}, {{A, B}, 1}}));
}

TEST(ComputeOutlineTest, VerticalSegments) {
  // There is some ambiguity in how to handle vertical portions of the boundary
  // in the monotone chains. The approach we take is conceptually to perturb
  // vertical segments to tilt slightly to the upward-right.

  //    D-----------C
  //    |           |
  //    |           |
  //    |           |
  //    |           |
  //    A-----------B
  Point A{0, 0}, B{2, 0}, C{2, 2}, D{0, 2};

  std::vector<std::vector<Point>> loops = {{A, B, C, D}};

  ShapeOutline outline(loops);

  EXPECT_EQ(outline, ShapeOutline({{{A, D, C}, -1}, {{A, B, C}, 1}}));
}

TEST(ComputeOutlineTest, SumTwoDiamonds) {
  //         D     H
  //        / \   / \
  //       /   \ /   \
  //      /    / \    \
  //     A    E   C    G
  //      \    \ /    /
  //       \   / \   /
  //        \ /   \ /
  //         B     F
  Point A{-4, 0}, B{-1, -3}, C{2, 0}, D{-1, 3};
  Point E{-2, 0}, F{1, -3}, G{4, 0}, H{1, 3};

  std::vector<std::vector<Point>> loops = {{A, B, C, D}, {E, F, G, H}};

  ShapeOutline outline(loops);

  Point X1{0, -2};  // Intersection of EF and BC
  Point X2{0, 2};   // Intersection of EH and CD

  EXPECT_EQ(outline,
            ShapeOutline({{{A, B, X1, F, G}, 1}, {{A, D, X2, H, G}, -1}}));
}

TEST(ComputeOutlineTest, SubtractTwoDiamonds) {
  //         D     F
  //        / \   / \
  //       /   \ /   \
  //      /    / \    \
  //     A    E   C    G
  //      \    \ /    /
  //       \   / \   /
  //        \ /   \ /
  //         B     H
  Point A{-4, 0}, B{-1, -3}, C{2, 0}, D{-1, 3};
  // Traverse the second diamond in the opposite direction.
  Point E{-2, 0}, H{1, -3}, G{4, 0}, F{1, 3};

  std::vector<std::vector<Point>> loops = {{A, B, C, D}, {E, F, G, H}};

  ShapeOutline outline(loops);

  Point X1{0, -2};
  Point X2{0, 2};

  EXPECT_EQ(
      outline,
      ShapeOutline(
          {{{A, B, X1}, 1}, {{E, X1}, -1}, {{A, D, X2}, -1}, {{E, X2}, 1}}));
}

TEST(ComputeOutlineTest, PolygonWithHole) {
  //         D-------------------C
  //         |                   |
  //         |      G-----H      |
  //         |      |     |      |
  //         |      |     |      |
  //  J------|------|-----I      |
  //  |      |      |            |
  //  |      E------F            |
  //  |                          |
  //  A--------------------------B
  Point A{0, 0}, B{14, 0}, C{14, 12}, D{4, 12}, E{4, 3};
  Point F{7, 3}, G{7, 9}, H{10, 9}, I{10, 6}, J{0, 6};

  Point X1{4, 6}, X2{7, 6};

  std::vector<std::vector<Point>> loops = {{J, A, B, C, D, E, F, G, H, I}};

  ShapeOutline outline(loops);

  EXPECT_EQ(outline, ShapeOutline({{{A, J, X1, D, C}, -1},
                                   {{A, B, C}, 1},
                                   {{X2, I, H}, -1},
                                   {{X2, G, H}, 1}}));
}

TEST(ComputeOutlineTest, DisjointPolygons) {
  //         C           F
  //        / \         / \
  //       /   \       /   \
  //      A-----B     D-----E
  Point A{0, 0}, B{2, 0}, C{1, 2};
  Point D{4, 0}, E{6, 0}, F{5, 2};

  std::vector<std::vector<Point>> loops = {{A, B, C}, {D, E, F}};

  ShapeOutline outline(loops);

  EXPECT_EQ(outline,
            ShapeOutline(
                {{{A, C, B}, -1}, {{A, B}, 1}, {{D, F, E}, -1}, {{D, E}, 1}}));
}

TEST(ComputeOutlineTest, Triangle) {
  //         C
  //        / \
  //       /   \
  //      A-----B
  Point A{0, 0}, B{2, 0}, C{1, 1};

  ShapeOutline expected({{{A, C, B}, -1}, {{A, B}, 1}});
  EXPECT_EQ(ShapeOutline(Triangle{A, B, C}), expected);
  EXPECT_EQ(ShapeOutline(Triangle{B, C, A}), expected);
  EXPECT_EQ(ShapeOutline(Triangle{C, A, B}), expected);
}

TEST(ComputeOutlineTest, TriangleVerticalBase) {
  //    B
  //    |\
  //    | \
  //    |  C
  //    | /
  //    |/
  //    A
  Point A{0, 0}, B{0, 2}, C{1, 1};

  ShapeOutline expected({{{A, B, C}, -1}, {{A, C}, 1}});
  EXPECT_EQ(ShapeOutline(Triangle{A, C, B}), expected);
  EXPECT_EQ(ShapeOutline(Triangle{C, B, A}), expected);
  EXPECT_EQ(ShapeOutline(Triangle{B, A, C}), expected);
}

TEST(AdjacentChains, DisjointPolygons) {
  //         C           F
  //        / \         / \
  //       /   \       /   \
  //      A-----B     D-----E
  Point A{0, 0}, B{2, 0}, C{1, 2};
  Point D{4, 0}, E{6, 0}, F{5, 2};

  ShapeOutline shape(
      {{{A, B}, 1}, {{A, C, B}, -1}, {{D, E}, 1}, {{D, F, E}, -1}});

  constexpr uint32_t kAB = 0, kACB = 1, kDE = 2, kDFE = 3;

  EXPECT_EQ(shape.NextIndex(kAB), kACB);
  EXPECT_EQ(shape.PrevIndex(kAB), kACB);

  EXPECT_EQ(shape.NextIndex(kACB), kAB);
  EXPECT_EQ(shape.PrevIndex(kACB), kAB);

  EXPECT_EQ(shape.NextIndex(kDE), kDFE);
  EXPECT_EQ(shape.PrevIndex(kDE), kDFE);

  EXPECT_EQ(shape.NextIndex(kDFE), kDE);
  EXPECT_EQ(shape.PrevIndex(kDFE), kDE);
}

TEST(AdjacentChains, PinchPoints) {
  // G-------------------F
  // |                   |
  // |      C-----D      |
  // |       \   /       |
  // |        \ /        |
  // A---------B---------E
  Point A{-5, -2}, B{0, -2}, C{-2, 0}, D{2, 0}, E{5, -2}, F{5, 2}, G{-5, 2};
  ShapeOutline shape({{{A, B}, 1},
                      {{A, G, F}, -1},
                      {{C, B}, -1},
                      {{B, D}, -1},
                      {{B, E, F}, 1},
                      {{C, D}, 1}});

  constexpr uint32_t kAB = 0, kAGF = 1, kCB = 2, kBD = 3, kBEF = 4, kCD = 5;

  EXPECT_EQ(shape.NextIndex(kAB), kCB);
  EXPECT_EQ(shape.PrevIndex(kCB), kAB);

  EXPECT_EQ(shape.NextIndex(kCB), kCD);
  EXPECT_EQ(shape.PrevIndex(kCD), kCB);

  EXPECT_EQ(shape.NextIndex(kCD), kBD);
  EXPECT_EQ(shape.PrevIndex(kBD), kCD);

  EXPECT_EQ(shape.NextIndex(kBD), kBEF);
  EXPECT_EQ(shape.PrevIndex(kBEF), kBD);

  EXPECT_EQ(shape.NextIndex(kBEF), kAGF);
  EXPECT_EQ(shape.PrevIndex(kAGF), kBEF);

  EXPECT_EQ(shape.NextIndex(kAGF), kAB);
  EXPECT_EQ(shape.PrevIndex(kAB), kAGF);
}

TEST(IntersectionTests, PointAndTriangle) {
  //         C
  //        / \
  //       /   \
  //      A-----B
  Point A{0, 0}, B{2, 0}, C{1, 2};
  ShapeOutline outline({{{A, B}, 1}, {{A, C, B}, -1}});

  // Inside
  EXPECT_TRUE(Intersects(outline, Point{1, 1}));

  // Outside
  EXPECT_FALSE(Intersects(outline, Point{1, 3}));
  EXPECT_FALSE(Intersects(outline, Point{-1, 0}));
  EXPECT_FALSE(Intersects(outline, Point{3, 0}));
  EXPECT_FALSE(Intersects(outline, Point{1, -1}));

  // Boundary
  EXPECT_TRUE(Intersects(outline, Point{0, 0}));
  EXPECT_TRUE(Intersects(outline, Point{1, 0}));
  EXPECT_TRUE(Intersects(outline, Point{0.5f, 1.0f}));
}

TEST(IntersectionTests, PointAndPolygonWithHole) {
  //         D-------------------C
  //         |                   |
  //         |      H-----I      |
  //         |      |     |      |
  //         |      |     |      |
  //  F------E      G-----J      |
  //  |                          |
  //  |                          |
  //  |                          |
  //  A--------------------------B

  Point A{0, 0}, B{14, 0}, C{14, 12}, D{4, 12}, E{4, 6}, F{0, 6};
  Point G{7, 6}, H{7, 9}, I{10, 9}, J{10, 6};

  ShapeOutline outline(
      {{{A, B, C}, 1}, {{G, H, I}, 1}, {{A, F, E, D, C}, -1}, {{G, J, I}, -1}});

  // Inside
  EXPECT_TRUE(Intersects(outline, Point{2, 2}));
  EXPECT_TRUE(Intersects(outline, Point{5, 6}));
  EXPECT_TRUE(Intersects(outline, Point{11, 8}));

  // Inside the hole
  EXPECT_FALSE(Intersects(outline, Point{8, 7}));
  EXPECT_FALSE(Intersects(outline, Point{9, 8}));

  // Boundary
  EXPECT_TRUE(Intersects(outline, Point{7, 7}));
  EXPECT_TRUE(Intersects(outline, Point{8, 9}));

  // Outside
  EXPECT_FALSE(Intersects(outline, Point{15, 6}));
  EXPECT_FALSE(Intersects(outline, Point{2, 13}));
}

TEST(IntersectionTests, RectAndTriangle) {
  //         C
  //        / \
  //       /   \
  //      /     \
  //     A-------B
  Point A{0, 0}, B{6, 0}, C{3, 4};
  std::vector<std::vector<Point>> loops = {{A, B, C}};
  ShapeOutline outline(loops);

  // Completely inside the triangle.
  EXPECT_TRUE(Intersects(outline, FromTwoPoints({2, 1}, {4, 2})));

  // Completely outside the triangle.
  EXPECT_FALSE(Intersects(outline, FromTwoPoints({2, 5}, {4, 7})));    // Above
  EXPECT_FALSE(Intersects(outline, FromTwoPoints({1, -3}, {5, -1})));  // Below
  EXPECT_FALSE(Intersects(outline, FromTwoPoints({-2, 1}, {0.5f, 3})));  // Left
  EXPECT_FALSE(Intersects(outline, FromTwoPoints({5.5f, 1}, {8, 3})));  // Right

  // Intersecting diagonal AC.
  EXPECT_TRUE(Intersects(outline, FromTwoPoints({1, 1}, {2, 3})));

  // Intersecting diagonal CB.
  EXPECT_TRUE(Intersects(outline, FromTwoPoints({4, 1}, {5, 3})));

  // Spanning entirely across the triangle from left to right.
  EXPECT_TRUE(Intersects(outline, FromTwoPoints({-1, 1}, {7, 2})));

  // Spanning across diagonal AC without containing any triangle vertices.
  EXPECT_TRUE(Intersects(outline, FromTwoPoints({0.5f, 1.5f}, {2.5f, 2.5f})));

  // Touching vertex C.
  EXPECT_TRUE(Intersects(outline, FromTwoPoints({2, 4}, {4, 6})));

  // Rect completely contains the triangle.
  EXPECT_TRUE(Intersects(outline, FromTwoPoints({-1, -1}, {7, 5})));
}

TEST(IntersectionTests, RectAndPolygonWithHole) {
  //         D-------------------C
  //         |                   |
  //         |      H-----I      |
  //         |      |     |      |
  //         |      |     |      |
  //  F------E      G-----J      |
  //  |                          |
  //  |                          |
  //  |                          |
  //  A--------------------------B

  Point A{0, 0}, B{14, 0}, C{14, 12}, D{4, 12}, E{4, 6}, F{0, 6};
  Point G{7, 6}, H{7, 9}, I{10, 9}, J{10, 6};

  ShapeOutline outline(
      {{{A, B, C}, 1}, {{G, H, I}, 1}, {{A, F, E, D, C}, -1}, {{G, J, I}, -1}});

  // Completely inside the outer polygon.
  EXPECT_TRUE(Intersects(outline, FromTwoPoints({1, 1}, {3, 5})));
  // Crossing the hole boundary.
  EXPECT_TRUE(Intersects(outline, FromTwoPoints({6, 7}, {8, 8})));
  // Crossing the outer polygon boundary.
  EXPECT_TRUE(Intersects(outline, FromTwoPoints({-2, 1}, {2, 5})));

  // Strictly inside the hole.
  EXPECT_FALSE(Intersects(outline, FromTwoPoints({8, 7}, {9, 8})));
  // Completely outside to the right of the polygon.
  EXPECT_FALSE(Intersects(outline, FromTwoPoints({15, 1}, {18, 5})));
  // Completely outside above the polygon.
  EXPECT_FALSE(Intersects(outline, FromTwoPoints({1, 13}, {3, 15})));
  // Touching from below, overlapping polygon's bottom boundary.
  EXPECT_TRUE(Intersects(outline, FromTwoPoints({1, -2}, {3, 0})));
  // Touching from the right, overlapping polygon's right boundary.
  EXPECT_TRUE(Intersects(outline, FromTwoPoints({14, 2}, {16, 5})));
  // Rect's top boundary contained in polygon's bottom boundary.
  EXPECT_TRUE(Intersects(outline, FromTwoPoints({-2, -5}, {16, 0})));
  // Rect's right boundary contains polygon's left boundary.
  EXPECT_TRUE(Intersects(outline, FromTwoPoints({-5, -2}, {0, 8})));
  // Rect's left boundary contains polygon's right boundary.
  EXPECT_TRUE(Intersects(outline, FromTwoPoints({14, -2}, {18, 15})));
  // Touching at a single vertex (0, 0).
  EXPECT_TRUE(Intersects(outline, FromTwoPoints({-2, -2}, {0, 0})));

  // Rect completely covers the hole.
  EXPECT_TRUE(Intersects(outline, FromTwoPoints({6, 5}, {11, 10})));
  // Rect completely contains the outer polygon.
  EXPECT_TRUE(Intersects(outline, FromTwoPoints({-1, -1}, {15, 13})));
}

TEST(ComputeBoundaryLoopsTest, BasicTriangle) {
  //         C
  //        / \
  //       /   \
  //      A-----B
  Point A{0, 0}, B{2, 0}, C{1, 1};

  ShapeOutline outline({{{A, C, B}, -1}, {{A, B}, 1}});

  std::vector<std::vector<Point>> loops = ComputeBoundaryLoops(outline);

  EXPECT_THAT(loops, UnorderedElementsAre(
                         IsCyclicPermutationOf(std::vector<Point>{A, B, C})));
}

TEST(ComputeBoundaryLoopsTest, SumTwoDiamonds) {
  //         H     F
  //        / \   / \
  //       /   \ /   \
  //      /     G     \
  //     A             E
  //      \     C     /
  //       \   / \   /
  //        \ /   \ /
  //         B     D
  Point A{-4, 0}, B{-1, -3}, C{0, -2}, D{1, -3}, E{4, 0}, F{1, 3}, G{0, 2},
      H{-1, 3};

  ShapeOutline outline({{{A, B, C, D, E}, 1}, {{A, H, G, F, E}, -1}});

  std::vector<std::vector<Point>> loops = ComputeBoundaryLoops(outline);

  EXPECT_THAT(loops, UnorderedElementsAre(IsCyclicPermutationOf(
                         std::vector<Point>{A, B, C, D, E, F, G, H})));
}

TEST(ComputeBoundaryLoopsTest, PolygonWithHole) {
  //         D-------------------C
  //         |                   |
  //         |      H-----I      |
  //         |      |     |      |
  //         |      |     |      |
  //  F------E      G-----J      |
  //  |                          |
  //  |                          |
  //  |                          |
  //  A--------------------------B
  Point A{0, 0}, B{14, 0}, C{14, 12}, D{4, 12}, E{4, 6}, F{0, 6};
  Point G{7, 6}, H{7, 9}, I{10, 9}, J{10, 6};

  ShapeOutline outline(
      {{{A, B, C}, 1}, {{G, H, I}, 1}, {{A, F, E, D, C}, -1}, {{G, J, I}, -1}});

  std::vector<std::vector<Point>> loops = ComputeBoundaryLoops(outline);

  EXPECT_THAT(loops,
              UnorderedElementsAre(
                  IsCyclicPermutationOf(std::vector<Point>{A, B, C, D, E, F}),
                  IsCyclicPermutationOf(std::vector<Point>{G, H, I, J})));
}

TEST(ComputeBoundaryLoopsTest, PolygonWithTwoHoles) {
  //                  C
  //                /   \
  //               /     \
  //              /       \
  //             /         \
  //            /  G     K  \
  //           /  / \   / \  \
  //          D  F   H J   L  B
  //           \  \ /   \ /  /
  //            \  E     I  /
  //             \         /
  //              \       /
  //               \     /
  //                \   /
  //                  A
  Point A{10, 0}, B{20, 10}, C{10, 20}, D{0, 10};
  Point E{7, 8}, F{4, 10}, G{7, 12}, H{9, 10};
  Point I{13, 8}, J{11, 10}, K{13, 12}, L{16, 10};

  ShapeOutline outline({{{D, A, B}, 1},
                        {{F, G, H}, 1},
                        {{J, K, L}, 1},
                        {{D, C, B}, -1},
                        {{F, E, H}, -1},
                        {{J, I, L}, -1}});

  std::vector<std::vector<Point>> loops = ComputeBoundaryLoops(outline);

  EXPECT_THAT(loops,
              UnorderedElementsAre(
                  IsCyclicPermutationOf(std::vector<Point>{A, B, C, D}),
                  IsCyclicPermutationOf(std::vector<Point>{E, F, G, H}),
                  IsCyclicPermutationOf(std::vector<Point>{I, J, K, L})));
}

TEST(ComputeBoundaryLoopsTest, PinchPoints) {
  // H-------------------G
  // |                   |
  // |      C-----D      |
  // |       \   /       |
  // |        \ /        |
  // A---------B---------F
  Point A{-5, -2}, B{0, -2}, C{-2, 0}, D{2, 0}, F{5, -2}, G{5, 2}, H{-5, 2};
  ShapeOutline shape({{{A, B}, 1},
                      {{A, H, G}, -1},
                      {{C, B}, -1},
                      {{B, D}, -1},
                      {{B, F, G}, 1},
                      {{C, D}, 1}});

  std::vector<std::vector<Point>> loops = ComputeBoundaryLoops(shape);
  EXPECT_THAT(loops, UnorderedElementsAre(IsCyclicPermutationOf(
                         std::vector<Point>{A, B, C, D, B, F, G, H})));
}

TEST(ComputeTriangulationTest, BasicQuad) {
  //    D-----------C
  //    |           |
  //    |           |
  //    |           |
  //    A-----------B
  Point A{0, 0}, B{2, 0}, C{2, 2}, D{0, 2};
  ShapeOutline outline({{{A, B, C}, 1}, {{A, D, C}, -1}});

  auto [points, triangles] = ComputeTriangulation(outline);
  EXPECT_THAT(points, UnorderedElementsAre(A, B, C, D));
  EXPECT_THAT(triangles, SizeIs(2));
  EXPECT_TRUE(HasValidIndices(points, triangles));
  EXPECT_TRUE(ContainsPoint(points, triangles, Point{1, 1}));
  EXPECT_TRUE(ContainsPoint(points, triangles, Point{0.5f, 1.5f}));
  EXPECT_FALSE(ContainsPoint(points, triangles, Point{3, 3}));
  EXPECT_FALSE(ContainsPoint(points, triangles, Point{-1, 1}));
}

TEST(ComputeTriangulationTest, TriangleWithExtraCollinearVertex) {
  //         C
  //        / \
  //       /   \
  //      A--M--B
  Point A{0, 0}, M{1, 0}, B{2, 0}, C{1, 2};
  ShapeOutline outline({{{A, M, B}, 1}, {{A, C, B}, -1}});

  auto [points, triangles] = ComputeTriangulation(outline);
  EXPECT_THAT(points, UnorderedElementsAre(A, M, B, C));
  EXPECT_THAT(triangles, SizeIs(2));
  EXPECT_TRUE(HasValidIndices(points, triangles));
  EXPECT_TRUE(ContainsPoint(points, triangles, Point{0.5f, 0.5f}));
  EXPECT_TRUE(ContainsPoint(points, triangles, Point{1.5f, 0.5f}));
  EXPECT_TRUE(ContainsPoint(points, triangles, Point{1.0f, 1.0f}));
  EXPECT_FALSE(ContainsPoint(points, triangles, Point{0.5f, -0.5f}));
  EXPECT_FALSE(ContainsPoint(points, triangles, Point{0.5f, 1.5f}));
}

TEST(ComputeTriangulationTest, SumTwoDiamonds) {
  //         H     F
  //        / \   / \
  //       /   \ /   \
  //      /     G     \
  //     A             E
  //      \     C     /
  //       \   / \   /
  //        \ /   \ /
  //         B     D
  Point A{-4, 0}, B{-1, -3}, C{0, -2}, D{1, -3}, E{4, 0}, F{1, 3}, G{0, 2},
      H{-1, 3};
  ShapeOutline outline({{{A, B, C, D, E}, 1}, {{E, F, G, H, A}, -1}});

  auto [points, triangles] = ComputeTriangulation(outline);
  EXPECT_THAT(points, UnorderedElementsAre(A, B, C, D, E, F, G, H));
  EXPECT_THAT(triangles, SizeIs(6));
  EXPECT_TRUE(HasValidIndices(points, triangles));
  EXPECT_TRUE(ContainsPoint(points, triangles, Point{-2, 0}));
  EXPECT_TRUE(ContainsPoint(points, triangles, Point{2, 0}));
  EXPECT_TRUE(ContainsPoint(points, triangles, Point{0, 0}));
  EXPECT_FALSE(ContainsPoint(points, triangles, Point{5, 5}));

  // Inside concavities
  EXPECT_FALSE(ContainsPoint(points, triangles, Point{0, -2.5f}));
  EXPECT_FALSE(ContainsPoint(points, triangles, Point{0, 2.5f}));
}

TEST(ComputeTriangulationTest, DisjointTriangles) {
  //         C           F
  //        / \         / \
  //       /   \       /   \
  //      A-----B     D-----E
  Point A{0, 0}, B{2, 0}, C{1, 2};
  Point D{4, 0}, E{6, 0}, F{5, 2};
  ShapeOutline outline(
      {{{A, B}, 1}, {{D, E}, 1}, {{A, C, B}, -1}, {{D, F, E}, -1}});

  auto [points, triangles] = ComputeTriangulation(outline);
  EXPECT_THAT(points, UnorderedElementsAre(A, B, C, D, E, F));
  EXPECT_THAT(triangles, SizeIs(2));
  EXPECT_TRUE(HasValidIndices(points, triangles));
  EXPECT_TRUE(ContainsPoint(points, triangles, Point{1, 1}));
  EXPECT_TRUE(ContainsPoint(points, triangles, Point{5, 1}));
  EXPECT_FALSE(ContainsPoint(points, triangles, Point{3, 1}));
  EXPECT_FALSE(ContainsPoint(points, triangles, Point{1, 3}));
  EXPECT_FALSE(ContainsPoint(points, triangles, Point{5, 3}));
}

TEST(ComputeTriangulationTest, PolygonWithHole) {
  // TODO(b/521449017): `ComputeTriangulation` does not yet handle triangulating
  // polygons with holes, and instead triangulates the outer polygon only.
  //         D-------------------C
  //         |                   |
  //         |      H-----I      |
  //         |      |     |      |
  //         |      |     |      |
  //  F------E      G-----J      |
  //  |                          |
  //  |                          |
  //  |                          |
  //  A--------------------------B
  Point A{0, 0}, B{14, 0}, C{14, 12}, D{4, 12}, E{4, 6}, F{0, 6};
  Point G{7, 6}, H{7, 9}, I{10, 9}, J{10, 6};
  ShapeOutline outline(
      {{{A, B, C}, 1}, {{G, H, I}, 1}, {{A, F, E, D, C}, -1}, {{G, J, I}, -1}});

  auto [points, triangles] = ComputeTriangulation(outline);
  EXPECT_THAT(points, UnorderedElementsAre(A, B, C, D, E, F));
  EXPECT_THAT(triangles, SizeIs(4));
  EXPECT_TRUE(HasValidIndices(points, triangles));
  EXPECT_TRUE(ContainsPoint(points, triangles, Point{1, 1}));
  EXPECT_TRUE(ContainsPoint(points, triangles, Point{5, 1}));
  EXPECT_TRUE(ContainsPoint(points, triangles, Point{8, 1}));
  // Point in the hole GHIJ
  EXPECT_TRUE(ContainsPoint(points, triangles, Point{8, 8}));
}

TEST(ComputeSubtractionTests, SubtractDisjointTriangles) {
  //         C           F
  //        / \         / \
  //       /   \       /   \
  //      A-----B     D-----E
  Point A{0, 0}, B{2, 0}, C{1, 2};
  Point D{4, 0}, E{6, 0}, F{5, 2};

  ShapeOutline shape_a({{{A, B}, 1}, {{A, C, B}, -1}});
  ShapeOutline shape_b({{{D, E}, 1}, {{D, F, E}, -1}});

  ShapeOutline result = ComputeSubtraction(shape_a, shape_b);

  EXPECT_EQ(result, shape_a);
}

TEST(ComputeSubtractionTests, SubtractTwoDiamonds) {
  //         D     F
  //        / \   / \
  //       /   \ /   \
  //      /    / \    \
  //     A    E   C    G
  //      \    \ /    /
  //       \   / \   /
  //        \ /   \ /
  //         B     H
  Point A{-4, 0}, B{-1, -3}, C{2, 0}, D{-1, 3};
  Point E{-2, 0}, F{1, 3}, G{4, 0}, H{1, -3};

  Point X1{0, -2};  // Intersection of BC and EH
  Point X2{0, 2};   // Intersection of CD and EF

  ShapeOutline shape_a({{{A, B, C}, 1}, {{A, D, C}, -1}});
  ShapeOutline shape_b({{{E, H, G}, 1}, {{E, F, G}, -1}});

  ShapeOutline result = ComputeSubtraction(shape_a, shape_b);

  EXPECT_THAT(
      result,
      ShapeOutlineNear(
          ShapeOutline(
              {{{A, B, X1}, 1}, {{A, D, X2}, -1}, {{E, X2}, 1}, {{E, X1}, -1}}),
          1e-6f));
}

TEST(ComputeSubtractionTests, SubtractRectFromRect) {
  //    D-----------C
  //    |           |
  //    |       H--------G
  //    |       |   |    |
  //    |       E--------F
  //    |           |
  //    A-----------B
  Point A{0, 0}, B{6, 0}, C{6, 6}, D{0, 6};
  Point E{2, 2}, F{8, 2}, G{8, 4}, H{2, 4};

  Point X1{6, 2};  // Intersection of BC and EF
  Point X2{6, 4};  // Intersection of BC and HG

  ShapeOutline shape_a({{{A, B, C}, 1}, {{A, D, C}, -1}});
  ShapeOutline shape_b({{{E, F, G}, 1}, {{E, H, G}, -1}});

  ShapeOutline result = ComputeSubtraction(shape_a, shape_b);

  EXPECT_THAT(result, ShapeOutlineNear(ShapeOutline({{{A, B, X1}, 1},
                                                     {{E, H, X2, C}, 1},
                                                     {{A, D, C}, -1},
                                                     {{E, X1}, -1}}),
                                       kFloatTolerance));
}

TEST(ComputeSubtractionTests, FullyDeletedShape) {
  //    H-----------G
  //    |   D---C   |
  //    |   |   |   |
  //    |   A---B   |
  //    E-----------F
  Point A{2, 2}, B{4, 2}, C{4, 4}, D{2, 4};
  Point E{0, 0}, F{6, 0}, G{6, 6}, H{0, 6};

  ShapeOutline shape_a({{{A, B, C}, 1}, {{A, D, C}, -1}});
  ShapeOutline shape_b({{{E, F, G}, 1}, {{E, H, G}, -1}});

  ShapeOutline result = ComputeSubtraction(shape_a, shape_b);

  EXPECT_EQ(result, ShapeOutline());
}

TEST(ComputeSubtractionTests, SubtractFullyContainedHole) {
  //    D-----------C
  //    |   H---G   |
  //    |   |   |   |
  //    |   E---F   |
  //    A-----------B
  Point A{0, 0}, B{6, 0}, C{6, 6}, D{0, 6};
  Point E{2, 2}, F{4, 2}, G{4, 4}, H{2, 4};

  ShapeOutline shape_a({{{A, B, C}, 1}, {{A, D, C}, -1}});
  ShapeOutline shape_b({{{E, F, G}, 1}, {{E, H, G}, -1}});

  ShapeOutline result = ComputeSubtraction(shape_a, shape_b);

  EXPECT_EQ(
      result,
      ShapeOutline(
          {{{A, B, C}, 1}, {{A, D, C}, -1}, {{E, F, G}, -1}, {{E, H, G}, 1}}));
}

TEST(ComputeSubtractionTests, SubtractShapeWithHole) {
  //    D---------------C
  //    |               |
  //    |       H-------+-------G
  //    |       |   L---+---K   |
  //    |       |   |   |   |   |
  //    |       |   I---+---J   |
  //    |       E-------+-------F
  //    |               |
  //    A---------------B
  Point A{0, 0}, B{8, 0}, C{8, 8}, D{0, 8};
  Point E{6, 2}, F{10, 2}, G{10, 6}, H{6, 6};
  Point I{7, 3}, J{9, 3}, K{9, 5}, L{7, 5};

  Point X1{8, 2};  // Intersection of BC and EF
  Point X2{8, 6};  // Intersection of BC and HG
  Point X3{8, 3};  // Intersection of BC and IJ
  Point X4{8, 5};  // Intersection of BC and KL

  ShapeOutline shape_a({{{A, B, C}, 1}, {{A, D, C}, -1}});
  ShapeOutline shape_b(
      {{{E, F, G}, 1}, {{E, H, G}, -1}, {{I, L, K}, 1}, {{I, J, K}, -1}});

  ShapeOutline result = ComputeSubtraction(shape_a, shape_b);

  EXPECT_THAT(result, ShapeOutlineNear(ShapeOutline({{{A, D, C}, -1},
                                                     {{A, B, X1}, 1},
                                                     {{E, X1}, -1},
                                                     {{E, H, X2, C}, 1},
                                                     {{I, L, X4}, -1},
                                                     {{I, X3, X4}, 1}}),
                                       kFloatTolerance));
}

TEST(ComputeSubtractionTests, VertexEdgeTransverse1) {
  //                            G
  //                         / /
  //                     /    /
  //                 /       /
  //    D---------E----C    /
  //    |          \   |   /
  //    |           \  |  /
  //    |            \ | /
  //    |              F
  //    |              |
  //    A--------------B
  Point A{0, 0}, B{4, 0}, C{4, 4}, D{0, 4};
  Point E{2, 4}, F{4, 1}, G{6, 6};

  ShapeOutline shape_abcd({{{A, B, C}, 1}, {{A, D, C}, -1}});
  ShapeOutline shape_efg({{{E, F, G}, 1}, {{E, G}, -1}});

  ShapeOutline result = ComputeSubtraction(shape_abcd, shape_efg);
  EXPECT_THAT(result, ShapeOutlineNear(
                          ShapeOutline({{{A, B, F}, 1}, {{A, D, E, F}, -1}}),
                          kFloatTolerance));
}

TEST(ComputeSubtractionTests, VertexEdgeTransverse2) {
  // EFG - ABCD
  //                            G
  //                         / /
  //                     /    /
  //                 /       /
  //    D---------E----C    /
  //    |          \   |   /
  //    |           \  |  /
  //    |            \ | /
  //    |              F
  //    |              |
  //    A--------------B
  Point A{0, 0}, B{4, 0}, C{4, 4}, D{0, 4};
  Point E{2, 4}, F{4, 1}, G{6, 6};

  ShapeOutline shape_efg({{{E, F, G}, 1}, {{E, G}, -1}});
  ShapeOutline shape_abcd({{{A, B, C}, 1}, {{A, D, C}, -1}});

  ShapeOutline result = ComputeSubtraction(shape_efg, shape_abcd);
  EXPECT_THAT(
      result,
      ShapeOutlineNear(
          ShapeOutline({{{F, C}, -1}, {{F, G}, 1}, {{E, C}, 1}, {{E, G}, -1}}),
          kFloatTolerance));
}

TEST(ComputeSubtractionTests, VertexEdgeTransverse3) {
  //       F---E
  //        \ /
  //    C----D----B
  //     \       /
  //      \     /
  //       \   /
  //        \ /
  //         A
  Point A{2, 0}, B{4, 4}, C{0, 4}, D{2, 4}, E{1, 5}, F{2, 4};

  ShapeOutline shape_abc({{{C, A, B}, 1}, {{C, B}, -1}});
  ShapeOutline shape_def({{{F, D, E}, 1}, {{F, E}, -1}});

  ShapeOutline result = ComputeSubtraction(shape_abc, shape_def);

  // It's important for us that edge CB is subdivided at D. If the triangle ABC
  // was part of a mesh and adjacent to another triangle along edge CB, then
  // preserving the vertex D would be necessary to properly glue the modified
  // triangles back together without leaving a topological gap.
  EXPECT_THAT(result,
              ShapeOutlineNear(ShapeOutline({{{C, A, B}, 1}, {{C, D, B}, -1}}),
                               kFloatTolerance));
}

TEST(ComputeSubtractionTests, VertexEdgeNonTransverse1) {
  //    D---------C
  //    |         |
  //    E---------G
  //    | \     / |
  //    |  \   /  |
  //    |   \ /   |
  //    A----F----B
  Point A{0, 0}, B{4, 0}, C{4, 4}, D{0, 4};
  Point E{0, 2}, F{2, 0}, G{4, 2};

  ShapeOutline shape_abcd({{{A, B, C}, 1}, {{A, D, C}, -1}});
  ShapeOutline shape_efg({{{E, F, G}, 1}, {{E, G}, -1}});

  ShapeOutline result = ComputeSubtraction(shape_abcd, shape_efg);

  EXPECT_THAT(result, ShapeOutlineNear(ShapeOutline({{{A, F}, 1},
                                                     {{A, E, F}, -1},
                                                     {{F, G}, -1},
                                                     {{F, B, G}, 1},
                                                     {{E, D, C}, -1},
                                                     {{E, G, C}, 1}}),
                                       kFloatTolerance));
}

TEST(ComputeSubtractionTests, VertexEdgeNonTransverse2) {
  //    H---C---G
  //    |  / \  |
  //    | /   \ |
  //    D       B
  //    | \   / |
  //    |  \ /  |
  //    E---A---F
  Point E{0, 0}, F{4, 0}, G{4, 4}, H{0, 4};
  Point A{2, 0}, B{4, 2}, C{2, 4}, D{0, 2};

  ShapeOutline shape_abcd({{{D, A, B}, 1}, {{D, C, B}, -1}});
  ShapeOutline shape_efgh({{{E, F, G}, 1}, {{E, H, G}, -1}});

  ShapeOutline result = ComputeSubtraction(shape_abcd, shape_efgh);
  EXPECT_THAT(result.Chains(), testing::IsEmpty());
}

TEST(ComputeSubtractionTests, VertexVertexTransverse) {
  //          D
  //         / \
  //        /   \
  //       /     \
  //      A   E---C
  //       \  |  /|
  //        \ | / |
  //         \|/  |
  //          B---F

  Point A{-4, 0}, B{0, -4}, C{4, 0}, D{0, 4};
  Point E{0, 0}, F{4, -4};

  ShapeOutline shape_abcd({{{A, D, C}, -1}, {{A, B, C}, 1}});
  ShapeOutline shape_bfce({{{B, E, C}, -1}, {{B, F, C}, 1}});

  ShapeOutline result = ComputeSubtraction(shape_abcd, shape_bfce);

  EXPECT_THAT(result, ShapeOutlineNear(
                          ShapeOutline({{{A, B, E, C}, 1}, {{A, D, C}, -1}}),
                          kFloatTolerance));
}

TEST(ComputeSubtractionTests, VertexVertexNontransverse2) {
  //    A-------------F
  //    |      G      |
  //    |      |\     |
  //    |      | \    |
  //    |      |  D---E
  //    |      | /|
  //    |      |/ |
  //    |      H  |
  //    B---------C
  Point A{0, 4}, B{0, 0}, C{4, 0}, D{4, 2}, E{5, 2}, F{5, 4};
  Point G{2, 3}, H{2, 1};

  ShapeOutline shape_abcdef({{{B, A, F}, -1}, {{B, C, D, E, F}, 1}});
  ShapeOutline shape_ghd({{{H, G, D}, -1}, {{H, D}, 1}});

  ShapeOutline result = ComputeSubtraction(shape_abcdef, shape_ghd);

  EXPECT_THAT(result, ShapeOutlineNear(ShapeOutline({{{B, C, D}, 1},
                                                     {{B, A, F}, -1},
                                                     {{H, D}, -1},
                                                     {{H, G, D, E, F}, 1}}),
                                       kFloatTolerance));
}

TEST(ComputeSubtractionTests, VertexVertexNontransverse3) {
  //           C     E
  //          / \   / \
  //         /   \ /   \
  //        A-----B-----D
  Point A{0, 0}, B{2, 0}, C{1, 2}, D{4, 0}, E{3, 2};

  ShapeOutline shape_abc({{{A, C, B}, -1}, {{A, B}, 1}});
  ShapeOutline shape_bde({{{B, E, D}, -1}, {{B, D}, 1}});

  ShapeOutline result = ComputeSubtraction(shape_abc, shape_bde);

  EXPECT_THAT(result, ShapeOutlineNear(shape_abc, kFloatTolerance));
}
}  // namespace
}  // namespace ink::geometry_internal
