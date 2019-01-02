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

// The interface of an object that can listen to changes in the current scene's
// page bounds.
#ifndef INK_ENGINE_PUBLIC_HOST_IPAGE_PROPERTIES_LISTENER_H_
#define INK_ENGINE_PUBLIC_HOST_IPAGE_PROPERTIES_LISTENER_H_

#include "ink/engine/scene/types/event_dispatch.h"
#include "ink/proto/document_portable_proto.pb.h"
#include "ink/proto/elements_portable_proto.pb.h"

namespace ink {
class IPagePropertiesListener : public EventListener<IPagePropertiesListener> {
 public:
  ~IPagePropertiesListener() override {}

  virtual void PageBoundsChanged(
      const proto::Rect& bounds,
      const proto::SourceDetails& source_details) = 0;

  virtual void BackgroundColorChanged(
      const proto::Color& color,
      const proto::SourceDetails& source_details) = 0;

  virtual void BackgroundImageChanged(
      const proto::BackgroundImageInfo& image,
      const proto::SourceDetails& source_details) = 0;

  virtual void BorderChanged(const proto::Border& border,
                             const proto::SourceDetails& source_details) = 0;

  virtual void GridChanged(const proto::GridInfo& grid_info,
                           const proto::SourceDetails& source_details) = 0;
};
}  // namespace ink

#endif  // INK_ENGINE_PUBLIC_HOST_IPAGE_PROPERTIES_LISTENER_H_
