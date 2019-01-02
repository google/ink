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

#ifndef INK_ENGINE_SCENE_GRAPH_ELEMENT_NOTIFIER_H_
#define INK_ENGINE_SCENE_GRAPH_ELEMENT_NOTIFIER_H_

#include <memory>
#include <unordered_map>
#include <vector>

#include "ink/engine/public/host/public_events.h"
#include "ink/engine/public/types/uuid.h"
#include "ink/engine/scene/data/common/processed_element.h"
#include "ink/engine/scene/data/common/serialized_element.h"
#include "ink/engine/scene/types/element_metadata.h"
#include "ink/engine/scene/types/source_details.h"

namespace ink {

// Controls what callbacks the listeners receive after each stroke is
// added, and provides information on how to serialize strokes for the host.
class ElementNotifier {
 public:
  explicit ElementNotifier(std::shared_ptr<IElementListener> element_listener);

  // Will deliver the callback specified by "serialized_element.callbackType" to
  // the IElementListeners.
  //
  // Invariant: serialized_element.serialize() must be called first.
  void OnElementsAdded(const std::vector<SerializedElement>& serialized_element,
                       UUID below_uuid, const SourceDetails& source);
  void OnElementsMutated(const std::vector<ElementMutationData>& mutation_data,
                         const SourceDetails& source);
  void OnElementsRemoved(const std::vector<UUID>& uuids,
                         const SourceDetails& source);
  void OnElementsReplaced(const std::vector<UUID>& elements_to_remove,
                          const std::vector<SerializedElement>& elements_to_add,
                          const std::vector<UUID>& elements_to_add_below,
                          const SourceDetails& source_details);

  // Get/Set the callback flags for all elements  "sourceDetails".
  CallbackFlags GetCallbackFlags(const SourceDetails& source_details) const;
  void SetCallbackFlags(const SourceDetails& source_details,
                        const CallbackFlags& callback_flags);

 private:
  struct SourceDetailsHasher {
    size_t operator()(const ink::SourceDetails& source_details) const {
      return std::hash<uint32_t>()(
                 static_cast<uint32_t>(source_details.origin)) +
             std::hash<uint32_t>()(source_details.host_source_details);
    }
  };

  std::shared_ptr<IElementListener> element_listener_;
  std::unordered_map<SourceDetails, CallbackFlags, SourceDetailsHasher>
      source_to_callback_flags_;
};

}  // namespace ink
#endif  // INK_ENGINE_SCENE_GRAPH_ELEMENT_NOTIFIER_H_
