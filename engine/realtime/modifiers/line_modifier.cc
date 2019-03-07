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

#include "ink/engine/realtime/modifiers/line_modifier.h"

#include <cmath>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/brushes/size/tip_size_world.h"
#include "ink/engine/colors/colors.h"
#include "ink/engine/geometry/algorithms/distance.h"
#include "ink/engine/geometry/line/fat_line.h"
#include "ink/engine/geometry/mesh/shader_type.h"
#include "ink/engine/input/input_data.h"
#include "ink/engine/realtime/modifiers/line_animation.h"
#include "ink/engine/util/funcs/rand_funcs.h"
#include "ink/engine/util/funcs/step_utils.h"
#include "ink/engine/util/funcs/utils.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {

// Only lines shorter than this distance trigger the expand behavior
static const float kTriggerExpandWhenShorterThanCm = 0.08f;
// Radius of the dot will be the same a brush traveling at this speed
static const float kExpandToSizeAtSpeed = 30;  // cm/s

using util::Clamp01;
using util::Lerp;
using util::Normalize;

LineModParams::LineModParams()
    : split_n(40),
      n_verts(glm::ivec2(20, 75)),
      refine_mesh(false),
      linearize_mesh_verts(false),
      linearize_combined_verts(false) {}

int LineModParams::NVertsAtRadius(float radius_screen) const {
  auto amt = Clamp01(Normalize(30.0f, 150.0f, radius_screen));
  return Lerp(n_verts.x, n_verts.y, amt);
}

////////////////////////////////////////////////////////////////

ShaderType LineModifier::GetShaderType() {
  return ShaderType::SingleColorShader;
}

LineModifier::LineModifier(const LineModParams& params, const glm::vec4& rgb)
    : params_(params),
      rgba_(rgb),
      speed_(0),
      distance_traveled_cm_(0),
      last_time_(0) {}

LineModifier::~LineModifier() {}

float LineModifier::GetMinScreenTravelThreshold(const Camera& cam) {
  return 2.5;
}

void LineModifier::OnAddVert(Vertex* vert, glm::vec2 center_pt, float radius,
                             float pressure) {
  auto prgb = RGBtoRGBPremultiplied(rgba_);
  vert->color = prgb;
}

void LineModifier::SetupNewLine(const input::InputData& data,
                                const Camera& camera) {
  last_time_ = InputTimeS(0);
  distance_traveled_cm_ = 0;
  noise_filter_ = signal_filters::ExpMovingAvg<double, double>(0.5, 0.8);
}

void LineModifier::Tick(float screen_radius, glm::vec2 new_position_screen,
                        InputTimeS time, const Camera& cam) {
  cam_ = cam;

  // update speed
  if (last_time_ == InputTimeS(0)) {
    speed_ = 10;
  } else {
    double screen_dist = geometry::Distance(last_pos_, new_position_screen);
    DurationS time_delta = time - last_time_;
    float speed = speed_;
    if (time_delta != 0) {
      // screen px / second
      double screen_speed = screen_dist / static_cast<double>(time_delta);
      speed = cam.ConvertDistance(screen_speed, DistanceType::kScreen,
                                  DistanceType::kCm);
    }

    // more drag when slowing down than speeding up
    auto drag = 0.97;
    if (speed > speed_) drag = 0.93;
    speed_ = (speed_ * (drag) + (speed * (1 - drag)));
  }

  // update distance
  if (last_time_ != InputTimeS(0)) {
    auto screen_dist = geometry::Distance(new_position_screen, last_pos_);
    auto cm_dist = cam_.ConvertDistance(screen_dist, DistanceType::kScreen,
                                        DistanceType::kCm);
    distance_traveled_cm_ += cm_dist;
  }

  last_pos_ = new_position_screen;
  last_time_ = time;
}

void LineModifier::ApplyAnimationToVert(Vertex* vert, glm::vec2 center_pt,
                                        float radius,
                                        DurationS time_since_tdown) {
  if (animation_) {
    animation_->ApplyToVert(vert, center_pt, radius, time_since_tdown, params_);
  }
}

bool LineModifier::ModifyFinalLine(std::vector<FatLine>* line_segments) {
  if (line_segments->empty()) return false;

  // Try and expand small strokes. First, figure out if the line is small
  // enough that we want to expand it.
  auto& first_line = (*line_segments)[0];
  bool expand_stroke =
      brush_params_.expand_small_strokes &&
      distance_traveled_cm_ < kTriggerExpandWhenShorterThanCm &&
      !first_line.MidPoints().empty();
  if (expand_stroke) {
    line_segments->resize(1);
    auto pt = first_line.MidPoints()[0];
    first_line.ClearVertices();

    // Figure out what the larger size should be
    float size = brush_params_.size.WorldSize(cam_);
    TipSizeScreen new_size =
        TipSizeWorld::FromSpeed(brush_params_.shape_params.GetRadius(size),
                                brush_params_.shape_params.speed_limit,
                                brush_params_.shape_params.base_speed,
                                kExpandToSizeAtSpeed)
            .ToScreen(cam_);
    (*line_segments)[0].SetTurnVerts(params_.NVertsAtRadius(new_size.radius));
    first_line.SetTipSize(new_size);

    // Note this calls back into into ourselves (onAddVert and
    // applyAnimationToVert)
    first_line.Extrude(pt.screen_position, pt.time_sec, false);
    first_line.Extrude(pt.screen_position, pt.time_sec, true);
    first_line.BuildEndCap();
    return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////

}  // namespace ink
