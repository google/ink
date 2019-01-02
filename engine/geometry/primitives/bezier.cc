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

#include "ink/engine/geometry/primitives/bezier.h"

namespace ink {

using glm::vec2;

Bezier::Bezier() { AdvanceSegment(); }

void Bezier::AdvanceSegment() {
  polyline_.emplace_back();
  current_segment_ = &polyline_.back();
}

void Bezier::PushToCurrentSegment(glm::vec2 v) {
  // If last_seen_ is in the current_segment_ (eg we didn't just do a MOVE_TO
  // jump), add the distance we traveled to the running path length.
  if (!current_segment_->empty()) {
    current_length_ += glm::distance(v, last_seen_);
  }
  current_segment_->push_back(v);
  last_seen_ = v;
}

void Bezier::MoveTo(vec2 v) {
  // MOVE_TO creates a new segment unless the current path is already empty.
  if (!current_segment_->empty()) {
    AdvanceSegment();
  }
  PushToCurrentSegment(v);
  last_moved_to_ = v;
}

void Bezier::LineTo(vec2 v) { PushToCurrentSegment(v); }

static vec2 BezierQuad(float t, const vec2& a, const vec2& cp, const vec2& b) {
  const float t_inv = 1.0f - t;
  return (a * (t_inv * t_inv)) + (cp * (2 * t_inv * t)) + (b * (t * t));
}

static vec2 BezierCubic(float t, const vec2& a, const vec2& cp1,
                        const vec2& cp2, const vec2& b) {
  return (BezierQuad(t, a, cp1, cp2) * (1.0f - t)) +
         (BezierQuad(t, cp1, cp2, b) * t);
}

void Bezier::CurveTo(vec2 cp1, vec2 cp2, vec2 to) {
  for (int i = 1; i <= num_eval_points_; ++i) {
    float t = i / static_cast<float>(num_eval_points_);
    vec2 v = BezierCubic(t, last_seen_, cp1, cp2, to);
    PushToCurrentSegment(v);
  }
}

void Bezier::CurveTo(vec2 cp, vec2 to) {
  for (int i = 1; i <= num_eval_points_; ++i) {
    float t = i / static_cast<float>(num_eval_points_);
    vec2 v = BezierQuad(t, last_seen_, cp, to);
    PushToCurrentSegment(v);
  }
}

void Bezier::Close() { PushToCurrentSegment(last_moved_to_); }

void Bezier::SetNumEvalPoints(int num_eval_points) {
  num_eval_points_ = num_eval_points;
}

float Bezier::PathLength() const { return current_length_; }

}  // namespace ink
