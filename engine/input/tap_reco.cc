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

#include "ink/engine/input/tap_reco.h"

#include <cstdint>

#include "third_party/absl/strings/substitute.h"
#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/input/input_data.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"

namespace ink {
namespace input {
namespace {
// Whether this InputData means this TapData has moved sufficiently far or
// with high velocity to mean it isn't a tap.
bool TapInvalidatedByMovement(const InputData& input_data,
                              const TapData& tap_data, const Camera& camera) {
  auto distance_from_down_cm = camera.ConvertDistance(
      glm::length(input_data.screen_pos - tap_data.down_data.screen_pos),
      DistanceType::kScreen, DistanceType::kCm);
  if (distance_from_down_cm > impl::kMaxLongPressDistanceCm) {
    return true;
  }
  if (input_data.DeltaTime() > 0) {
    auto delta_cm =
        camera.ConvertDistance(glm::length(input_data.DeltaScreen()),
                               DistanceType::kScreen, DistanceType::kCm);
    if (delta_cm / input_data.DeltaTime() > impl::kMaxLongPressSpeedCmPerSec) {
      return true;
    }
  }
  return false;
}
}  // namespace

std::string TapStatusToString(TapStatus status) {
  switch (status) {
    case TapStatus::NotStarted:
      return "NotStarted";
    case TapStatus::Ambiguous:
      return "Ambiguous";
    case TapStatus::LongPressHeld:
      return "LongPressHeld";
    case TapStatus::NoTap:
      return "NoTap";
    case TapStatus::Tap:
      return "Tap";
    case TapStatus::LongPressReleased:
      return "LongPressReleased";
    default:
      ASSERT(false);
      return "Unknown";
  }
}

TapData::TapData() : status(TapStatus::NotStarted) {}

bool TapData::IsTap() const { return status == TapStatus::Tap; }

bool TapData::IsAmbiguous() const { return status == TapStatus::Ambiguous; }

bool TapData::IsTerminalState() const {
  return status == TapStatus::LongPressReleased || status == TapStatus::NoTap ||
         status == TapStatus::Tap;
}

std::string TapData::ToString() const {
  return Substitute("tap status $0 for id: $1", TapStatusToString(status),
                    current_data.id);
}

////////////////////////////////////////////////////////////////

TapData TapReco::OnInput(const InputData& input_data, const Camera& camera) {
  auto tap_data = id_to_tap_data_[input_data.id];
  auto old_status = tap_data.status;
  tap_data.current_data = input_data;

  if (input_data.Get(Flag::TUp)) {
    tap_data.up_data = input_data;
  }

  if (input_data.Get(Flag::TDown)) {
    tap_data.down_data = input_data;
  }

  // All states change to NoTap if we see a Cancel
  if (input_data.Get(Flag::Cancel)) {
    tap_data.status = TapStatus::NoTap;
  }

  bool tap_invalidated_by_move =
      TapInvalidatedByMovement(input_data, tap_data, camera);
  bool is_long_hold =
      input_data.time - tap_data.down_data.time > impl::kLongPressThreshold;

  // See comment in tap_reco.h for the state machine.
  switch (tap_data.status) {
    case TapStatus::NotStarted:
      if (input_data.Get(Flag::TDown)) {
        tap_data.status = TapStatus::Ambiguous;
      }
      break;

    case TapStatus::Ambiguous:
      if (tap_invalidated_by_move) {
        tap_data.status = TapStatus::NoTap;
      } else if (input_data.Get(Flag::TUp)) {
        tap_data.status = TapStatus::Tap;
      } else if (is_long_hold) {
        tap_data.status = TapStatus::LongPressHeld;
      }
      break;

    case TapStatus::LongPressHeld:
      if (tap_invalidated_by_move) {
        tap_data.status = TapStatus::NoTap;
      } else if (input_data.Get(Flag::TUp)) {
        tap_data.status = TapStatus::LongPressReleased;
      }
      break;

    case TapStatus::NoTap:
    case TapStatus::Tap:
    case TapStatus::LongPressReleased:
      break;

    default:
      ASSERT(false);
  }

  if (tap_data.status != old_status && change_handler_) {
    change_handler_(tap_data);
  }

  // If we are in a terminal state, clear the data from the map. Otherwise write
  // the updated data back into the map.
  if (tap_data.IsTerminalState()) {
    id_to_tap_data_.erase(input_data.id);
  } else {
    id_to_tap_data_[input_data.id] = tap_data;
  }

  return tap_data;
}

void TapReco::Reset() { id_to_tap_data_.clear(); }

}  // namespace input
}  // namespace ink
