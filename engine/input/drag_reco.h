/*
 * Copyright 2018 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef INK_ENGINE_INPUT_DRAG_RECO_H_
#define INK_ENGINE_INPUT_DRAG_RECO_H_

#include <cstdint>
#include <map>
#include <set>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/camera/camera.h"
#include "ink/engine/input/input_data.h"
#include "ink/engine/input/input_handler.h"
#include "ink/engine/util/dbg/str.h"
#include "ink/engine/util/funcs/step_utils.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {

namespace input {

struct DragData {
  // The scale of the final distance between the touch points, relative to the
  // initial distance between the touch points. Note that for a pinch-zoom
  // effect, you'll need scale the camera by the inverse of this scale.
  float scale;
  // A positive value indicates counter-clockwise rotation w.r.t. a right-handed
  // coordinate system (i.e., (0,0) is in the lower-left corner).
  float rotation_radians;
  glm::vec2 world_scale_center{0, 0};
  glm::vec2 world_drag{0, 0};

  DragData();
  void Clear();
};

// Maintains state of if we are currently dragging, and can be queried to get
// updates to screen position/zoom based on previous drag.
//
// Current implementation (subject to change):
//
// Enter isDragging_ only when we go from zero to two fingers down in
// less than .3 seconds.  Remain in isDragging_ until zero fingers are
// down.
class DragReco {
 public:
  DragReco();
  void Reset();

  // Result is captured iff we are currently isDragging_.
  CaptureResult OnInput(const InputData& data, const Camera& cam);

  // Fills "data" with to the information needed to update the camera position
  // after a drag when returning true.
  //
  // Warning: can be false while isDragging_ (e.g. if there is no change in the
  // screen position).
  bool GetDrag(DragData* data);

  void SetAllowOneFingerPan(bool should_pan) {
    allow_one_finger_pan_ = should_pan;
  }

 private:
  glm::vec2 ComputeCenterScreen();
  glm::vec2 ComputeActiveDifferenceVector();

  void OnUp(const InputData& data, const Camera& cam);
  void OnMove(const InputData& data, const Camera& cam);

  bool allow_one_finger_pan_;

  bool is_dragging_;
  // The most recent time when the number of fingers with state down changed
  // from zero to one.
  InputTimeS first_down_time_;

  DragData drag_;

  // Will always be of size at most 2.
  std::set<uint32_t> active_ids_;
  // Most recent touch data observed for each finger by finger id.
  std::map<uint32_t, InputData> last_inputs_;
  glm::vec2 last_center_screen_{0, 0};
  // The vector difference of the second input minus the first input.
  glm::vec2 last_diff_vector_{0, 0};
};
}  // namespace input

template <>
std::string Str(const input::DragData& drag_data);

}  // namespace ink
#endif  // INK_ENGINE_INPUT_DRAG_RECO_H_
