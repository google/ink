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

#include "ink/engine/settings/flags.h"

namespace ink {

namespace settings {

Flags::Flags(std::shared_ptr<IEngineListener> engine_listener)
    : dispatch_(new EventDispatch<FlagListener>),
      engine_listener_(engine_listener) {}

void Flags::SetFlag(Flag which, bool value) {
  values_[which] = value;

  dispatch_->Send(&FlagListener::OnFlagChanged, which, value);
  engine_listener_->FlagChanged(GetProtoFlag(which), value);
}

bool Flags::GetFlag(Flag which) const {
  if (values_.find(which) == values_.end()) {
    return false;
  }
  return values_.at(which);
}

void Flags::SetFlag(const proto::Flag& proto_flag, bool value) {
  settings::Flag flag;
  switch (proto_flag) {
    case proto::Flag::READ_ONLY_MODE:
      flag = settings::Flag::ReadOnlyMode;
      break;
    case proto::Flag::ENABLE_PAN_ZOOM:
      flag = settings::Flag::PanZoomEnabled;
      break;
    case proto::Flag::ENABLE_ROTATION:
      flag = settings::Flag::RotationEnabled;
      break;
    case proto::Flag::ENABLE_AUTO_PEN_MODE:
      flag = settings::Flag::AutoPenModeEnabled;
      break;
    case proto::Flag::ENABLE_PEN_MODE:
      flag = settings::Flag::PenModeEnabled;
      break;
    case proto::Flag::LOW_MEMORY_MODE:
      flag = settings::Flag::LowMemoryMode;
      break;
    case proto::Flag::OPAQUE_PREDICTED_SEGMENT:
      flag = settings::Flag::OpaquePredictedSegment;
      break;
    case proto::Flag::CROP_MODE_ENABLED:
      flag = settings::Flag::CropModeEnabled;
      break;
    case proto::Flag::DEBUG_TILES:
      flag = settings::Flag::DebugTiles;
      break;
    case proto::Flag::DEBUG_LINE_TOOL_MESH:
      flag = settings::Flag::DebugLineToolMesh;
      break;
    case proto::Flag::STRICT_NO_MARGINS:
      flag = settings::Flag::StrictNoMargins;
      break;
    case proto::Flag::KEEP_MESHES_IN_CPU_MEMORY:
      flag = settings::Flag::KeepMeshesInCpuMemory;
      break;
    case proto::Flag::ENABLE_FLING:
      flag = settings::Flag::EnableFling;
      break;
    case proto::Flag::ENABLE_HOST_CAMERA_CONTROL:
      flag = settings::Flag::EnableHostCameraControl;
      break;
    default:
      SLOG(SLOG_ERROR, "Unknown flag sent to assignFlag");
      return;
  }
  SetFlag(flag, value);
}

proto::Flag Flags::GetProtoFlag(settings::Flag which) {
  proto::Flag flag;
  switch (which) {
    case settings::Flag::ReadOnlyMode:
      flag = proto::Flag::READ_ONLY_MODE;
      break;
    case settings::Flag::PanZoomEnabled:
      flag = proto::Flag::ENABLE_PAN_ZOOM;
      break;
    case settings::Flag::RotationEnabled:
      flag = proto::Flag::ENABLE_ROTATION;
      break;
    case settings::Flag::AutoPenModeEnabled:
      flag = proto::Flag::ENABLE_AUTO_PEN_MODE;
      break;
    case settings::Flag::PenModeEnabled:
      flag = proto::Flag::ENABLE_PEN_MODE;
      break;
    case settings::Flag::LowMemoryMode:
      flag = proto::Flag::LOW_MEMORY_MODE;
      break;
    case settings::Flag::OpaquePredictedSegment:
      flag = proto::Flag::OPAQUE_PREDICTED_SEGMENT;
      break;
    case settings::Flag::CropModeEnabled:
      flag = proto::Flag::CROP_MODE_ENABLED;
      break;
    case settings::Flag::DebugTiles:
      flag = proto::Flag::DEBUG_TILES;
      break;
    case settings::Flag::DebugLineToolMesh:
      flag = proto::Flag::DEBUG_LINE_TOOL_MESH;
      break;
    case settings::Flag::StrictNoMargins:
      flag = proto::Flag::STRICT_NO_MARGINS;
      break;
    case settings::Flag::KeepMeshesInCpuMemory:
      flag = proto::Flag::KEEP_MESHES_IN_CPU_MEMORY;
      break;
    case settings::Flag::EnableFling:
      flag = proto::Flag::ENABLE_FLING;
      break;
    case settings::Flag::EnableHostCameraControl:
      flag = proto::Flag::ENABLE_HOST_CAMERA_CONTROL;
      break;
    default:
      SLOG(SLOG_ERROR, "Unknown flag in Flags::GetFlag");
      flag = proto::Flag::UNKNOWN;
  }
  return flag;
}

void Flags::AddListener(FlagListener* listener) {
  listener->RegisterOnDispatch(dispatch_);
}

void Flags::RemoveListener(FlagListener* listener) {
  listener->Unregister(dispatch_);
}

}  // namespace settings
}  // namespace ink
