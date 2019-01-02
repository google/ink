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

#include "ink/engine/scene/data/common/input_points.h"

#include <cmath>
#include <memory>
#include <vector>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/algorithms/transform.h"
#include "ink/engine/geometry/primitives/matrix_utils.h"
#include "ink/engine/util/casts.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/funcs/utils.h"
#include "ink/engine/util/security.h"
#include "ink/engine/util/time/time_types.h"

using glm::ivec3;
using glm::mat4;
using glm::vec2;

using std::vector;

namespace ink {

void InputPoints::AddRawInputPoint(glm::vec2 point, InputTimeS time_s) {
  // Elide adjacent spatial duplicates
  if (!raw_points_.empty() && raw_points_.back() == point) {
    return;
  }
  raw_points_.push_back(point);
  raw_time_seconds_.push_back(time_s);
}

void InputPoints::AddModeledInputPoint(glm::vec2 point, InputTimeS time_s,
                                       float radius) {
  modeled_points_.push_back(point);
  modeled_time_seconds_.push_back(time_s);
  modeled_radii_.push_back(radius);
}

bool InputPoints::empty() const {
  ASSERT(raw_points_.size() == raw_time_seconds_.size());
  return raw_points_.empty();
}

size_t InputPoints::size() const {
  ASSERT(raw_points_.size() == raw_time_seconds_.size());
  return raw_points_.size();
}

glm::vec2 InputPoints::point(size_t i) const {
  ASSERT(i < raw_points_.size());
  return raw_points_[i];
}

const std::vector<glm::vec2>& InputPoints::points() const {
  return raw_points_;
}

InputTimeS InputPoints::time_seconds(size_t i) const {
  ASSERT(i < raw_time_seconds_.size());
  return raw_time_seconds_[i];
}

const std::vector<InputTimeS>& InputPoints::times_seconds() const {
  return raw_time_seconds_;
}

void InputPoints::Clear() {
  raw_points_.clear();
  raw_time_seconds_.clear();
  modeled_points_.clear();
  modeled_time_seconds_.clear();
  modeled_radii_.clear();
}

void InputPoints::TransformPoints(mat4 transform) {
  for (size_t i = 0; i < raw_points_.size(); i++) {
    raw_points_[i] = geometry::Transform(raw_points_[i], transform);
  }

  auto scale = matrix_utils::GetAverageAbsScale(transform);
  ASSERT(modeled_points_.size() == modeled_radii_.size());
  for (size_t i = 0; i < modeled_points_.size(); i++) {
    modeled_points_[i] = geometry::Transform(modeled_points_[i], transform);
    modeled_radii_[i] = scale * modeled_radii_[i];
  }
}

bool InputPoints::HasModeledInput() const {
  ASSERT(modeled_points_.size() == modeled_time_seconds_.size());
  ASSERT(modeled_points_.size() == modeled_radii_.size());
  return !modeled_points_.empty();
}
const std::vector<glm::vec2>& InputPoints::GetModeledPoints() const {
  ASSERT(modeled_points_.size() == modeled_time_seconds_.size());
  ASSERT(modeled_points_.size() == modeled_radii_.size());
  return modeled_points_;
}
const std::vector<InputTimeS>& InputPoints::GetModeledTimes() const {
  ASSERT(modeled_points_.size() == modeled_time_seconds_.size());
  ASSERT(modeled_points_.size() == modeled_radii_.size());
  return modeled_time_seconds_;
}
const std::vector<float>& InputPoints::GetModeledRadii() const {
  ASSERT(modeled_points_.size() == modeled_time_seconds_.size());
  ASSERT(modeled_points_.size() == modeled_radii_.size());
  return modeled_radii_;
}

namespace {
const int kMsPerSecond = 1000;

// Round stroke x,y data to integers (they are in object coordinates, this
// should not cause serious data loss).  Moves the stroke timing data from
// double in seconds, with 0 as system start time, to ints representing
// milliseconds, where 0 is the time of the first point in the stroke.
//
// Note: it is better to convert to an integer type before delta encoding than
// after.
vector<ivec3> ChangeStrokeUnits(const InputPoints& stroke_data,
                                InputTimeS start_time_s) {
  vector<ivec3> ans;
  for (size_t i = 0; i < stroke_data.size(); i++) {
    vec2 pos = stroke_data.point(i);
    DurationS rel_time_s = stroke_data.time_seconds(i) - start_time_s;
    double rel_time_ms_double =
        std::round(kMsPerSecond * static_cast<double>(rel_time_s));
    ASSERT(rel_time_ms_double <= std::numeric_limits<int32_t>::max());
    ASSERT(rel_time_ms_double >= std::numeric_limits<int32_t>::lowest());
    int32_t rel_time_ms = SafeNumericCast<int64_t, int32_t>(rel_time_ms_double);
    ans.push_back(ivec3(std::round(pos.x), std::round(pos.y), rel_time_ms));
  }
  return ans;
}

}  // namespace

// static
void InputPoints::CompressToProto(proto::Stroke* proto,
                                  const InputPoints& input_points) {
  if (input_points.empty()) {
    return;
  }
  InputTimeS start_time_s = input_points.time_seconds(0);
  uint64_t start_time_ms = static_cast<uint64_t>(
      std::round(static_cast<double>(start_time_s) * kMsPerSecond));
  proto->set_start_time_ms(start_time_ms);

  vector<ivec3> integer_stroke = ChangeStrokeUnits(input_points, start_time_s);

  // Delta encode position and time: The first value is the initial
  // offset. Each following value is the difference from the last.
  ivec3 previous(0, 0, 0);
  ivec3 delta{0, 0, 0};
  for (size_t i = 0; i < integer_stroke.size(); i++) {
    delta = integer_stroke[i] - previous;
    proto->add_point_x(delta.x);
    proto->add_point_y(delta.y);
    proto->add_point_t_ms(delta.z);
    previous = integer_stroke[i];
  }
}

// static
Status InputPoints::DecompressFromProto(const proto::Stroke& unsafe_proto,
                                        InputPoints* input_points) {
  int size_x = unsafe_proto.point_x().size();
  int size_y = unsafe_proto.point_y().size();
  int size_t_ms = unsafe_proto.point_t_ms().size();
  if (size_x != size_y || size_x != size_t_ms) {
    return ErrorStatus(
        StatusCode::INVALID_ARGUMENT,
        "Could not decode midpoint data, num x points, num y points, and num "
        "times should be equal, but found x=$0, y=$1, and t=$2.",
        size_x, size_y, size_t_ms);
  }
  if (!BoundsCheckIncEx(size_x, 0, 100000)) {
    return ErrorStatus(
        StatusCode::OUT_OF_RANGE,
        "Cannot decompress (x,y,t) data, found more than 100k points.");
  }
  *input_points = InputPoints();
  std::vector<vec2> de_delta_position;
  std::vector<int32_t> times;
  ivec3 current_sum(0, 0, 0);
  // Undo delta encoding
  // use exact arithmetic to avoid accumulating floating point errors, many add
  // operations in a row.
  de_delta_position.reserve(size_x);
  times.reserve(size_x);
  for (int i = 0; i < size_x; i++) {
    if (unsafe_proto.point_t_ms(i) >
        static_cast<uint32_t>(std::numeric_limits<int32_t>::max())) {
      return ErrorStatus(
          StatusCode::OUT_OF_RANGE,
          "Cannot decompress (x,y,t) data, found a time that was too large to "
          "be a signed int.");
    }
    ivec3 next_point(unsafe_proto.point_x(i), unsafe_proto.point_y(i),
                     unsafe_proto.point_t_ms(i));
    if (AddOverflowsSigned(current_sum.x, next_point.x) ||
        AddOverflowsSigned(current_sum.y, next_point.y) ||
        AddOverflowsSigned(current_sum.z, next_point.z)) {
      return ErrorStatus(
          StatusCode::OUT_OF_RANGE,
          "Cannot decompress (x,y,t) data, overflowed when removing delta "
          "encoding.");
    }
    current_sum += next_point;
    de_delta_position.emplace_back(current_sum.x, current_sum.y);
    times.emplace_back(current_sum.z);
  }
  for (int i = 0; i < size_x; i++) {
    // change the time from ms to seconds, add back in start time.
    if (AddOverflowsUnsigned(
            static_cast<uint64_t>(unsafe_proto.start_time_ms()),
            static_cast<uint64_t>(times[i]))) {
      return ErrorStatus(
          StatusCode::OUT_OF_RANGE,
          "Cannot decompress (x,y,t) data, overflowed when adding start time "
          "to time at position $0.",
          i);
    }
    int64_t time_ms = unsafe_proto.start_time_ms() + times[i];
    InputTimeS time_seconds(time_ms / static_cast<double>(kMsPerSecond));
    input_points->AddRawInputPoint(vec2(de_delta_position[i]), time_seconds);
  }
  return OkStatus();
}

}  // namespace ink
