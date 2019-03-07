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

#include "ink/engine/input/sinput_helpers.h"

#include "third_party/absl/types/optional.h"
#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/input/input_handler.h"
#include "ink/engine/public/host/iengine_listener.h"
#include "ink/engine/public/host/public_events.h"

namespace ink {
namespace input {
namespace {
// Makes a copy of all InputData values passed to it.
class EventHandler : public IInputHandler {
 public:
  CaptureResult OnInput(const InputData& data, const Camera&) override {
    data_.push_back(ProcessedInputData(index_, data));
    return CapResCapture;
  }

  bool RefuseAllNewInput() override { return false; }

  // Sets the index that will be associated with the next InputData
  // to be seen.
  void set_index(const size_t index) { index_ = index; }

  absl::optional<Cursor> CurrentCursor(const Camera& camera) const override {
    return absl::nullopt;
  }

  Priority InputPriority() const override { return Priority::Default; }

  // Retrieves input events that have been captured by this event
  // handler. Do not use this object after moving out of this rvalue
  // reference.
  std::vector<ProcessedInputData>&& TakeData() { return std::move(data_); }
  inline std::string ToString() const override { return "<EventHandler>"; }

 private:
  std::vector<ProcessedInputData> data_;
  size_t index_;
};

Camera StreamCamera(const proto::SInputStream& input_stream) {
  Camera cam;
  cam.SetScreenDim({input_stream.screen_width(), input_stream.screen_height()});
  cam.SetPPI(input_stream.screen_ppi());
  return cam;
}
}  // namespace

bool ProcessPlaybackStream(
    const proto::PlaybackStream& playback_stream,
    std::function<bool(const size_t, const Camera&)> camera_on_input_fn,
    std::function<bool(const size_t, const Camera&, SInput)> sinput_fn) {
  Camera cam;
  if (!Camera::ReadFromProto(playback_stream.initial_camera(), &cam)) {
    SLOG(SLOG_ERROR,
         "PlaybackStream does not have valid initial camera settings");
    return false;
  }

  for (size_t event_index = 0; event_index < playback_stream.events_size();
       event_index++) {
    const proto::PlaybackEvent& event = playback_stream.events(event_index);
    SInput sinput;
    Camera new_camera;
    switch (event.event_case()) {
      case proto::PlaybackEvent::kSinput:
        if (!SInput::ReadFromProto(event.sinput(), &sinput)) {
          SLOG(SLOG_ERROR, "PlaybackStream has invalid SInput");
          return false;
        }
        if (sinput_fn != nullptr && !sinput_fn(event_index, cam, sinput)) {
          return true;
        }
        break;
      case proto::PlaybackEvent::kCameraOnInput:
        if (!Camera::ReadFromProto(event.camera_on_input(), &new_camera)) {
          SLOG(SLOG_ERROR, "PlaybackStream camera changed to an invalid state");
          return false;
        }
        cam = new_camera;
        if (camera_on_input_fn != nullptr &&
            !camera_on_input_fn(event_index, cam)) {
          return true;
        }
        break;
      case proto::PlaybackEvent::EVENT_NOT_SET:
        SLOG(SLOG_ERROR, "PlaybackEvent does not have an actual event set");
        return false;
      default:
        SLOG(SLOG_ERROR, "Unhandled PlaybackEvent type");
        break;
    }
  }
  return true;
}

InputData ConvertToRawInputData(const SInput& sinput, const Camera& cam) {
  InputData data;
  data.type = sinput.type;
  data.id = sinput.id;
  data.flags = sinput.flags;
  data.time = sinput.time_s;
  data.screen_pos.x = sinput.screen_pos.x;
  data.screen_pos.y = cam.ScreenDim().y - sinput.screen_pos.y;
  data.pressure = sinput.pressure;
  data.wheel_delta_x = sinput.wheel_delta_x;
  data.wheel_delta_y = sinput.wheel_delta_y;

  data.world_pos = cam.ConvertPosition(data.screen_pos, CoordType::kScreen,
                                       CoordType::kWorld);

  return data;
}

std::vector<ProcessedInputData> ConvertToProcessedInputData(
    const proto::SInputStream& input_stream) {
  const Camera cam = StreamCamera(input_stream);
  InputDispatch dispatch(
      std::make_shared<settings::Flags>(std::make_shared<PublicEvents>()));
  EventHandler handler;
  dispatch.RegisterHandler(&handler);
  for (size_t index = 0; index < input_stream.input_size(); index++) {
    SInput sinput;
    SInput::ReadFromProto(input_stream.input(index), &sinput).IgnoreError();
    handler.set_index(index);
    dispatch.Dispatch(cam, ConvertToRawInputData(sinput, cam));
  }
  return handler.TakeData();
}

std::vector<ProcessedInputData> ConvertToProcessedInputData(
    const proto::PlaybackStream& playback_stream) {
  InputDispatch dispatch(
      std::make_shared<settings::Flags>(std::make_shared<PublicEvents>()));
  EventHandler handler;
  dispatch.RegisterHandler(&handler);
  if (!ProcessPlaybackStream(
          playback_stream,
          /*camera_on_input_fn=*/nullptr,
          /*sinput_fn=*/
          [&handler, &dispatch](const size_t index, const Camera& cam,
                                SInput sinput) {
            InputData data = ConvertToRawInputData(sinput, cam);
            handler.set_index(index);
            dispatch.Dispatch(cam, data);
            return true;
          })) {
    SLOG(SLOG_ERROR, "Error reading playback stream");
  }
  return handler.TakeData();
}

Slice::Slice(const size_t start, const size_t end, const bool any_down)
    : start(start), end(end), any_down(any_down) {}

bool Slice::operator==(const Slice& other) const {
  return start == other.start && end == other.end && any_down == other.any_down;
}

bool Slice::operator!=(const Slice& other) const {
  return start != other.start || end != other.end || any_down != other.any_down;
}

std::vector<Slice> SliceBoundaries(const proto::SInputStream& input_stream) {
  proto::PlaybackStream playback_stream;
  Camera::WriteToProto(playback_stream.mutable_initial_camera(),
                       StreamCamera(input_stream));
  for (const proto::SInput& sinput : input_stream.input()) {
    *playback_stream.add_events()->mutable_sinput() = sinput;
  }
  return SliceBoundaries(playback_stream);
}

std::vector<Slice> SliceBoundaries(
    const proto::PlaybackStream& playback_stream) {
  std::vector<ProcessedInputData> inputs =
      ConvertToProcessedInputData(playback_stream);
  std::vector<Slice> slices;
  auto start = inputs.begin(), end = inputs.begin();

  while (end != inputs.end()) {
    if (end->data.n_down > 0) {
      // At least one pen is down. Read through events until all pens are up.
      while (end != inputs.end() && end->data.n_down > 0) {
        ++end;
      }
      if (end != inputs.end()) ++end;  // "Pen up" belongs to this slice.
      slices.emplace_back(Slice(/*start=*/start->index,
                                /*end=*/(end - 1)->index + 1,
                                /*any_down=*/true));
      start = end;
    } else {
      // No pens are down. Read through events until a pen goes down.
      while (end != inputs.end() && end->data.n_down == 0) {
        ++end;
      }
      slices.emplace_back(Slice(/*start=*/start->index,
                                /*end=*/(end - 1)->index + 1,
                                /*any_down=*/false));
      start = end;
    }
  }

  return slices;
}

bool ValidateCamera(const proto::SInputStream& input_stream) {
  proto::Viewport viewport;
  viewport.set_ppi(input_stream.screen_ppi());
  viewport.set_width(input_stream.screen_width());
  viewport.set_height(input_stream.screen_height());
  return Camera::IsValidViewport(viewport).ok();
}

void AppendStream(const proto::PlaybackStream& src,
                  proto::PlaybackStream* dest) {
  if (src.events_size() == 0) return;
  if (dest->events_size() == 0) {
    *dest = src;
    return;
  }

  // Find the last camera setting in dest.
  const proto::CameraSettings* last_camera = &dest->initial_camera();
  for (auto i = dest->events().rbegin(); i != dest->events().rend(); i++) {
    if (i->has_camera_on_input()) {
      last_camera = &i->camera_on_input();
      break;
    }
  }

  if (last_camera->viewport().width() !=
          src.initial_camera().viewport().width() ||
      last_camera->viewport().height() !=
          src.initial_camera().viewport().height() ||
      last_camera->viewport().ppi() != src.initial_camera().viewport().ppi() ||
      last_camera->position().world_center().x() !=
          src.initial_camera().position().world_center().x() ||
      last_camera->position().world_center().y() !=
          src.initial_camera().position().world_center().y() ||
      last_camera->position().world_width() !=
          src.initial_camera().position().world_width() ||
      last_camera->position().world_height() !=
          src.initial_camera().position().world_height()) {
    *dest->add_events()->mutable_camera_on_input() = src.initial_camera();
  }

  for (const auto& event : src.events()) {
    *dest->add_events() = event;
  }
}

}  // namespace input
}  // namespace ink
