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

#include "ink/engine/geometry/mesh/mesh_splitter.h"

#include "testing/base/public/benchmark.h"
#include "testing/base/public/gunit.h"
#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/geometry/mesh/mesh_test_helpers.h"

namespace ink {
namespace {

static void BM_DisjointMeshes(benchmark::State &state) {
  int n_subdivisions = state.range(0);
  auto base_mesh = MakeRingMesh({0, 0}, 10, 20, n_subdivisions);
  auto cutting_mesh = MakeRingMesh({0, 0}, 30, 40, n_subdivisions);
  while (state.KeepRunning()) {
    Mesh result_mesh;
    MeshSplitter splitter(base_mesh);
    splitter.Split(cutting_mesh);
    splitter.GetResult(&result_mesh);
  }
}
BENCHMARK(BM_DisjointMeshes)->Range(8, 1024);

static void BM_SplitRingWithRing(benchmark::State &state) {
  int n_subdivisions = state.range(0);
  auto base_mesh = MakeRingMesh({0, 0}, 15, 20, n_subdivisions);
  auto cutting_mesh = MakeRingMesh({12.5, 0}, 15, 20, n_subdivisions);
  while (state.KeepRunning()) {
    Mesh result_mesh;
    MeshSplitter splitter(base_mesh);
    splitter.Split(cutting_mesh);
    splitter.GetResult(&result_mesh);
  }
}
BENCHMARK(BM_SplitRingWithRing)->Range(8, 1024);

static void BM_SplitWaveWithWave(benchmark::State &state) {
  int n_subdivisions = state.range(0);
  auto base_mesh = MakeSineWaveMesh({0, 0}, 100, .02, 100, 20, n_subdivisions);
  auto cutting_mesh =
      MakeSineWaveMesh({1, 0}, 100, .02, 100, 20, n_subdivisions);
  while (state.KeepRunning()) {
    Mesh result_mesh;
    MeshSplitter splitter(base_mesh);
    splitter.Split(cutting_mesh);
    splitter.GetResult(&result_mesh);
  }
}
BENCHMARK(BM_SplitWaveWithWave)->Range(8, 1024);

}  // namespace
}  // namespace ink
