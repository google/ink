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

// Helper functions for processing SInput structs. This library should
// be useful if you want to simulate dispatching events to the
// Sketchology engine.

#ifndef INK_ENGINE_INPUT_SINPUT_HELPERS_H_
#define INK_ENGINE_INPUT_SINPUT_HELPERS_H_

#include <vector>

#include "ink/engine/camera/camera.h"
#include "ink/engine/input/sinput.h"
#include "ink/engine/public/types/input.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/security.h"
#include "ink/proto/sengine_portable_proto.pb.h"

namespace ink {
namespace input {
// Iterates through events in playback_stream, dispatching events to
// their corresponding callbacks. Callbacks may be nullptr.
//
// If any events or settings in playback_stream are found to be
// invalid, event processing will terminate early.
//
// All callbacks should return true to indicate that processing should
// continue, or false to exit event processing early.
//
// Returns true iff the entirety of playback_stream could be processed
// without errors. Errors arise from playback_stream having contents
// that are inconsistent or invalid. It is not an error if a callback
// returns false. Events in playback_stream that do not have a
// recognized type will be ignored, to enable processing of newer
// event streams by older code.
//
// Callback parameters:
//   - CameraOnInputF: event index, Camera (the new camera settings)
//   - SInputF: event index, Camera (current camera settings), SInput
S_WARN_UNUSED_RESULT
bool ProcessPlaybackStream(
    const proto::PlaybackStream& playback_stream,
    std::function<bool(const size_t, const Camera&)> camera_on_input_fn,
    std::function<bool(const size_t, const Camera&, SInput)> sinput_fn);

// In the function definitions below, a "raw" InputData instance is
// one which hasn't yet been processed by SEngine::inputDispatch. Its
// n_down and last_* members will not be populated.
//
// In contrast, a "processed" InputData instance has been processed by
// SEngine::inputDispatch. When computing processed InputData, it is
// assumed that no other inputs are occurring at the same time as
// those provided.
//
// Converts a single input with respect to some camera.
InputData ConvertToRawInputData(const SInput& sinput, const Camera& cam);

// Consists of an InputData that has been pumped through SEngine event
// dispatch and an index that identifies the event in some underlying
// source of input events, like a proto::PlaybackStream or
// std::vector<proto::SInput>.
//
// This is needed because input streams can have invalid input events
// (which SEngine discards) or camera changes, which results in a
// mapping from input events to processed events which is injective
// but not surjective.
struct ProcessedInputData {
  ProcessedInputData(const size_t i, InputData d) : index(i), data(d) {}
  ProcessedInputData(const ProcessedInputData&) = default;
  ProcessedInputData(ProcessedInputData&&) = default;
  ProcessedInputData& operator=(const ProcessedInputData&) = default;
  ProcessedInputData& operator=(ProcessedInputData&&) = default;

  size_t index;
  InputData data;
};

// Converts a stream of inputs. Camera information is taken from
// fields in input_stream.
std::vector<ProcessedInputData> ConvertToProcessedInputData(
    const proto::SInputStream& input_stream);

// Converts a stream of inputs. Camera information is taken from
// fields and events in playback_stream.
std::vector<ProcessedInputData> ConvertToProcessedInputData(
    const proto::PlaybackStream& playback_stream);

// A slice of an input stream. Slices are defined by the range [start,
// end). Additional fields describing the slice are provided.
struct Slice {
  size_t start;   // Inclusive.
  size_t end;     // Exclusive.
  bool any_down;  // True iff any pen was down at any point during the slice.

  Slice() {}
  Slice(size_t start, size_t end, bool any_down);
  Slice(const Slice& other) = default;
  Slice(Slice&& other) = default;
  Slice& operator=(const Slice& other) = default;
  Slice& operator=(Slice&& other) = default;
  bool operator==(const Slice& other) const;
  bool operator!=(const Slice& other) const;
};

// Returns slices of input_stream which correspond to logically
// coherent chunks of input. Slices during which a pen was down
// correspond to input strokes. Returned ranges index into the "input"
// field of input_stream.
//
// It is safe to play these slices back into a Sketchology engine one
// by one (e.g., via SEngine::dispatchInput). These slices will be
// disjoint. They are returned in an order matching their occurrence
// in the stream.
std::vector<Slice> SliceBoundaries(const proto::SInputStream& input_stream);

std::vector<Slice> SliceBoundaries(
    const proto::PlaybackStream& playback_stream);

// Returns false iff input_stream has any invalid camera values.
bool ValidateCamera(const proto::SInputStream& input_stream);

// Appends all events from src onto dest. If dest already contains at
// least one event and the camera settings of its last event are
// different from the initial settings of src, then the initial camera
// settings of src are injected as a camera change event.
void AppendStream(const proto::PlaybackStream& src,
                  proto::PlaybackStream* dest);
}  // namespace input
}  // namespace ink

#endif  // INK_ENGINE_INPUT_SINPUT_HELPERS_H_
