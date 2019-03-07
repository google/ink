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

#include "third_party/absl/container/flat_hash_map.h"
#include "ink/engine/public/host/iengine_listener.h"
#include "ink/engine/scene/types/event_dispatch.h"
#include "ink/engine/service/dependencies.h"
#include "ink/proto/sengine_portable_proto.pb.h"

namespace ink {

namespace settings {

// See sengine.proto's Flag enum for explanations of these flags.
enum class Flag : uint32_t {
  ReadOnlyMode,
  EnablePanZoom,
  EnableRotation,
  EnableAutoPenMode,
  EnablePenMode,
  LowMemoryMode,
  OpaquePredictedSegment,
  CropModeEnabled,
  DebugTiles,
  DebugLineToolMesh,
  StrictNoMargins,
  KeepMeshesInCpuMemory,
  EnableFling,
  EnableHostCameraControl,
  EnableMotionBlur,
  EnableSelectionBoxHandles,
  EnablePartialDraw,
};
//     ../../proto/sengine.proto,
//     flags.cc)

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
  void SetFlag(proto::Flag proto_flag, bool value);
  bool GetFlag(Flag which) const;
  void AddListener(FlagListener* listener);
  void RemoveListener(FlagListener* listener);

 private:
  proto::Flag GetProtoFlag(settings::Flag which);

  // Maps the underlying value of a settings::Flag enum to the flag's boolean
  // value. If a flag is not found in the map, its value is considered false.
  // Default true values are set in the Flags constructor.
  absl::flat_hash_map<uint32_t, bool> values_;

  std::shared_ptr<EventDispatch<FlagListener>> dispatch_;
  std::shared_ptr<IEngineListener> engine_listener_;
};

}  // namespace settings

}  // namespace ink
#endif  // INK_ENGINE_SETTINGS_FLAGS_H_
