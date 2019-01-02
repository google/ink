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

#ifndef INK_ENGINE_INPUT_SINPUT_H_
#define INK_ENGINE_INPUT_SINPUT_H_

#include <vector>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/camera/camera.h"
#include "ink/engine/input/input_data.h"
#include "ink/engine/public/types/status.h"
#include "ink/engine/util/funcs/step_utils.h"
#include "ink/engine/util/time/time_types.h"
#include "ink/proto/sengine_portable_proto.pb.h"

namespace ink {

// A lightweight struct holding the arguments to the sketchology public API for
// input.
//
// Keep in sync with SEngine::dispatchInput().
// Keep in sync with proto::SInput
struct SInput {
  input::InputType type = input::InputType::Touch;
  uint32_t id = 1;
  uint32_t flags = 0;
  InputTimeS time_s;

  // WARNING These coordinates assume (0,0) in the top left of the screen!
  // x increases to the right and y increases going down.
  glm::vec2 screen_pos{0, 0};
  float pressure = 0;
  float tilt = 0;
  float orientation = 0;
  float wheel_delta_x = 0;
  float wheel_delta_y = 0;

  SInput();
  SInput(const input::InputData& data, const Camera& cam);
  bool operator==(const SInput& other) const;
  std::string ToString() const;

  static SInput Lerpnc(SInput from, SInput to, float amount);

  static ABSL_MUST_USE_RESULT Status
  ReadFromProto(const proto::SInput& unsafe_proto, SInput* sinput);
  static void WriteToProto(proto::SInput* proto_sinput,
                           const SInput& obj_sinput);

  // Compares two SInputs, first by time, then by ID. Returns true if lhs is
  // strictly-less-than rhs.
  static bool LessThan(const SInput& lhs, const SInput& rhs);
};

namespace util {
template <>
SInput Lerpnc(SInput from, SInput to, float amount);
}  // namespace util

}  // namespace ink

#endif  // INK_ENGINE_INPUT_SINPUT_H_
