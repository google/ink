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

#ifndef INK_ENGINE_SCENE_DATA_COMMON_INPUT_POINTS_H_
#define INK_ENGINE_SCENE_DATA_COMMON_INPUT_POINTS_H_

#include <vector>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/public/types/status.h"
#include "ink/engine/util/time/time_types.h"
#include "ink/proto/elements_portable_proto.pb.h"

namespace ink {
class InputPoints {
 public:
  void AddRawInputPoint(glm::vec2 point, InputTimeS time_s);
  void AddModeledInputPoint(glm::vec2 point, InputTimeS time_s, float radius);

  void Clear();

  // Changes the coordinates of all the points, in place.
  void TransformPoints(glm::mat4 transform);

  bool empty() const;
  size_t size() const;
  glm::vec2 point(size_t i) const;
  const std::vector<glm::vec2>& points() const;
  InputTimeS time_seconds(size_t i) const;
  const std::vector<InputTimeS>& times_seconds() const;

  // Warning: some precision loss, floating point values will be rounded to
  // integer. Typically, you should have called "TransformPoints" to move data
  // into a coordinate system where rounding to floats results in a small and
  // predictable precision loss.
  //
  static void CompressToProto(proto::Stroke* proto,
                              const InputPoints& input_points);

  static Status DecompressFromProto(const proto::Stroke& unsafe_proto,
                                    InputPoints* input_points);

  // This data is not persisted and does not round trip through proto.
  // For longform use only.
  bool HasModeledInput() const;
  const std::vector<glm::vec2>& GetModeledPoints() const;
  const std::vector<InputTimeS>& GetModeledTimes() const;
  const std::vector<float>& GetModeledRadii() const;

 private:
  std::vector<glm::vec2> raw_points_;
  std::vector<InputTimeS> raw_time_seconds_;

  // This data is not persisted and does not round trip through proto.
  // For longform use only.
  //
  // Modeled(raw_points) given the modeler and brush config of the line tool at
  // the time of input.
  std::vector<glm::vec2> modeled_points_;
  std::vector<InputTimeS> modeled_time_seconds_;
  std::vector<float> modeled_radii_;
};
}  // namespace ink


#endif  // INK_ENGINE_SCENE_DATA_COMMON_INPUT_POINTS_H_
