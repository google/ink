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

#include "ink/engine/input/sinput_test_helpers.h"

#include "testing/base/public/gmock.h"
#include "third_party/absl/strings/substitute.h"
#include "ink/engine/camera/camera.h"
#include "ink/engine/geometry/primitives/circle_utils.h"
#include "ink/engine/input/input_data.h"
#include "ink/engine/input/input_dispatch.h"
#include "ink/engine/input/input_handler.h"
#include "ink/engine/input/mock_input_handler.h"
#include "ink/engine/input/sinput.h"
#include "ink/engine/input/sinput_helpers.h"
#include "ink/engine/public/host/mock_engine_listener.h"
#include "ink/engine/settings/flags.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {
namespace sinput_test_helpers {

using input::Flag;
using input::InputData;
using input::InputDispatch;
using input::InputType;
using input::MockInputHandler;
using input::Priority;

using ::testing::_;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Return;

std::vector<SInput> CreateSampledLine(glm::vec2 from_screen_pos,
                                      glm::vec2 to_screen_pos,
                                      InputTimeS start_time_seconds,
                                      DurationS duration_seconds) {
  return CreateExactLine(from_screen_pos, to_screen_pos,
                         GetNumberOfInterpolationPoints(duration_seconds),
                         start_time_seconds,
                         start_time_seconds + duration_seconds);
}

std::vector<SInput> CreateSampledMultiTouchLines(glm::vec2 first_start_pos,
                                                 glm::vec2 first_end_pos,
                                                 glm::vec2 second_start_pos,
                                                 glm::vec2 second_end_pos,
                                                 InputTimeS start_time,
                                                 DurationS duration) {
  InputTimeS end_time = start_time + duration;
  int num_points = GetNumberOfInterpolationPoints(duration);
  return CreateExactMultiTouchLines(first_start_pos, first_end_pos,
                                    second_start_pos, second_end_pos,
                                    start_time, end_time, start_time + .01,
                                    end_time + .01, num_points, num_points);
}

std::vector<SInput> CreateExactLine(glm::vec2 from_screen_pos,
                                    glm::vec2 to_screen_pos,
                                    int num_interp_points,
                                    InputTimeS start_time,
                                    InputTimeS end_time) {
  return CreateExactLine(from_screen_pos, to_screen_pos, num_interp_points,
                         start_time, end_time, 1, true);
}

std::vector<SInput> CreateExactLine(glm::vec2 from_screen_pos,
                                    glm::vec2 to_screen_pos,
                                    int num_interp_points,
                                    InputTimeS start_time, InputTimeS end_time,
                                    uint32_t id, bool set_primary_flag) {
  EXPECT(num_interp_points >= 0);
  int total_points = num_interp_points + 2;
  std::vector<SInput> ans(total_points);
  uint32_t flags = FlagBitfield(Flag::InContact);
  if (set_primary_flag) flags |= FlagBitfield(Flag::Primary);

  ans[0].type = InputType::Touch;
  ans[0].id = id;
  ans[0].flags = flags;
  ans[0].time_s = start_time;
  ans[0].screen_pos = from_screen_pos;
  ans[0].pressure = 1;

  SInput& last = ans[total_points - 1];
  last.type = InputType::Touch;
  last.id = id;
  last.flags = flags;
  last.time_s = end_time;
  last.screen_pos = to_screen_pos;
  last.pressure = 1;

  for (int i = 1; i < total_points - 1; i++)
    ans[i] = util::Lerp(ans[0], last, i / (total_points - 1.0));

  ans[0].flags |= FlagBitfield(Flag::TDown);
  last.flags |= FlagBitfield(Flag::TUp);
  last.flags &= ~FlagBitfield(Flag::InContact);
  return ans;
}

std::vector<SInput> CreateExactMultiTouchLines(
    glm::vec2 first_start_pos, glm::vec2 first_end_pos,
    glm::vec2 second_start_pos, glm::vec2 second_end_pos,
    InputTimeS first_start_time, InputTimeS first_end_time,
    InputTimeS second_start_time, InputTimeS second_end_time,
    int first_num_points, int second_num_points) {
  auto line1 = CreateExactLine(first_start_pos, first_end_pos, first_num_points,
                               first_start_time, first_end_time, 1, true);
  auto line2 =
      CreateExactLine(second_start_pos, second_end_pos, second_num_points,
                      second_start_time, second_end_time, 2, false);
  std::vector<SInput> merged;
  std::merge(line1.begin(), line1.end(), line2.begin(), line2.end(),
             std::back_inserter(merged), SInput::LessThan);
  return merged;
}

std::vector<SInput> CreateTap(glm::vec2 screen_pos, InputTimeS start_time) {
  return CreateExactLine(screen_pos, screen_pos, 0, start_time,
                         start_time + .1);
}

std::vector<SInput> CreateArc(const glm::vec2& center, float radius,
                              int num_interp_points, float start_radians,
                              float end_radians, InputTimeS start_time,
                              InputTimeS end_time) {
  EXPECT(num_interp_points >= 0);
  int total_points = num_interp_points + 2;
  std::vector<SInput> result(total_points);

  for (int i = 0; i < total_points; ++i) {
    float ratio = static_cast<float>(i) / (total_points - 1);
    result[i].screen_pos = PointOnCircle(
        util::Lerp(start_radians, end_radians, ratio), radius, center);
    result[i].time_s = util::Lerp(start_time, end_time, ratio);
    result[i].pressure = 1;
    if (i == 0) {
      result[i].flags = FlagBitfield(Flag::InContact, Flag::TDown);
    } else if (i == total_points - 1) {
      result[i].flags = FlagBitfield(Flag::TUp);
    } else {
      result[i].flags = FlagBitfield(Flag::InContact);
    }
  }

  return result;
}

std::vector<SInput> CreateScrollWheelEvent(glm::vec2 screen_pos, float delta_x,
                                           float delta_y, InputTimeS time,
                                           uint32_t modifiers) {
  std::vector<SInput> result(1);
  result[0].screen_pos = screen_pos;
  result[0].time_s = time;
  result[0].wheel_delta_x = delta_x;
  result[0].wheel_delta_y = delta_y;
  result[0].flags = FlagBitfield(Flag::Wheel) | modifiers;
  return result;
}

int GetNumberOfInterpolationPoints(DurationS duration_seconds) {
  constexpr double kInputHz = 80;
  return static_cast<int>(
      std::max(0.0, kInputHz * static_cast<double>(duration_seconds) - 2));
}

proto::SInput Input(const proto::SInput::InputType type, const uint32_t id,
                    const uint32_t flags, const uint32_t time_s,
                    const double screen_pos_x, const double screen_pos_y,
                    const double pressure, const double wheel_delta_x,
                    const double wheel_delta_y, const double tilt,
                    const double orientation) {
  proto::SInput sinput;
  sinput.set_type(type);
  sinput.set_id(id);
  sinput.set_flags(flags);
  sinput.set_time_s(time_s);
  sinput.set_screen_pos_x(screen_pos_x);
  sinput.set_screen_pos_y(screen_pos_y);
  sinput.set_pressure(pressure);
  sinput.set_wheel_delta_x(wheel_delta_x);
  sinput.set_wheel_delta_y(wheel_delta_y);
  sinput.set_tilt(tilt);
  sinput.set_orientation(orientation);
  return sinput;
}

proto::SInputStream InputStream(const std::vector<proto::SInput>& sinputs) {
  proto::SInputStream stream;
  stream.set_screen_width(kScreenWidth);
  stream.set_screen_height(kScreenHeight);
  stream.set_screen_ppi(kScreenPpi);

  for (const proto::SInput& s : sinputs) {
    *stream.add_input() = s;
  }
  return stream;
}

proto::PlaybackStream MakePlaybackStream(
    const std::vector<proto::SInput>& sinputs) {
  proto::PlaybackStream stream;
  Camera initial_camera;
  initial_camera.SetScreenDim({kScreenWidth, kScreenHeight});
  initial_camera.SetPPI(kScreenPpi);
  Camera::WriteToProto(stream.mutable_initial_camera(), initial_camera);

  for (const proto::SInput& s : sinputs) {
    *stream.add_events()->mutable_sinput() = s;
  }
  return stream;
}
}  // namespace sinput_test_helpers
}  // namespace ink
