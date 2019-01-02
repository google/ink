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

#include "ink/engine/geometry/line/fat_line.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <memory>
#include <vector>

#include "third_party/absl/strings/str_format.h"
#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/camera/camera.h"
#include "ink/engine/geometry/algorithms/distance.h"
#include "ink/engine/geometry/algorithms/intersect.h"
#include "ink/engine/geometry/algorithms/simplify.h"
#include "ink/engine/geometry/algorithms/transform.h"
#include "ink/engine/geometry/line/tip/chisel_tip_model.h"
#include "ink/engine/geometry/line/tip/round_tip_model.h"
#include "ink/engine/geometry/line/tip_type.h"
#include "ink/engine/geometry/primitives/circle_utils.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/funcs/utils.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {

FatLine::FatLine(TipSizeScreen tip_size, uint32_t turn_verts, TipType tip_type)
    : min_screen_travel_threshold_(0),
      tip_size_(tip_size),
      turn_verts_(turn_verts) {
  fwd_.reserve(500);
  back_.reserve(500);
  pts_.reserve(500);
  end_cap_.reserve(20);
  SetTipType(tip_type);
  ClearVertices();
}

FatLine& FatLine::operator=(const FatLine& other) {
  on_add_vert_ = other.on_add_vert_;
  min_screen_travel_threshold_ = other.min_screen_travel_threshold_;
  down_cam_ = other.down_cam_;
  tip_size_ = other.tip_size_;
  min_radius_seen_ = other.min_radius_seen_;
  max_radius_seen_ = other.max_radius_seen_;
  stylus_state_ = other.stylus_state_;

  turn_verts_ = other.turn_verts_;
  last_center_ = other.last_center_;
  join_to_line_end_ = other.join_to_line_end_;
  SetTipType(other.tip_model_->GetTipType());

  fwd_ = other.fwd_;
  back_ = other.back_;
  start_cap_ = other.start_cap_;
  end_cap_ = other.end_cap_;
  pts_ = other.pts_;
  return *this;
}

void FatLine::ClearVertices() {
  fwd_.clear();
  back_.clear();
  pts_.clear();
  start_cap_.clear();
  end_cap_.clear();
  join_to_line_end_ = false;
  min_radius_seen_ = std::numeric_limits<float>::infinity();
  max_radius_seen_ = 0;
  stylus_state_ = input::kStylusStateUnknown;
  tip_model_->Clear();
}

bool FatLine::Extrude(glm::vec2 new_pt, InputTimeS time, bool force,
                      bool simplify) {
  last_center_ = new_pt;

  if (!pts_.empty() &&
      geometry::Distance(pts_[pts_.size() - 1].screen_position, new_pt) <
          min_screen_travel_threshold_) {
    if (!force) {
      return false;
    }

    // Force the extrusion. Modify new_pt so it's a stable numerical distance
    // away from the prior point
    glm::dvec2 proposed_point(new_pt);
    glm::dvec2 last_point(pts_.rbegin()->screen_position);
    double dist_to_proposed = geometry::Distance(proposed_point, last_point);

    if (dist_to_proposed < 0.001) {
      glm::dvec2 dir_to_proposed(0.001, 0);
      auto fixed_proposed =
          last_point +
          (dir_to_proposed * static_cast<double>(min_screen_travel_threshold_));
      new_pt = glm::vec2(fixed_proposed);
    }
  }

  last_extrude_time_ = time;
  MidPoint midpt;
  midpt.screen_position = new_pt;
  midpt.tip_size = tip_size_;
  midpt.time_sec = time;
  midpt.stylus_state = stylus_state_;

  if (!pts_.empty()) {
    // We let the tip model override the force parameter as the final point may
    // be, for example, within the previous point, and we don't want to allow
    // force to create a broken line.
    if (tip_model_->ShouldDropNewPoint(pts_.back(), midpt)) {
      return false;
    }

    if (tip_model_->ShouldPruneBeforeNewPoint(pts_, midpt)) {
      start_cap_.clear();
      back_.clear();
      fwd_.clear();
      pts_.clear();
    }
  }

  Append(&pts_, midpt);

  if (pts_.size() == 2) {
    if (join_to_line_end_) {
      tip_model_->AddTurnPoints(join_midpoint_, pts_[0], pts_[1], turn_verts_,
                                [this](glm::vec2 v) { AppendFwd(v); },
                                [this](glm::vec2 v) { AppendBack(v); });
    } else {
      BuildStartCap();
    }
  } else if (pts_.size() >= 3) {
    ExtendLine();
  }

  if (simplify) Simplify();
  return true;
}

void FatLine::BuildEndCap() {
  EXPECT(!pts_.empty());

  std::vector<glm::vec2> cap;
  if (pts_.size() == 1) {
    cap = PointsOnCircle(pts_[0].screen_position, pts_[0].tip_size.radius,
                         turn_verts_, 0, static_cast<float>(-M_TAU));
  } else {
    cap = tip_model_->CreateEndcap(pts_[pts_.size() - 2], pts_[pts_.size() - 1],
                                   turn_verts_);
  }

  end_cap_.clear();
  Append(&end_cap_, cap);
}

