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

#ifndef INK_ENGINE_PUBLIC_HOST_HOST_H_
#define INK_ENGINE_PUBLIC_HOST_HOST_H_

#include <sys/types.h>
#include <cstdint>
#include <string>
#include <vector>

#include "ink/engine/public/host/ihost.h"
#include "ink/engine/public/types/uuid.h"
#include "ink/proto/elements_portable_proto.pb.h"
#include "ink/proto/mutations_portable_proto.pb.h"
#include "ink/proto/scene_change_portable_proto.pb.h"
#include "ink/proto/sengine_portable_proto.pb.h"

namespace ink {

// No-op implementation of IHost. You must implement BindScreen().
//
// Host is 72.1% of the way through an API change. See doc for details.
// https://docs.google.com/document/d/1CtUPIdad7VyoXg5EKgNXReEWicfXqpIydaALSzz64Q8
class Host : public IHost {
 public:
  ~Host() override {}

  // IPlatform
  void RequestFrame() override {
    // no-op.... the default environment assumes it is always running
    // at 60fps (see below). A frame will happen... soon.
  }
  void SetTargetFPS(uint32_t fps) override {}
  uint32_t GetTargetFPS() const override { return 60; }
  void RequestImage(const std::string& uri) override {}
  std::string GetPlatformId() const override { return ""; }
  void SetCursor(const ink::proto::Cursor& cursor) override {}

  // The inherited Host behavior is to preload shaders during engine
  // construction. This is slow, but theoretically prevents jank when starting
  // to draw.
  bool ShouldPreloadShaders() const override { return true; }

  // Render the given text proto at the given bitmap dimensions.
  std::unique_ptr<ClientBitmap> RenderText(const proto::text::Text& text,
                                           int width_px,
                                           int height_px) override {
    return nullptr;
  }

  // You must implement BindScreen.
  void BindScreen() override = 0;

  // IEngineListener
  void ImageExportComplete(uint32_t width_px, uint32_t height_px,
                           const std::vector<uint8_t>& img_bytes,
                           uint64_t fingerprint) override {}
  void PdfSaveComplete(const std::string& pdf_bytes) override {}
  void ToolEvent(const proto::ToolEvent& tool_event) override {}
  void SequencePointReached(int32_t id) override {}
  void UndoRedoStateChanged(bool canUndo, bool canRedo) override {}
  void FlagChanged(const proto::Flag& flag, bool enabled) override {}
  void LoggingEventFired(
      const ::logs::proto::research::ink::InkEvent& event) override {}
  void CameraMovementStateChanged(bool is_moving) override {}
  void BlockingStateChanged(bool is_blocked) override {}

  // IElementListener
  void ElementsAdded(const proto::ElementBundleAdds& element_bundle_adds,
                     const proto::SourceDetails& source_details) override {}
  void ElementsRemoved(const proto::ElementIdList& removed_ids,
                       const proto::SourceDetails& source_details) override {}
  void ElementsReplaced(const proto::ElementBundleReplace& replace,
                        const proto::SourceDetails& source_details) override {}
  void ElementsTransformMutated(
      const proto::ElementTransformMutations& unsafe_mutations,
      const proto::SourceDetails& source_details) override {}
  void ElementsVisibilityMutated(
      const proto::ElementVisibilityMutations& unsafe_mutations,
      const proto::SourceDetails& source_details) override {}
  void ElementsOpacityMutated(
      const proto::ElementOpacityMutations& unsafe_mutations,
      const proto::SourceDetails& source_details) override {}
  void ElementsZOrderMutated(
      const proto::ElementZOrderMutations& unsafe_mutations,
      const proto::SourceDetails& source_details) override {}

  // IPagePropertiesListener
  void PageBoundsChanged(const proto::Rect& bounds,
                         const proto::SourceDetails& sourceDetails) override {}
  void BackgroundColorChanged(
      const proto::Color& color,
      const proto::SourceDetails& sourceDetails) override {}
  void BackgroundImageChanged(
      const proto::BackgroundImageInfo& image,
      const proto::SourceDetails& sourceDetails) override {}
  void BorderChanged(const proto::Border& border,
                     const proto::SourceDetails& sourceDetails) override {}
  void GridChanged(const proto::GridInfo& grid_info,
                   const proto::SourceDetails& source_details) override {}

  // IMutationListener
  void OnMutation(const proto::mutations::Mutation& unsafe_mutation) override {}

  // ISceneChangeListener
  void SceneChanged(
      const proto::scene_change::SceneChangeEvent& scene_change) override {}
};

}  // namespace ink
#endif  // INK_ENGINE_PUBLIC_HOST_HOST_H_
