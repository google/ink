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

#include "ink/engine/realtime/line_builder.h"

#include "ink/engine/geometry/mesh/vertex.h"
#include "ink/engine/util/funcs/step_utils.h"

namespace ink {
namespace {
using util::Lerp;
using util::Normalize;
using util::Smoothstep;
}  // namespace

void LineBuilder::SetupNewLine(
    const Camera& down_camera, TipType tip_type, InputTimeS start_time,
    input::InputType input_type, std::unique_ptr<LineModifier> modifier,
    std::unique_ptr<LineModifier> prediction_modifier) {
  Clear();
  modifier_ = std::move(modifier);
  prediction_modifier_ = std::move(prediction_modifier);

  // Always refine_mesh and linearize_mesh_verts in the predicted line.
  prediction_modifier_->mutable_params().refine_mesh = true;
  prediction_modifier_->mutable_params().linearize_mesh_verts = true;

  vertex_callback_ = [this, start_time](glm::vec2 center_pt, float vert_radius,
                                        InputTimeS time, float pressure,
                                        Vertex* pt, std::vector<Vertex>* pts) {
    modifier_->OnAddVert(pt, center_pt, vert_radius, *pts, pressure);
    modifier_->ApplyAnimationToVert(pt, center_pt, vert_radius,
                                    time - start_time, *pts);
  };
  prediction_vertex_callback_ = [this, start_time, input_type](
                                    glm::vec2 center_pt, float vert_radius,
                                    InputTimeS time, float pressure, Vertex* pt,
                                    std::vector<Vertex>* pts) {
    modifier_->OnAddVert(pt, center_pt, vert_radius, *pts, pressure);
    modifier_->ApplyAnimationToVert(pt, center_pt, vert_radius,
                                    time - start_time, *pts);
    pt->color *= ModifyVertexOpacity(input_type, pt->position);
  };

  unstable_line_.SetupNewLine(
      down_camera, modifier_->GetMinScreenTravelThreshold(down_camera),
      tip_type, vertex_callback_, modifier_->params());
  predicted_line_.SetupNewLine(
      down_camera,
      prediction_modifier_->GetMinScreenTravelThreshold(down_camera), tip_type,
      prediction_vertex_callback_, prediction_modifier_->params());

  stable_mesh_.object_matrix = down_camera.ScreenToWorld();
  if (modifier_->params().texture_uri.empty()) {
    stable_mesh_.texture = nullptr;
  } else {
    stable_mesh_.texture =
        absl::make_unique<TextureInfo>(modifier_->params().texture_uri);
  }
}

void LineBuilder::Clear() {
  completed_lines_.clear();
  stable_mesh_.Clear();
  unstable_line_.ClearVertices();
  predicted_line_.ClearVertices();
  modifier_.reset();
  prediction_modifier_.reset();
  vertex_callback_ = nullptr;
  prediction_vertex_callback_ = nullptr;
}

void LineBuilder::ExtrudeModeledInput(
    const Camera& cam, const std::vector<input::ModeledInput>& modeled,
    bool is_line_end) {
  predicted_line_.ClearVertices();
  Extrude(cam, modeled, is_line_end, modifier_.get(), &unstable_line_);

  if (is_line_end) {
    unstable_line_.BuildEndCap();
    SplitLine();

    // Note: Since we don't synchronously regenerate back_mesh from scratch
    // here, any modification by ModifyFinalLine will only become visible once
    // the OptMesh is being drawn. Since we only use ModifyFinalLine to expand
    // very small strokes that are fast to process, this will only be ~1 frame
    // later.
    modifier_->ModifyFinalLine(&completed_lines_);
  } else if (unstable_line_.Line().MidPoints().size() >=
             modifier_->params().split_n) {
    // Split the line every so often so we aren't re-triangulating everything
    // constantly.
    SplitLine();
  }
}

void LineBuilder::ConstructPrediction(
    const Camera& cam,
    const std::vector<input::ModeledInput>& prediction_points) {
  const FatLine* last_line = LastNonEmptyLine();

  if (last_line != nullptr) {
    predicted_line_.RestartFromBackOfLine(*last_line,
                                          prediction_modifier_->params(),
                                          prediction_vertex_callback_);
    Extrude(cam, prediction_points, true, prediction_modifier_.get(),
            &predicted_line_);
    predicted_line_.BuildEndCap();
  } else {
    predicted_line_.ClearVertices();
    Extrude(cam, prediction_points, true, prediction_modifier_.get(),
            &predicted_line_);
    predicted_line_.BuildEndCap();
  }
}

void LineBuilder::Extrude(const Camera& cam,
                          const std::vector<input::ModeledInput>& modeled_input,
                          bool is_line_end, LineModifier* modifier,
                          TessellatedLine* tessellated_line) {
  EXPECT(modifier != nullptr);
  ASSERT(!modeled_input.empty());

  for (const input::ModeledInput& mi : modeled_input) {
    TipSizeScreen tip_size = mi.tip_size.ToScreen(cam);
    glm::vec2 screen_pos = cam.ConvertPosition(mi.world_pos, CoordType::kWorld,
                                               CoordType::kScreen);
    modifier->Tick(tip_size.radius, screen_pos, mi.time, cam);

    tessellated_line->Extrude(
        screen_pos, mi.time, tip_size, mi.stylus_state,
        modifier->params().NVertsAtRadius(tip_size.radius), is_line_end);
  }
}

int LineBuilder::MidPointCount() const {
  int n_midpoints = UnstableMidPoints().size();
  for (const auto& line : CompletedLines())
    n_midpoints += line.MidPoints().size();
  return n_midpoints;
}

const FatLine* LineBuilder::LastNonEmptyLine() const {
  if (unstable_line_.Line().MidPoints().size() > 1) {
    return &unstable_line_.Line();
  } else if (!completed_lines_.empty()) {
    ASSERT(completed_lines_.back().MidPoints().size() > 1);
    return &completed_lines_.back();
  } else {
    return nullptr;
  }
}

float LineBuilder::ModifyVertexOpacity(input::InputType input_type,
                                       glm::vec2 position) {
  if (flags_->GetFlag(settings::Flag::OpaquePredictedSegment)) {
    return 1.0;
  }
  float line_radius_screen = unstable_line_.Line().TipSize().radius;
  Camera cam = unstable_line_.Line().DownCamera();
  float line_radius_cm = cam.ConvertDistance(
      line_radius_screen, DistanceType::kScreen, DistanceType::kCm);

  float opacity_multiplier = 0;
  if (input_type != input::InputType::Touch) {
    if (line_radius_cm <= 0.2) {
      opacity_multiplier =
          Lerp(0.0f, 0.7f, Normalize(0.0f, 0.2f, line_radius_cm));
    } else {
      opacity_multiplier =
          Lerp(0.7f, 1.0f, Normalize(0.2f, 1.0f, line_radius_cm));
    }
  } else {
    // Make the predicted segment less opaque based on line radius. Thin
    // predicted segment tend to have no shared area with the final line, which
    // is visually very obvious.
    if (line_radius_cm <= 0.2) {
      opacity_multiplier =
          Lerp(0.15f, 0.3f, Normalize(0.0f, 0.2f, line_radius_cm));
    } else {
      opacity_multiplier =
          Lerp(0.3f, 0.4f, Normalize(0.2f, 0.5f, line_radius_cm));
    }

    const FatLine* last_line = LastNonEmptyLine();
    if (last_line != nullptr) {
      // Make the prediction more opaque the closer to last known position.
      //   - Prediction is very accurate for slow moving lines.
      //   - Predicted results only diverge as you move away from the known
      //   base.
      MidPoint last_midpoint = last_line->MidPoints().back();
      glm::vec2 dir_last_real_to_current =
          glm::normalize(position - last_midpoint.screen_position);
      glm::vec2 last_projected =
          last_midpoint.screen_position +
          (last_midpoint.tip_size.radius * dir_last_real_to_current);
      float projected_to_current_world_dist =
          glm::length(position - last_projected);
      float projected_to_current_cm_dist =
          cam.ConvertDistance(projected_to_current_world_dist,
                              DistanceType::kWorld, DistanceType::kCm);
      opacity_multiplier =
          Smoothstep(1.0f, opacity_multiplier,
                     Normalize(0.0f, 0.2f, projected_to_current_cm_dist));
    }
  }

  return opacity_multiplier;
}

void LineBuilder::SplitLine() {
  ASSERT(!unstable_line_.Line().MidPoints().empty());
  stable_mesh_.Append(UnstableMesh());
  gl_resources_->mesh_vbo_provider->ExtendVBO(&stable_mesh_, GL_DYNAMIC_DRAW);

  completed_lines_.push_back(unstable_line_.Line());
  unstable_line_.RestartFromBackOfLine(completed_lines_.back(),
                                       modifier_->params());
}

}  // namespace ink
