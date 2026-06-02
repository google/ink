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
#include "ink/geometry/point.h"

namespace ink::geometry_internal {
namespace {

using ::testing::UnorderedElementsAre;

TEST(OutlineProcessingTest, BasicTriangle) {
  //         C
  //        / \
  //       /   \
  //      A-----B
  Point A{0, 0}, B{2, 0}, C{1, 1};

  std::vector<std::vector<Point>> loops = {{A, B, C}};

  std::vector<std::vector<Point>> chains = ComputeMonotoneBoundaryChains(loops);

  EXPECT_THAT(chains, UnorderedElementsAre(std::vector<Point>{A, B},
                                           std::vector<Point>{B, C, A}));
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

  std::vector<std::vector<Point>> chains = ComputeMonotoneBoundaryChains(loops);

  EXPECT_THAT(chains, UnorderedElementsAre(std::vector<Point>{A, B},
                                           std::vector<Point>{B, C, X1, F, A}));
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

  std::vector<std::vector<Point>> chains = ComputeMonotoneBoundaryChains(loops);

  EXPECT_THAT(chains, UnorderedElementsAre(std::vector<Point>{A, B, C},
                                           std::vector<Point>{C, D, A}));
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

  std::vector<std::vector<Point>> chains = ComputeMonotoneBoundaryChains(loops);

  Point X1{0, -2};  // Intersection of EF and BC
  Point X2{0, 2};   // Intersection of EH and CD

  EXPECT_THAT(chains, UnorderedElementsAre(std::vector<Point>{A, B, X1, F, G},
                                           std::vector<Point>{G, H, X2, D, A}));
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

  std::vector<std::vector<Point>> chains = ComputeMonotoneBoundaryChains(loops);

  Point X1{0, -2};
  Point X2{0, 2};

  EXPECT_THAT(chains, UnorderedElementsAre(std::vector<Point>{A, B, X1},
                                           std::vector<Point>{X2, D, A},
                                           std::vector<Point>{E, X2},
                                           std::vector<Point>{X1, E}));
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

  std::vector<std::vector<Point>> chains = ComputeMonotoneBoundaryChains(loops);

  EXPECT_THAT(chains, UnorderedElementsAre(std::vector<Point>{A, B, C},
                                           std::vector<Point>{X2, G, H},
                                           std::vector<Point>{C, D, X1, J, A},
                                           std::vector<Point>{H, I, X2}));
}

TEST(OutlineProcessingTest, DisjointPolygons) {
  //         C           F
  //        / \         / \
  //       /   \       /   \
  //      A-----B     D-----E
  Point A{0, 0}, B{2, 0}, C{1, 2};
  Point D{4, 0}, E{6, 0}, F{5, 2};

  std::vector<std::vector<Point>> loops = {{A, B, C}, {D, E, F}};

  std::vector<std::vector<Point>> chains = ComputeMonotoneBoundaryChains(loops);

  EXPECT_THAT(chains, UnorderedElementsAre(std::vector<Point>{A, B},
                                           std::vector<Point>{B, C, A},
                                           std::vector<Point>{D, E},
                                           std::vector<Point>{E, F, D}));
}

}  // namespace
}  // namespace ink::geometry_internal
