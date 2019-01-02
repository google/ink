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

#include "ink/engine/processing/element_converters/scene_element_adder.h"

#include <algorithm>
#include <memory>

#include "third_party/absl/memory/memory.h"
#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/public/types/uuid.h"
#include "ink/engine/scene/frame_state/frame_state.h"
#include "ink/engine/scene/graph/element_notifier.h"
#include "ink/engine/scene/graph/scene_graph.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"

namespace ink {

SceneElementAdder::SceneElementAdder(
    std::unique_ptr<IElementConverter> processor,
    std::shared_ptr<SceneGraph> scene_graph,
    const SourceDetails& source_details, const UUID& uuid,
    const ElementId& below_element_with_id, const GroupId& group)
    : Task(),
      processor_(std::move(processor)),
      weak_scene_graph_(scene_graph),
      id_(kInvalidElementId),
      group_(group),
      was_cancelled_(false) {
  SLOG(SLOG_OBJ_LIFETIME, "polyprocessor ctor");

  UUID group_uuid = kInvalidUUID;
  if (group_ != kInvalidElementId) {
    group_uuid = scene_graph->UUIDFromElementId(group_);
    // If we're here the group UUID should exist. If this fails, it will look
    // like a failure on the load path after the element is saved, as the
    // element will be a root element.
    ASSERT(group_uuid != kInvalidUUID);
  }

  element_to_add_.id_to_add_below = below_element_with_id;
  if (!scene_graph->GetNextPolyId(uuid, &id_)) {
    // Give up if we can't get an id (e.g. the requested mapping is bad)
    was_cancelled_ = true;
  } else {
    element_to_add_.serialized_element = absl::make_unique<SerializedElement>(
        uuid, group_uuid, source_details,
        scene_graph->GetElementNotifier()->GetCallbackFlags(source_details));
    if (!scene_graph->IsBulkLoading()) {
      // No removals can happen during bulk loading.
      scene_graph->AddListener(this);
    }
  }
}

void SceneElementAdder::Execute() {
  if (!element_to_add_.serialized_element) return;

  element_to_add_.processed_element = processor_->CreateProcessedElement(id_);

  // The brix processor may return null from CreateProcessedElement if
  // deserialization fails.
  if (element_to_add_.processed_element != nullptr) {
    element_to_add_.processed_element->group = group_;
    element_to_add_.serialized_element->Serialize(
        *element_to_add_.processed_element);
  }
}

void SceneElementAdder::OnPostExecute() {
  if (was_cancelled_) return;
  if (element_to_add_.processed_element == nullptr) {
    SLOG(SLOG_ERROR, "Encountered null line: ignoring");
    return;
  }

  if (auto scene_graph = weak_scene_graph_.lock())
    scene_graph->AddStroke(std::move(element_to_add_));
}

void SceneElementAdder::OnElementsRemoved(
    SceneGraph* graph, const std::vector<SceneGraphRemoval>& removed_elements) {
  ASSERT(!graph->IsBulkLoading());
  auto id_equal = [=](const SceneGraphRemoval& r) { return r.id == id_; };
  if (std::any_of(removed_elements.begin(), removed_elements.end(), id_equal)) {
    was_cancelled_ = true;
    if (auto scene_graph = weak_scene_graph_.lock()) {
      scene_graph->RemoveListener(this);
    }
  }
}

}  // namespace ink