bool FatLine::SetStartCapToLineBack(const FatLine& other) {
  if (other.fwd_.empty() || other.back_.empty() || other.pts_.empty()) {
    return false;
  }
  EXPECT(fwd_.empty());
  EXPECT(back_.empty());
  EXPECT(pts_.empty());

  join_to_line_end_ = true;
  join_midpoint_ = other.pts_[other.pts_.size() - 2];
  MidPoint line_end = other.pts_.back();

  // Directly append instead of calling append. We explicitly want to use these
  // points without any modifiers so that the join is clean.
  fwd_.push_back(other.fwd_.back());
  back_.push_back(other.back_.back());

  SetTipSize(line_end.tip_size);
  SetStylusState(other.stylus_state_);
  Extrude(line_end.screen_position, line_end.time_sec, true);
  return true;
}

void FatLine::Simplify(uint32_t n_verts, float simplification_threshold) {
  auto trim = [n_verts, simplification_threshold](std::vector<Vertex>& v) {
    if (v.size() <= 1) return;  // No use simplifying a single point
    auto n = (n_verts < v.size()) ? n_verts : v.size();
    std::vector<Vertex> out;
    out.reserve(n);
    geometry::Simplify(v.end() - n, v.end(), simplification_threshold,
                       std::back_inserter(out),
                       [](const Vertex& v) { return v.position; });

    if (out.size() == n) {
      // No simplification
      return;
    }
    for (size_t i = 0; i < out.size(); i++) {
      v[i + v.size() - n] = out[i];
    }
    v.resize(v.size() - (n - out.size()));
  };

  trim(fwd_);
  trim(back_);
}

std::string FatLine::ToString() const {
  std::string result;
  if (!MidPoints().empty()) {
    result += "Midpoints:\n";
    for (auto& p : MidPoints()) {
      result +=
          absl::StrFormat("%10.3f %10.3f %10.3f %10.3f\n", p.screen_position.x,
                          p.screen_position.y, p.tip_size.radius,
                          static_cast<double>(p.time_sec));
    }
  }

  auto append_vertices = [&result](const std::string& label,
                                   const std::vector<Vertex>& points) {
    if (!points.empty()) {
      result += label;
      for (auto& p : points) {
        result +=
            absl::StrFormat("%10.3f %10.3f\n", p.position.x, p.position.y);
      }
    }
  };

  append_vertices("Forward:\n", ForwardLine());
  append_vertices("Backward:\n", BackwardLine());
  append_vertices("Start Cap:\n", StartCap());
  append_vertices("End Cap:\n", EndCap());

  return result;
}

void FatLine::BuildStartCap() {
  EXPECT(pts_.size() >= 2);

  fwd_.clear();
  back_.clear();

  std::vector<glm::vec2> cap =
      tip_model_->CreateStartcap(pts_[0], pts_[1], turn_verts_);
  if (!cap.empty()) {
    // Add the first and last points of the startcap to back_ and fwd_,
    // respectively, and everything else to startCap_.
    auto ai = cap.begin();
    Append(&back_, *ai);

    while (++ai != cap.end()) Append(&start_cap_, *ai);

    Append(&fwd_, *cap.rbegin());
  }
}

void FatLine::ExtendLine() {
  size_t n_points = pts_.size();
  EXPECT(n_points >= 3);

  tip_model_->AddTurnPoints(pts_[n_points - 3], pts_[n_points - 2],
                            pts_[n_points - 1], turn_verts_,
                            [this](glm::vec2 v) { AppendFwd(v); },
                            [this](glm::vec2 v) { AppendBack(v); });
}

// static
std::vector<glm::vec2> FatLine::OutlineAsArray(
    const std::vector<FatLine>& lines, const glm::mat4& world_to_object) {
  std::vector<glm::vec2> vec;
  // Particle-only lines (pencil etc) have no outline.
  if (lines.empty()) {
    return vec;
  }
  auto& cam = lines[0].DownCamera();
  auto AddPt = [&vec, &world_to_object, &cam](const glm::vec2& pt) -> void {
    // Use the down camera recorded line tool to transform screen to world.
    auto worldPt =
        cam.ConvertPosition(pt, CoordType::kScreen, CoordType::kWorld);
    // Use the object matrix inverse to transform world to object.
    vec.push_back(geometry::Transform(worldPt, world_to_object));
  };
  for (auto& pt : lines.begin()->StartCap()) {
    AddPt(pt.position);
  }
  for (auto& l : lines) {
    for (auto& pt : l.ForwardLine()) {
      AddPt(pt.position);
    }
  }
  for (auto& pt : lines.rbegin()->EndCap()) {
    AddPt(pt.position);
  }
  for (auto l = lines.rbegin(); l != lines.rend(); l++) {
    auto& back_line = l->BackwardLine();
    for (auto pt = back_line.rbegin(); pt != back_line.rend(); pt++) {
      AddPt(pt->position);
    }
  }
  return vec;
}

}  // namespace ink
