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

#include <utility>
#include <vector>

#include "benchmark/benchmark.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "ink/geometry/internal/generic_tessellator.h"
#include "ink/geometry/internal/outline_processing.h"
#include "ink/geometry/internal/outline_triangulation.h"
#include "ink/geometry/internal/point_tessellation_helper.h"
#include "ink/geometry/point.h"
#include "libtess2/tesselator.h"

namespace ink::geometry_internal {
namespace {

const std::vector<
    std::pair<absl::string_view, std::vector<std::vector<Point>>>>&
TestShapes() {
  static const auto* shapes = new std::vector<
      std::pair<absl::string_view, std::vector<std::vector<Point>>>>{
      {"notched_triangle", {{{0, 0}, {3, 0}, {5, 3}, {7, 0}, {10, 0}, {4, 8}}}},
      {"split_triangle",
       {{{0, 0}, {10, 0}, {8.5f, 3}, {1.5f, 3}},
        {{2.5f, 5}, {7.5f, 5}, {4, 8}}}},
      {"notched_rectangle",
       {{{0, 0}, {10, 0}, {7, 3}, {10, 6}, {0, 6}, {3, 3}}}},
      {"crescent",
       {{{-9.801f, -1.987f},
         {-8.300f, -5.577f},
         {-5.543f, -8.323f},
         {-1.946f, -9.809f},
         {1.946f, -9.809f},
         {5.543f, -8.323f},
         {8.300f, -5.577f},
         {9.801f, -1.987f},
         {9.430f, -1.911f},
         {7.986f, -5.366f},
         {5.333f, -8.008f},
         {1.872f, -9.437f},
         {-1.872f, -9.437f},
         {-5.333f, -8.008f},
         {-7.986f, -5.366f},
         {-9.430f, -1.911f}}}},
      {"square_with_hole",
       {{{0, 0}, {6, 0}, {6, 6}, {0, 6}}, {{3, 4}, {4, 3}, {3, 2}, {2, 3}}}},
      {"blob", {{{5.89f, 0.00f},   {5.60f, 0.55f},   {5.13f, 1.02f},
                 {4.69f, 1.42f},   {4.45f, 1.85f},   {4.46f, 2.38f},
                 {4.61f, 3.08f},   {4.71f, 3.86f},   {4.59f, 4.59f},
                 {4.16f, 5.07f},   {3.47f, 5.19f},   {2.64f, 4.95f},
                 {1.85f, 4.46f},   {1.19f, 3.93f},   {0.70f, 3.51f},
                 {0.33f, 3.32f},   {0.00f, 3.33f},   {-0.34f, 3.49f},
                 {-0.73f, 3.68f},  {-1.17f, 3.87f},  {-1.68f, 4.05f},
                 {-2.26f, 4.23f},  {-2.95f, 4.42f},  {-3.74f, 4.56f},
                 {-4.56f, 4.56f},  {-5.28f, 4.33f},  {-5.73f, 3.83f},
                 {-5.82f, 3.11f},  {-5.54f, 2.29f},  {-5.02f, 1.52f},
                 {-4.49f, 0.89f},  {-4.15f, 0.41f},  {-4.11f, 0.00f},
                 {-4.35f, -0.43f}, {-4.68f, -0.93f}, {-4.88f, -1.48f},
                 {-4.78f, -1.98f}, {-4.36f, -2.33f}, {-3.71f, -2.48f},
                 {-3.02f, -2.48f}, {-2.48f, -2.48f}, {-2.18f, -2.66f},
                 {-2.09f, -3.13f}, {-2.07f, -3.87f}, {-1.98f, -4.78f},
                 {-1.71f, -5.64f}, {-1.25f, -6.29f}, {-0.65f, -6.63f},
                 {-0.00f, -6.67f}, {0.64f, -6.47f},  {1.22f, -6.12f},
                 {1.73f, -5.70f},  {2.15f, -5.19f},  {2.45f, -4.59f},
                 {2.60f, -3.89f},  {2.60f, -3.17f},  {2.51f, -2.51f},
                 {2.45f, -2.01f},  {2.58f, -1.72f},  {3.00f, -1.60f},
                 {3.70f, -1.53f},  {4.55f, -1.38f},  {5.32f, -1.06f},
                 {5.81f, -0.57f}}}},
  };
  return *shapes;
}

TessellationResult<Point> Libtess2(const ShapeOutline& shape) {
  std::vector<std::vector<Point>> loops = ComputeBoundaryLoops(shape);
  std::vector<absl::Span<const Point>> contours(loops.begin(), loops.end());
  return Tessellate<PointTessellationHelper>(contours, TESS_WINDING_POSITIVE);
}

template <auto Fn>
void BM_Triangulation(benchmark::State& state) {
  const auto& [name, loops] = TestShapes()[state.range(0)];
  state.SetLabel(name);
  ShapeOutline shape(loops);
  for (auto s : state) {
    benchmark::DoNotOptimize(Fn(shape));
  }
}
BENCHMARK(BM_Triangulation<Triangulate>)
    ->DenseRange(0, TestShapes().size() - 1);
BENCHMARK(BM_Triangulation<Libtess2>)->DenseRange(0, TestShapes().size() - 1);

}  // namespace
}  // namespace ink::geometry_internal
