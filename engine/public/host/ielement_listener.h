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

#ifndef INK_ENGINE_PUBLIC_HOST_IELEMENT_LISTENER_H_
#define INK_ENGINE_PUBLIC_HOST_IELEMENT_LISTENER_H_

#include "ink/engine/public/types/uuid.h"
#include "ink/engine/scene/types/event_dispatch.h"
#include "ink/proto/elements_portable_proto.pb.h"

namespace ink {
class IElementListener : public EventListener<IElementListener> {
 public:
  ~IElementListener() override {}

  // Called when an Element was locally created (e.g. when you lift a finger
  // when drawing a stroke).
  // NOTE: Beware of unsafe/malicious protos.
  virtual void ElementsAdded(const proto::ElementBundleAdds& unsafe_adds,
                             const proto::SourceDetails& sourceDetails) = 0;
  virtual void ElementsRemoved(const proto::ElementIdList& removedIds,
                               const proto::SourceDetails& sourceDetails) = 0;
  virtual void ElementsReplaced(
      const proto::ElementBundleReplace& unsafe_replace,
      const proto::SourceDetails& source_details) = 0;
  virtual void ElementsTransformMutated(
      const proto::ElementTransformMutations& unsafe_mutations,
      const proto::SourceDetails& source_details) = 0;
  virtual void ElementsVisibilityMutated(
      const proto::ElementVisibilityMutations& unsafe_mutations,
      const proto::SourceDetails& source_details) = 0;
  virtual void ElementsOpacityMutated(
      const proto::ElementOpacityMutations& unsafe_mutations,
      const proto::SourceDetails& source_details) = 0;
  virtual void ElementsZOrderMutated(
      const proto::ElementZOrderMutations& unsafe_mutations,
      const proto::SourceDetails& source_details) = 0;
};
}  // namespace ink

#endif  // INK_ENGINE_PUBLIC_HOST_IELEMENT_LISTENER_H_
