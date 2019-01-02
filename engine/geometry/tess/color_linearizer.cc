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

#include "ink/engine/geometry/tess/color_linearizer.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <random>
#include <utility>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/colors/colors.h"
#include "ink/engine/geometry/algorithms/distance.h"
#include "ink/engine/util/dbg/errors.h"
namespace ink {

ColorLinearizer::ColorLinearizer(Mesh* mesh) : mesh_(mesh) {
  EXPECT(mesh_->idx.size() % 3 == 0);
  ntris_ = mesh_->idx.size() / 3;
  tris_ = reinterpret_cast<MeshTriangle*>(&mesh_->idx[0]);

  Initdata();
}

void ColorLinearizer::Initdata() {
  for (uint32_t i = 0; i < ntris_; i++) {
    auto& tri = tris_[i];
    if (!tri.Valid()) continue;

    for (uint32_t j = 0; j < 3; j++) {
      auto s = tri.Segment(j);
      pttoseg_.insert(std::pair<uint16_t, uint16_t>(s.idx[0], s.idx[1]));
      pttoseg_.insert(std::pair<uint16_t, uint16_t>(s.idx[1], s.idx[0]));
    }
  }
}

void ColorLinearizer::LinearizeCombinedVerts() {
  for (int i = 0; i < 3; i++) {
    std::shuffle(mesh_->combined_idx.begin(), mesh_->combined_idx.end(),
                 std::mt19937());
    Pass(mesh_->combined_idx.begin(), mesh_->combined_idx.end(), 0.1f);
  }
}

void ColorLinearizer::LinearizeAllVerts() {
  std::vector<uint16_t> ui(mesh_->verts.size());
  for (size_t i = 0; i < ui.size(); i++) ui[i] = i;

  for (int j = 0; j < 2; j++) {
    std::shuffle(ui.begin(), ui.end(), std::mt19937());
    for (int i = 0; i < 1; i++) Pass(ui.begin(), ui.end(), 0.35f);
  }
}

void ColorLinearizer::Pass(std::vector<uint16_t>::iterator from,
                           std::vector<uint16_t>::iterator to, float amt) {
  for (auto ai = from; ai != to; ai++) {
    ASSERT(*ai < mesh_->verts.size());
    auto& pt = mesh_->verts[*ai];
    auto ohsv = RGBtoHSV(pt.color);
    auto rng = pttoseg_.equal_range(*ai);
    if (rng.first == rng.second) continue;

    std::vector<float> weights;
    std::vector<glm::vec4> colors_hsv;
    for (auto aj = rng.first; aj != rng.second; aj++) {
      ASSERT(aj->second < mesh_->verts.size());
      auto nbr = mesh_->verts[aj->second];
      weights.push_back(geometry::Distance(pt.position, nbr.position));
      colors_hsv.push_back(RGBtoHSV(nbr.color));
    }

    // normalize weights
    float sum_dist = 0;
    for (float d : weights) sum_dist += d;
    for (float& d : weights) d /= sum_dist;

    auto nhsv = glm::vec4(0);
    for (size_t i = 0; i < weights.size(); i++) {
      nhsv += (colors_hsv[i] * weights[i]);
    }
    nhsv = nhsv * amt + ohsv * (1.0f - amt);
    pt.color = HSVtoRGB(nhsv);
  }
}
}  // namespace ink
