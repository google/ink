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

#include "ink/engine/input/drag_reco.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <utility>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/primitives/vector_utils.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"
#include "ink/engine/util/funcs/utils.h"

namespace ink {

template <>
std::string Str(const input::DragData& drag_data) {
  return Substitute("<DragData scale:$0 center:$1 drag:$2 rot:$3>",
                    drag_data.scale, drag_data.world_scale_center,
                    drag_data.world_drag, drag_data.rotation_radians);
}

namespace input {

// 2 fingers must come down within this time period for it to be detected as
// a drag. Seconds
const float kMaxTimeToDetectMultiFingerDragS = 0.3f;

// The minimum screen distance required between the first and second input
// points in order to detect scaling or rotation
const float kMinDistanceBetweenFingersToScaleOrRotate = 30.0f;

DragData::DragData() { Clear(); }

void DragData::Clear() {
  scale = 1;
  rotation_radians = 0;
  world_scale_center = glm::vec2(0, 0);
  world_drag = glm::vec2(0, 0);
}

DragReco::DragReco()
    : allow_one_finger_pan_(false), is_dragging_(false), first_down_time_(0) {}

void DragReco::Reset() {
  is_dragging_ = false;
  first_down_time_ = InputTimeS(0);
  drag_.Clear();
  active_ids_.clear();
  last_inputs_.clear();

  last_center_screen_ = glm::vec2(0);
  last_diff_vector_ = glm::vec2(0);
}

glm::vec2 DragReco::ComputeCenterScreen() {
  ASSERT(!active_ids_.empty());
  glm::vec2 center{0, 0};
  for (auto id : active_ids_) {
    ASSERT(last_inputs_.count(id) > 0);
    center += last_inputs_[id].screen_pos;
  }
  center /= active_ids_.size();
  return center;
}

glm::vec2 DragReco::ComputeActiveDifferenceVector() {
  ASSERT(active_ids_.size() == 2);
  auto iter = active_ids_.begin();
  uint32_t first_id = *iter;
  iter++;
  uint32_t second_id = *iter;
  return last_inputs_[second_id].screen_pos - last_inputs_[first_id].screen_pos;
}

void DragReco::OnUp(const InputData& data, const Camera& cam) {
  last_inputs_.erase(data.id);
  // check if finger lifted was active,
  if (active_ids_.count(data.id) > 0) {
    // remove it from active
    active_ids_.erase(data.id);
    // replace it if possible
    for (auto& finger : last_inputs_) {
      if (active_ids_.count(finger.first) == 0) {
        active_ids_.insert(finger.first);
        break;
      }
    }
  }

  if (active_ids_.empty()) {
    // Last finger up
    Reset();
  }
}

void DragReco::OnMove(const InputData& data, const Camera& cam) {
  if (last_inputs_.empty()) {
    ASSERT(first_down_time_ == InputTimeS(0));
    first_down_time_ = data.time;
  }

  last_inputs_[data.id] = data;

  if (active_ids_.size() < 2 && active_ids_.count(data.id) == 0) {
    active_ids_.insert(data.id);
    last_center_screen_ = ComputeCenterScreen();
    if (active_ids_.size() >= 2) {
      last_diff_vector_ = ComputeActiveDifferenceVector();
    }

    if (allow_one_finger_pan_ || active_ids_.size() >= 2) {
      if (data.time - first_down_time_ < kMaxTimeToDetectMultiFingerDragS) {
        is_dragging_ = true;
      }
    }
  }

  if (is_dragging_) {
    glm::vec2 current_center_screen = ComputeCenterScreen();
    drag_.world_drag =
        cam.ConvertVector(current_center_screen - last_center_screen_,
                          CoordType::kScreen, CoordType::kWorld);

    // scale and rotate
    drag_.world_scale_center = cam.ConvertPosition(
        current_center_screen, CoordType::kScreen, CoordType::kWorld);
    if (last_inputs_.size() > 1) {
      glm::vec2 new_diff_vector = ComputeActiveDifferenceVector();
      float old_dist = glm::length(last_diff_vector_);
      float new_dist = glm::length(new_diff_vector);
      if (old_dist > kMinDistanceBetweenFingersToScaleOrRotate &&
          new_dist > kMinDistanceBetweenFingersToScaleOrRotate) {
        drag_.scale = new_dist / old_dist;
        drag_.rotation_radians =
            SignedAngleBetweenVectors(last_diff_vector_, new_diff_vector);
      }
    }
  }
}

CaptureResult DragReco::OnInput(const InputData& data, const Camera& cam) {
  drag_.Clear();

  if (data.Get(Flag::InContact)) {
    OnMove(data, cam);
  } else {
    OnUp(data, cam);
  }
  ASSERT(active_ids_.size() ==
         std::min(static_cast<size_t>(2), last_inputs_.size()));

  if (!active_ids_.empty()) {
    last_center_screen_ = ComputeCenterScreen();
    if (active_ids_.size() == 2) {
      last_diff_vector_ = ComputeActiveDifferenceVector();
    }
  }
  return is_dragging_ ? CapResCapture : CapResObserve;
}

bool DragReco::GetDrag(DragData* data) {
  if (is_dragging_ && glm::length(drag_.world_drag) > 0) {
    *data = drag_;
    return true;
  }
  return false;
}
}  // namespace input
}  // namespace ink
