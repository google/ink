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

#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "ink/geometry/internal/test_matchers.h"
#include "ink/geometry/point.h"
#include "ink/geometry/rect.h"

namespace ink::geometry_internal {
namespace {

using ::testing::UnorderedElementsAre;

constexpr auto FromTwoPoints = Rect::FromTwoPoints;

TEST(OutlineProcessingTest, BasicTriangle) {
  //         C
  //        / \
  //       /   \
  //      A-----B
  Point A{0, 0}, B{2, 0}, C{1, 1};

  std::vector<std::vector<Point>> loops = {{A, B, C}};

  ShapeOutline outline = ComputeShapeOutline(loops);

  EXPECT_EQ(outline, ShapeOutline({{{A, C, B}, -1}, {{A, B}, 1}}));
}

TEST(OutlineProcessingTest, SelfIntersection) {
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

  ShapeOutline outline = ComputeShapeOutline(loops);

  EXPECT_EQ(outline, ShapeOutline({{{A, F, X1, C, B}, -1}, {{A, B}, 1}}));
}

TEST(OutlineProcessingTest, VerticalSegments) {
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

  ShapeOutline outline = ComputeShapeOutline(loops);

  EXPECT_EQ(outline, ShapeOutline({{{A, D, C}, -1}, {{A, B, C}, 1}}));
}

TEST(OutlineProcessingTest, SumTwoDiamonds) {
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

  ShapeOutline outline = ComputeShapeOutline(loops);

  Point X1{0, -2};  // Intersection of EF and BC
  Point X2{0, 2};   // Intersection of EH and CD

  EXPECT_EQ(outline,
            ShapeOutline({{{A, B, X1, F, G}, 1}, {{A, D, X2, H, G}, -1}}));
}

TEST(OutlineProcessingTest, SubtractTwoDiamonds) {
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

  ShapeOutline outline = ComputeShapeOutline(loops);

  Point X1{0, -2};
  Point X2{0, 2};

  EXPECT_EQ(
      outline,
      ShapeOutline(
          {{{A, B, X1}, 1}, {{E, X1}, -1}, {{A, D, X2}, -1}, {{E, X2}, 1}}));
}

TEST(OutlineProcessingTest, PolygonWithHole) {
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

  ShapeOutline outline = ComputeShapeOutline(loops);

  EXPECT_EQ(outline, ShapeOutline({{{A, J, X1, D, C}, -1},
                                   {{A, B, C}, 1},
                                   {{X2, I, H}, -1},
                                   {{X2, G, H}, 1}}));
}

TEST(OutlineProcessingTest, DisjointPolygons) {
  //         C           F
  //        / \         / \
  //       /   \       /   \
  //      A-----B     D-----E
  Point A{0, 0}, B{2, 0}, C{1, 2};
  Point D{4, 0}, E{6, 0}, F{5, 2};

  std::vector<std::vector<Point>> loops = {{A, B, C}, {D, E, F}};

  ShapeOutline outline = ComputeShapeOutline(loops);

  EXPECT_EQ(outline,
            ShapeOutline(
                {{{A, C, B}, -1}, {{A, B}, 1}, {{D, F, E}, -1}, {{D, E}, 1}}));
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
  ShapeOutline outline = ComputeShapeOutline(loops);

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
  //.               /   \
  //               /     \
  //              /       \
  //             /         \
  //            /  G     K  \
  //           /  / \   / \  \
  //          D  F  H  J  L  B
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

}  // namespace
}  // namespace ink::geometry_internal
