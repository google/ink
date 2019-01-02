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

// ░█▀▄░█▀█░░░█▀█░█▀█░▀█▀░░░█░█░█▀▀░█▀▀░░░▀█▀░█░█░▀█▀░█▀▀░░░█▀▀░▀█▀░█░░░█▀▀
// ░█░█░█░█░░░█░█░█░█░░█░░░░█░█░▀▀█░█▀▀░░░░█░░█▀█░░█░░▀▀█░░░█▀▀░░█░░█░░░█▀▀
// ░▀▀░░▀▀▀░░░▀░▀░▀▀▀░░▀░░░░▀▀▀░▀▀▀░▀▀▀░░░░▀░░▀░▀░▀▀▀░▀▀▀░░░▀░░░▀▀▀░▀▀▀░▀▀▀
//
// WARNING: this file was re-added to the repository because of the Wear
// Handwriting project. You probably don't want to use it (and we will
// hopefully re-delete it in the future. See b/29177172 for more context.

#if WEAR_HANDWRITING

#include "ink/engine/input/input_coalescer.h"

#include <cstdint>
#include <utility>

#include "ink/engine/input/input_dispatch.h"

namespace ink {
namespace input {

void InputCoalescer::QueueInput(InputDispatch* dispatch, const Camera& cam,
                                InputData data) {
  if (id_to_data_.find(data.id) == id_to_data_.end()) {
    id_to_data_[data.id] = std::deque<InputData>();
  }
  auto& q = id_to_data_[data.id];

  // If the flags haven't changed, remove the last input
  // (We'll use the new one instead)
  if (!q.empty() && q.back().flags == data.flags) {
    q.pop_back();
  }

  q.push_back(data);
}

bool InputCoalescer::PopMinTimeData(InputData* res) {
  // Find the data with the lowest time
  InputData min_data;
  bool found_min = false;
  for (auto ai : id_to_data_) {
    auto& q = ai.second;
    if (!found_min || q.begin()->time < min_data.time) {
      min_data = *q.begin();
      found_min = true;
    }
  }

  if (found_min) {
    // Remove that data from it's queue
    auto& q = id_to_data_[min_data.id];
    *res = min_data;
    q.pop_front();

    // Don't keep empty queues around in case input ids are not reused
    if (q.empty()) {
      id_to_data_.erase(min_data.id);
    }
    return true;
  }
  return false;
}

void InputCoalescer::DispatchQueuedInput(InputDispatch* dispatch,
                                         const Camera& cam) {
  InputData data;
  while (PopMinTimeData(&data)) {
    dispatch->Dispatch(cam, data);
  }
}

}  // namespace input
}  // namespace ink

#endif  // WEAR_HANDWRITING
