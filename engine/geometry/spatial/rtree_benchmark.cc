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

#include "ink/engine/geometry/spatial/rtree.h"

#include "testing/base/public/benchmark.h"
#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/util/funcs/rand_funcs.h"

namespace ink {
namespace spatial {
namespace {

Rect Bounds(glm::vec2 v) { return Rect::CreateAtPoint(v, {0, 0}); }
const Rect &Bounds(const Rect &r) { return r; }

void PopulateRandom(glm::vec2 *v) { *v = {Drand(-100, 100), Drand(-100, 100)}; }
void PopulateRandom(Rect *r) {
  *r = Rect::CreateAtPoint({Drand(-100, 100), Drand(-100, 100)},
                           {Drand(.1, 10), Drand(.1, 10)});
}

template <typename T>
std::vector<T> GenerateData(int n_elements, uint64_t seed) {
  Seed_random(seed);
  std::vector<T> data(n_elements);
  for (auto &d : data) PopulateRandom(&d);
  return data;
}

template <typename DataType>
void BM_BulkLoad(benchmark::State &state) {
  auto data = GenerateData<DataType>(state.range(0), 0);
  auto bounds_func = [](const DataType &d) { return Bounds(d); };
  for (auto _ : state) {
    RTree<DataType>(data.begin(), data.end(), bounds_func);
  }
}
BENCHMARK_TEMPLATE(BM_BulkLoad, glm::vec2)->Range(8, 4096);
BENCHMARK_TEMPLATE(BM_BulkLoad, Rect)->Range(8, 4096);

template <typename DataType>
void BM_Insert(benchmark::State &state) {
  auto data = GenerateData<DataType>(state.range(0), 0);
  auto bounds_func = [](const DataType &d) { return Bounds(d); };
  for (auto _ : state) {
    RTree<DataType> rtree(bounds_func);
    for (const auto &d : data) rtree.Insert(d);
  }
}
BENCHMARK_TEMPLATE(BM_Insert, glm::vec2)->Range(8, 4096);
BENCHMARK_TEMPLATE(BM_Insert, Rect)->Range(8, 4096);

template <typename DataType>
void BM_FindAny(benchmark::State &state) {
  auto data = GenerateData<DataType>(state.range(0), 0);
  auto bounds_func = [](const DataType &d) { return Bounds(d); };
  RTree<DataType> rtree(data.begin(), data.end(), bounds_func);
  for (auto _ : state) {
    rtree.FindAny({{-25, -25}, {75, 75}});
  }
}
BENCHMARK_TEMPLATE(BM_FindAny, glm::vec2)->Range(8, 4096);
BENCHMARK_TEMPLATE(BM_FindAny, Rect)->Range(8, 4096);

template <typename DataType>
void BM_FindAll(benchmark::State &state) {
  auto data = GenerateData<DataType>(state.range(0), 0);
  auto bounds_func = [](const DataType &d) { return Bounds(d); };
  RTree<DataType> rtree(data.begin(), data.end(), bounds_func);
  std::vector<DataType> output;
  output.reserve(state.range(0));
  for (auto _ : state) {
    output.clear();
    rtree.FindAll({{-25, -25}, {75, 75}}, std::back_inserter(output));
    break;
  }
}
BENCHMARK_TEMPLATE(BM_FindAll, glm::vec2)->Range(8, 4096);
BENCHMARK_TEMPLATE(BM_FindAll, Rect)->Range(8, 4096);

template <typename DataType>
void BM_Remove(benchmark::State &state) {
  auto data = GenerateData<DataType>(state.range(0), 0);
  auto bounds_func = [](const DataType &d) { return Bounds(d); };
  for (auto _ : state) {
    StopBenchmarkTiming();
    RTree<DataType> rtree(data.begin(), data.end(), bounds_func);
    StartBenchmarkTiming();
    rtree.Remove({{-25, -25}, {75, 75}});
  }
}
BENCHMARK_TEMPLATE(BM_Remove, glm::vec2)->Range(8, 4096);
BENCHMARK_TEMPLATE(BM_Remove, Rect)->Range(8, 4096);

template <typename DataType>
void BM_RemoveAll(benchmark::State &state) {
  auto data = GenerateData<DataType>(state.range(0), 0);
  auto bounds_func = [](const DataType &d) { return Bounds(d); };
  for (auto _ : state) {
    StopBenchmarkTiming();
    RTree<DataType> rtree(data.begin(), data.end(), bounds_func);
    StartBenchmarkTiming();
    rtree.RemoveAll({{-25, -25}, {75, 75}});
  }
}
BENCHMARK_TEMPLATE(BM_RemoveAll, glm::vec2)->Range(8, 4096);
BENCHMARK_TEMPLATE(BM_RemoveAll, Rect)->Range(8, 4096);

}  // namespace
}  // namespace spatial
}  // namespace ink
