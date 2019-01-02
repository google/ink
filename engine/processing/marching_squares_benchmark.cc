// Copyright 2018 Google LLC
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

#include "ink/engine/processing/marching_squares.h"

#include <vector>

#include "testing/base/public/benchmark.h"
#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/algorithms/distance.h"
#include "ink/engine/rendering/baseGL/gpupixels.h"

namespace ink {
namespace marching_squares {
namespace {

using glm::vec2;
using glm::ivec2;
using benchmark::State;

static void BM_TraceSolidImage(State &state) {
  int size = state.range(0);
  GPUPixels pixels(ivec2(size, size), std::vector<uint32_t>(size * size, 0x1));
  MarchingSquares<ColorEqualPredicate> ms(ColorEqualPredicate(1), &pixels);
  while (state.KeepRunning()) {
    ms.TraceAllBoundaries();
  }
}
BENCHMARK(BM_TraceSolidImage)->Arg(128)->Arg(256)->Arg(512)->Arg(1024);

static void BM_TraceBullseye(State &state) {
  int size = state.range(0);
  GPUPixels pixels(ivec2(size, size), std::vector<uint32_t>(size * size, 0x0));
  vec2 center(.5 * (size - 1), .5 * (size - 1));
  for (int x = 0; x < size; ++x) {
    for (int y = 0; y < size; ++y) {
      if (std::fmod(geometry::Distance(center, vec2(x, y)), 16) < 8)
        pixels.Set(ivec2(x, y), 0x1);
    }
  }
  MarchingSquares<ColorEqualPredicate> ms(ColorEqualPredicate(1), &pixels);
  while (state.KeepRunning()) {
    ms.TraceAllBoundaries();
  }
}
BENCHMARK(BM_TraceBullseye)->Arg(128)->Arg(256)->Arg(512)->Arg(1024);

static void BM_TraceCheckerboard(State &state) {
  int size = state.range(0);
  GPUPixels pixels(ivec2(size, size), std::vector<uint32_t>(size * size, 0x0));
  vec2 center(.5 * (size - 1), .5 * (size - 1));
  for (int x = 0; x < size; ++x) {
    for (int y = 0; y < size; ++y) {
      if ((x + y) % 2 == 0) pixels.Set(ivec2(x, y), 0x1);
    }
  }
  MarchingSquares<ColorEqualPredicate> ms(ColorEqualPredicate(1), &pixels);
  while (state.KeepRunning()) {
    ms.TraceAllBoundaries();
  }
}
BENCHMARK(BM_TraceCheckerboard)->Arg(128)->Arg(256)->Arg(512)->Arg(1024);

}  // namespace
}  // namespace marching_squares
}  // namespace ink
