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

#include "ink/engine/geometry/spatial/spatial_index.h"

#include "testing/base/public/benchmark.h"
#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/geometry/mesh/mesh_test_helpers.h"
#include "ink/engine/geometry/mesh/vertex.h"
#include "ink/engine/geometry/primitives/circle_utils.h"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/geometry/spatial/mesh_rtree.h"

namespace ink {
namespace spatial {
namespace {

constexpr ShaderType kDefaultShaderType = ShaderType::TexturedVertShader;

using benchmark::State;
using glm::mat4;

OptimizedMesh MakeRingOptMesh(int subdivisions, ShaderType shader_type) {
  return OptimizedMesh(shader_type, MakeRingMesh({0, 0}, 9, 11, subdivisions));
}

OptimizedMesh MakeSineWaveOptMesh(int subdivisions, ShaderType shader_type) {
  return OptimizedMesh(shader_type,
                       MakeSineWaveMesh({0, 0}, 2, .2, 10, 1, subdivisions));
}

// Micro-benchmarks only accept integer arguments, so we specify the shader type
// as a template parameter.
template <typename IndexType, ShaderType SType>
static void BM_RingMeshConstruction(State &state) {
  OptimizedMesh m = MakeRingOptMesh(state.range(0), SType);
  while (state.KeepRunning()) {
    IndexType index(m);
  }
}
BENCHMARK_TEMPLATE(BM_RingMeshConstruction, MeshRTree,
                   ShaderType::ColoredVertShader)
    ->Range(8, 4096);
BENCHMARK_TEMPLATE(BM_RingMeshConstruction, MeshRTree,
                   ShaderType::SingleColorShader)
    ->Range(8, 4096);
BENCHMARK_TEMPLATE(BM_RingMeshConstruction, MeshRTree,
                   ShaderType::TexturedVertShader)
    ->Range(8, 4096);

template <typename IndexType, ShaderType SType>
static void BM_SineWaveMeshConstruction(State &state) {
  OptimizedMesh m = MakeSineWaveOptMesh(state.range(0), SType);
  while (state.KeepRunning()) {
    IndexType index(m);
  }
}
BENCHMARK_TEMPLATE(BM_SineWaveMeshConstruction, MeshRTree,
                   ShaderType::ColoredVertShader)
    ->Range(8, 4096);
BENCHMARK_TEMPLATE(BM_SineWaveMeshConstruction, MeshRTree,
                   ShaderType::SingleColorShader)
    ->Range(8, 4096);
BENCHMARK_TEMPLATE(BM_SineWaveMeshConstruction, MeshRTree,
                   ShaderType::TexturedVertShader)
    ->Range(8, 4096);

template <typename IndexType>
static void BM_RingMeshIntersect(State &state) {
  IndexType index(MakeRingOptMesh(state.range(0), kDefaultShaderType));
  Rect region(7, -1, 12, 1);
  while (state.KeepRunning()) {
    index.Intersects(region, mat4{1});
  }
}
BENCHMARK_TEMPLATE(BM_RingMeshIntersect, MeshRTree)->Range(8, 4096);

template <typename IndexType>
static void BM_RingMeshMiss(State &state) {
  IndexType index(MakeRingOptMesh(state.range(0), kDefaultShaderType));
  Rect region(20, 20, 30, 30);
  while (state.KeepRunning()) {
    index.Intersects(region, mat4{1});
  }
}
BENCHMARK_TEMPLATE(BM_RingMeshMiss, MeshRTree)->Range(8, 4096);

template <typename IndexType>
static void BM_RingMeshContainedInRegion(State &state) {
  IndexType index(MakeRingOptMesh(state.range(0), kDefaultShaderType));
  Rect region(-15, -15, 15, 15);
  while (state.KeepRunning()) {
    index.Intersects(region, mat4{1});
  }
}
BENCHMARK_TEMPLATE(BM_RingMeshContainedInRegion, MeshRTree)->Range(8, 4096);

template <typename IndexType>
static void BM_RingMeshRegionInsideRing(State &state) {
  IndexType index(MakeRingOptMesh(state.range(0), kDefaultShaderType));
  Rect region(-3, -3, 3, 3);
  while (state.KeepRunning()) {
    index.Intersects(region, mat4{1});
  }
}
BENCHMARK_TEMPLATE(BM_RingMeshRegionInsideRing, MeshRTree)->Range(8, 4096);

template <typename IndexType>
static void BM_RingMeshMbr(State &state) {
  IndexType index(MakeRingOptMesh(state.range(0), kDefaultShaderType));
  while (state.KeepRunning()) {
    index.Mbr(mat4{1});
  }
}
BENCHMARK_TEMPLATE(BM_RingMeshMbr, MeshRTree)->Range(8, 4096);

template <typename IndexType>
static void BM_SineWaveMeshIntersect(State &state) {
  IndexType index(MakeSineWaveOptMesh(state.range(0), kDefaultShaderType));
  Rect region(4, -1, 6, 1);
  while (state.KeepRunning()) {
    index.Intersects(region, mat4{1});
  }
}
BENCHMARK_TEMPLATE(BM_SineWaveMeshIntersect, MeshRTree)->Range(8, 4096);

template <typename IndexType>
static void BM_SineWaveMeshNearMiss(State &state) {
  IndexType index(MakeSineWaveOptMesh(state.range(0), kDefaultShaderType));
  Rect region(3, 1, 4, 2);
  while (state.KeepRunning()) {
    index.Intersects(region, mat4{1});
  }
}
BENCHMARK_TEMPLATE(BM_SineWaveMeshNearMiss, MeshRTree)->Range(8, 4096);

template <typename IndexType>
static void BM_SineWaveMeshContainedInRegion(State &state) {
  IndexType index(MakeSineWaveOptMesh(state.range(0), kDefaultShaderType));
  Rect region(-5, -5, 15, 5);
  while (state.KeepRunning()) {
    index.Intersects(region, mat4{1});
  }
}
BENCHMARK_TEMPLATE(BM_SineWaveMeshContainedInRegion, MeshRTree)->Range(8, 4096);

template <typename IndexType>
static void BM_SineWaveMeshMiss(State &state) {
  IndexType index(MakeSineWaveOptMesh(state.range(0), kDefaultShaderType));
  Rect region(-10, 10, -5, 15);
  while (state.KeepRunning()) {
    index.Intersects(region, mat4{1});
  }
}
BENCHMARK_TEMPLATE(BM_SineWaveMeshMiss, MeshRTree)->Range(8, 4096);

template <typename IndexType>
static void BM_SineWaveMeshMbr(State &state) {
  IndexType index(MakeSineWaveOptMesh(state.range(0), kDefaultShaderType));
  while (state.KeepRunning()) {
    index.Mbr(mat4{1});
  }
}
BENCHMARK_TEMPLATE(BM_SineWaveMeshMbr, MeshRTree)->Range(8, 4096);

}  // namespace
}  // namespace spatial
}  // namespace ink
