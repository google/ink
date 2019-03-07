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

#include "ink/engine/geometry/tess/tessellated_line.h"

#include "ink/engine/geometry/tess/cdrefinement.h"
#include "ink/engine/geometry/tess/color_linearizer.h"
#include "ink/engine/rendering/gl_managers/mesh_vbo_provider.h"

namespace ink {

void TessellatedLine::SetupNewLine(float min_screen_travel_threshold,
                                   TipType tip_type,
                                   FatLine::VertAddFn vertex_callback,
                                   const LineModParams& line_mod_params) {
  ClearVertices();
  params_ = line_mod_params;
  if (params_.texture_uri.empty()) {
    tessellator_.mesh_.texture = nullptr;
  } else {
    tessellator_.mesh_.texture =
        absl::make_unique<TextureInfo>(params_.texture_uri);
  }
  line_.SetMinScreenTravelThreshold(min_screen_travel_threshold);
  line_.SetTipType(tip_type);
  line_.SetVertCallback(vertex_callback);
}

OptRect TessellatedLine::RestartFromBackOfLine(
    const FatLine& other, const LineModParams& line_mod_params,
    absl::optional<FatLine::VertAddFn> vertex_callback) {
  SetupNewLine(other.MinScreenTravelThreshold(), other.GetTipType(),
               vertex_callback.value_or(other.VertCallback()), line_mod_params);
  return line_.SetStartCapToLineBack(other);
}

OptRect TessellatedLine::Extrude(glm::vec2 new_pt, InputTimeS time,
                                 TipSizeScreen tip_size,
                                 input::StylusState stylus_state,
                                 int n_turn_verts, bool force) {
  line_.SetTipSize(tip_size);
  line_.SetStylusState(stylus_state);
  line_.SetTurnVerts(n_turn_verts);
  OptRect new_region = line_.Extrude(new_pt, time, force);
  mesh_dirty_ = mesh_dirty_ || new_region.has_value();
  return new_region;
}

OptRect TessellatedLine::BuildEndCap() {
  mesh_dirty_ = true;
  has_end_cap_ = true;
  return line_.BuildEndCap();
}

void TessellatedLine::ClearVertices() {
  tessellator_.ClearGeometry();
  mesh_dirty_ = false;
  has_end_cap_ = false;
  line_.ClearVertices();
}

const Mesh& TessellatedLine::GetMesh() const {
  if (!mesh_dirty_) {
    return tessellator_.mesh_;
  }

  tessellator_.ClearGeometry();
  if (tessellator_.Tessellate(line_, has_end_cap_) && tessellator_.HasMesh()) {
    if (params_.refine_mesh) {
      auto cdr = CDR(&tessellator_.mesh_);
      cdr.RefineMesh();
      auto clr = ColorLinearizer(&tessellator_.mesh_);
      if (params_.linearize_combined_verts) {
        clr.LinearizeCombinedVerts();
      }
      if (params_.linearize_mesh_verts) {
        clr.LinearizeAllVerts();
      }
    }
    gl_resources_->mesh_vbo_provider->ReplaceVBOs(&tessellator_.mesh_,
                                                  GL_DYNAMIC_DRAW);
  } else {
    tessellator_.mesh_.Clear();
  }
  mesh_dirty_ = false;
  return tessellator_.mesh_;
}

}  // namespace ink
