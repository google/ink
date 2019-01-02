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

// Non-Ion flags.

#ifndef INK_ENGINE_SETTINGS_FLAGS_H_
#define INK_ENGINE_SETTINGS_FLAGS_H_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <unordered_map>

#include "ink/engine/public/host/iengine_listener.h"
#include "ink/engine/scene/types/event_dispatch.h"
#include "ink/engine/service/dependencies.h"
#include "ink/proto/sengine_portable_proto.pb.h"

namespace ink {

namespace settings {

// See sengine.proto's Flag enum for explanations of these flags.
enum class Flag : uint32_t {
  ReadOnlyMode,
  PanZoomEnabled,
  RotationEnabled,
  AutoPenModeEnabled,
  PenModeEnabled,
  LowMemoryMode,
  OpaquePredictedSegment,
  CropModeEnabled,
  DebugTiles,
  DebugLineToolMesh,
  StrictNoMargins,
  KeepMeshesInCpuMemory,
  EnableFling,
  EnableHostCameraControl,
};

struct FlagHash {
  size_t operator()(const settings::Flag& v) const {
    return std::hash<uint32_t>()(static_cast<uint32_t>(v));
  }
};

class FlagListener : public EventListener<FlagListener> {
 public:
  virtual void OnFlagChanged(Flag which, bool new_value) = 0;
};

// Flags are boolean values set at runtime to control ink engine behavior.
// They are typically set in client code, e.g. through SEngine::assignFlag().
// Internally, classes can register themselves as listeners for flag value
// changes or request a flag's value directly.
//
// Note that the default flag value is not always false, see values_ below for
// default values.
class Flags {
 public:
  using SharedDeps = service::Dependencies<IEngineListener>;

  explicit Flags(std::shared_ptr<IEngineListener> engine_listener);
  void SetFlag(Flag which, bool value);
  void SetFlag(const proto::Flag& proto_flag, bool value);
  bool GetFlag(Flag which) const;
  void AddListener(FlagListener* listener);
  void RemoveListener(FlagListener* listener);

 private:
  proto::Flag GetProtoFlag(settings::Flag which);

  std::unordered_map<Flag, bool, FlagHash> values_ = {
      // Blank comments are to make clang-format vertically-align this list.
      {Flag::ReadOnlyMode, false},             //
      {Flag::PanZoomEnabled, true},            //
      {Flag::RotationEnabled, false},          //
      {Flag::AutoPenModeEnabled, false},       //
      {Flag::PenModeEnabled, false},           //
      {Flag::LowMemoryMode, false},            //
      {Flag::OpaquePredictedSegment, false},   //
      {Flag::CropModeEnabled, false},          //
      {Flag::DebugTiles, false},               //
      {Flag::DebugLineToolMesh, false},        //
      {Flag::StrictNoMargins, false},          //
      {Flag::KeepMeshesInCpuMemory, false},    //
      {Flag::EnableFling, false},              //
      {Flag::EnableHostCameraControl, false},  //
  };

  std::shared_ptr<EventDispatch<FlagListener>> dispatch_;
  std::shared_ptr<IEngineListener> engine_listener_;
};

}  // namespace settings

}  // namespace ink
#endif  // INK_ENGINE_SETTINGS_FLAGS_H_
