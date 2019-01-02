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

#include "ink/engine/geometry/spatial/texture_rtree_creator.h"

#include <algorithm>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include "ink/engine/geometry/algorithms/simplify.h"
#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/geometry/mesh/vertex.h"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/geometry/spatial/mesh_rtree.h"
#include "ink/engine/geometry/spatial/spatial_index.h"
#include "ink/engine/geometry/spatial/spatial_index_factory.h"
#include "ink/engine/geometry/tess/tessellator.h"
#include "ink/engine/processing/marching_squares.h"
#include "ink/engine/processing/runner/task_runner.h"
#include "ink/engine/rendering/baseGL/gpupixels.h"
#include "ink/engine/rendering/gl_managers/texture.h"
#include "ink/engine/scene/graph/scene_graph.h"
#include "ink/engine/scene/types/element_id.h"

namespace ink {
namespace spatial {

using glm::ivec2;
using marching_squares::ColorEqualPredicate;
using marching_squares::MarchingSquares;

static const std::array<ivec2, 9> kPixelOffsets = {
    {ivec2{-1, -1}, ivec2{-1, 0}, ivec2{-1, 1}, ivec2{0, -1}, ivec2{0, 0},
     ivec2{0, 1}, ivec2{1, -1}, ivec2{1, 0}, ivec2{1, 1}}};

TextureRTreeCreator::TextureRTreeCreator(
    std::weak_ptr<SpatialIndexFactory> weak_factory, std::string texture_uri,
    const Texture& texture)
    : weak_factory_(std::move(weak_factory)), texture_uri_(texture_uri) {
  // Fetching the pixels has to occur on the main thread.
  texture.GetPixels(&pixels_);
}

void TextureRTreeCreator::Execute() {
  if (weak_factory_.expired()) {
    return;
  }

  // We pre-process the texture, expanding the opaque region by 1 pixel in each
  // direction, to ensure the simplification doesn't cut anything off.
  GPUPixels processed_pixels = PreprocessTexture();
  ivec2 dim = processed_pixels.PixelDim();

  // The pixels are ABGR.
  MarchingSquares<ColorEqualPredicate> marching_squares(
      ColorEqualPredicate(0xFF000000), &processed_pixels);
  std::vector<std::vector<ivec2>> boundaries =
      marching_squares.TraceAllBoundaries();

  std::vector<std::vector<Vertex>> vertices(boundaries.size());
  for (int i = 0; i < boundaries.size(); ++i) {
    // Add the first point to the back of the boundary, or the simplification
    // won't consider the last point for removal.
    boundaries[i].push_back(boundaries[i].front());
    std::vector<ivec2> simplified;
    simplified.reserve(boundaries[i].size());
    geometry::Simplify(boundaries[i].begin(), boundaries[i].end(), 1,
                       std::back_inserter(simplified));
    simplified.pop_back();
    for (const auto& point : simplified) {
      vertices[i].emplace_back(point.x, dim.y - point.y);
    }
  }

  Tessellator tessellator;
  if (tessellator.Tessellate(vertices)) {
    index_.reset(new MeshRTree(OptimizedMesh(ShaderType::TexturedVertShader,
                                             tessellator.mesh_,
                                             Rect(0, 0, dim.x, dim.y))));
  }
}

void TextureRTreeCreator::OnPostExecute() {
  std::shared_ptr<SpatialIndexFactory> factory = weak_factory_.lock();
  if (factory) factory->RegisterTextureSpatialIndex(texture_uri_, index_);
}

GPUPixels TextureRTreeCreator::PreprocessTexture() const {
  ivec2 dim = pixels_.PixelDim();
  GPUPixels processed_pixels(dim, std::vector<uint32_t>(dim.x * dim.y, 0x0));
  for (int i = 0; i < dim.x; ++i) {
    for (int j = 0; j < dim.y; ++j) {
      ivec2 pos(i, j);
      for (const ivec2& offset : kPixelOffsets) {
        if (pixels_.InBounds(pos + offset) &&
            (pixels_.Get(pos + offset) & 0xFF000000)) {
          processed_pixels.Set(pos, 0xFF000000);
          break;
        }
      }
    }
  }

  return processed_pixels;
}

}  // namespace spatial
}  // namespace ink
