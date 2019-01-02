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

#ifndef INK_ENGINE_PUBLIC_HOST_MOCK_PAGE_PROPERTIES_LISTENER_H_
#define INK_ENGINE_PUBLIC_HOST_MOCK_PAGE_PROPERTIES_LISTENER_H_

#include "testing/base/public/gmock.h"
#include "ink/engine/public/host/ipage_properties_listener.h"
#include "ink/proto/document_portable_proto.pb.h"
#include "ink/proto/elements_portable_proto.pb.h"

namespace ink {
class MockPagePropertiesListener : public IPagePropertiesListener {
 public:
  MOCK_METHOD2(PageBoundsChanged,
               void(const proto::Rect& bounds,
                    const proto::SourceDetails& sourceDetails));

  MOCK_METHOD2(BackgroundColorChanged,
               void(const proto::Color& color,
                    const proto::SourceDetails& sourceDetails));

  MOCK_METHOD2(BackgroundImageChanged,
               void(const proto::BackgroundImageInfo& image,
                    const proto::SourceDetails& sourceDetails));

  MOCK_METHOD2(BorderChanged, void(const proto::Border& bounds,
                                   const proto::SourceDetails& sourceDetails));

  MOCK_METHOD2(GridChanged, void(const proto::GridInfo& grid_info,
                                 const proto::SourceDetails& source_details));
};

}  // namespace ink

#endif  // INK_ENGINE_PUBLIC_HOST_MOCK_PAGE_PROPERTIES_LISTENER_H_
