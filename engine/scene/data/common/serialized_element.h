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

#ifndef INK_ENGINE_SCENE_DATA_COMMON_SERIALIZED_ELEMENT_H_
#define INK_ENGINE_SCENE_DATA_COMMON_SERIALIZED_ELEMENT_H_

#include <memory>

#include "ink/engine/public/types/uuid.h"
#include "ink/engine/scene/data/common/processed_element.h"
#include "ink/engine/scene/types/element_id.h"
#include "ink/engine/scene/types/source_details.h"
#include "ink/engine/util/proto/serialize.h"
#include "ink/proto/elements_portable_proto.pb.h"

namespace ink {

// Holds serialized version of a scene element.  Data serialized is determined
// by what callback the host requires.
struct SerializedElement {
  UUID uuid;
  UUID parent_uuid;

  // Stores the result of serialization (calling serialize()).
  std::unique_ptr<proto::ElementBundle> bundle;

  SourceDetails source_details;

  // Determines what serialization (if any) is necessary.
  CallbackFlags callback_flags;

 public:
  SerializedElement(const UUID& uuid, const UUID& parent_uuid,
                    SourceDetails source_details, CallbackFlags callback_flags);

  // Sets "serializedData" to contain needed data for giving host
  // "callbackType".
  void Serialize(const ProcessedElement& processed_element);
};

}  // namespace ink
#endif  // INK_ENGINE_SCENE_DATA_COMMON_SERIALIZED_ELEMENT_H_
