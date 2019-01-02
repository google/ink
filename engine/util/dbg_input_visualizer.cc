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

#include "ink/engine/util/dbg_input_visualizer.h"

#include <atomic>

#include "ink/engine/geometry/mesh/vertex.h"
#include "ink/engine/input/input_dispatch.h"

namespace ink {

namespace {
const int kDbgInputId = 23;
}  // namespace

DbgInputVisualizer::DbgInputVisualizer(
    std::shared_ptr<input::InputDispatch> dispatch,
    std::shared_ptr<IDbgHelper> dbg_helper)
    : input::InputHandler(input::Priority::ObserveOnly),
      dbg_helper_(std::move(dbg_helper)) {
  RegisterForInput(dispatch);

#if !INK_DEBUG
  assert(false);
#endif
}

input::CaptureResult DbgInputVisualizer::OnInput(const input::InputData& data,
                                                 const Camera& camera) {
  if (data.Get(input::Flag::TDown)) {
    dbg_helper_->Remove(kDbgInputId);
  }

  if (data.Get(input::Flag::InContact)) {
    auto v_world = Vertex(data.world_pos);
    v_world.color = color_;
    auto v_world_last = v_world;
    v_world_last.position = data.last_world_pos;

    static const bool kDbgInputPts = false;
    static const bool kDbgInputLines = false;

    if (kDbgInputPts) {
      dbg_helper_->AddPoint(v_world, 4, kDbgInputId);
    }
    if (kDbgInputLines) {
      dbg_helper_->AddLine(v_world_last, v_world, 2, kDbgInputId);
    }
  }
  return input::CapResObserve;
}

}  // namespace ink
