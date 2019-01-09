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

#ifndef INK_ENGINE_PROCESSING_ELEMENT_CONVERTERS_SCENE_ELEMENT_ADDER_H_
#define INK_ENGINE_PROCESSING_ELEMENT_CONVERTERS_SCENE_ELEMENT_ADDER_H_

#include <memory>
#include <vector>

#include "ink/engine/processing/element_converters/element_converter.h"
#include "ink/engine/processing/runner/task_runner.h"
#include "ink/engine/public/types/uuid.h"
#include "ink/engine/scene/data/common/processed_element.h"
#include "ink/engine/scene/data/common/serialized_element.h"
#include "ink/engine/scene/graph/scene_graph.h"
#include "ink/engine/scene/graph/scene_graph_listener.h"
#include "ink/engine/scene/types/element_id.h"
#include "ink/engine/scene/types/source_details.h"

namespace ink {

// SceneElementAdder is a background task that creates a ProcessedElement and
// SerializedElement using the given IElementConverter.
//
// When the task is completed, the element is added to the scene graph.
class SceneElementAdder : public SceneGraphListener, public Task {
 public:
  SceneElementAdder(std::unique_ptr<IElementConverter> processor,
                    std::shared_ptr<SceneGraph> scene_graph,
                    const settings::Flags& flags,
                    const SourceDetails& source_details, const UUID& uuid,
                    const ElementId& below_element_with_id,
                    const GroupId& group);

  bool RequiresPreExecute() const override { return false; }
  void PreExecute() override {}
  void Execute() final;
  void OnPostExecute() override;

  void OnElementAdded(SceneGraph* graph, ElementId id) override {}
  void OnElementsRemoved(
      SceneGraph* graph,
      const std::vector<SceneGraphRemoval>& removed_elements) override;
  void OnElementsMutated(
      SceneGraph* graph,
      const std::vector<ElementMutationData>& mutation_data) override {}

 private:
  IElementConverter::ElementConverterOptions element_converter_options_;
  std::unique_ptr<IElementConverter> processor_;
  std::weak_ptr<SceneGraph> weak_scene_graph_;
  ElementId id_;
  GroupId group_;
  SceneGraph::ElementAdd element_to_add_;

  // main thread only
  bool was_cancelled_;
};

}  // namespace ink
#endif  // INK_ENGINE_PROCESSING_ELEMENT_CONVERTERS_SCENE_ELEMENT_ADDER_H_
