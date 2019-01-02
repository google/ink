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

#ifndef INK_ENGINE_PUBLIC_HOST_MOCK_ELEMENT_LISTENER_H_
#define INK_ENGINE_PUBLIC_HOST_MOCK_ELEMENT_LISTENER_H_

#include "testing/base/public/gmock.h"
#include "ink/engine/public/host/ielement_listener.h"

namespace ink {

class MockElementListener : public IElementListener {
 public:
  MOCK_METHOD2(ElementsAdded,
               void(const proto::ElementBundleAdds& elementBundleAdds,
                    const proto::SourceDetails& sourceDetails));
  MOCK_METHOD2(ElementsRemoved,
               void(const proto::ElementIdList& removedIds,
                    const proto::SourceDetails& sourceDetails));
  MOCK_METHOD2(ElementsReplaced,
               void(const proto::ElementBundleReplace& replace,
                    const proto::SourceDetails& source_details));
  MOCK_METHOD2(ElementsTransformMutated,
               void(const proto::ElementTransformMutations& mutations,
                    const proto::SourceDetails& source_details));
  MOCK_METHOD2(ElementsVisibilityMutated,
               void(const proto::ElementVisibilityMutations& mutations,
                    const proto::SourceDetails& source_details));
  MOCK_METHOD2(ElementsOpacityMutated,
               void(const proto::ElementOpacityMutations& mutations,
                    const proto::SourceDetails& source_details));
  MOCK_METHOD2(ElementsZOrderMutated,
               void(const proto::ElementZOrderMutations& mutations,
                    const proto::SourceDetails& source_details));
};

}  // namespace ink
#endif  // INK_ENGINE_PUBLIC_HOST_MOCK_ELEMENT_LISTENER_H_
