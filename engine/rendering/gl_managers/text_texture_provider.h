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

#ifndef INK_ENGINE_RENDERING_GL_MANAGERS_TEXT_TEXTURE_PROVIDER_H_
#define INK_ENGINE_RENDERING_GL_MANAGERS_TEXT_TEXTURE_PROVIDER_H_

#include "ink/engine/camera/camera.h"
#include "ink/engine/public/host/iplatform.h"
#include "ink/engine/public/types/itexture_request_handler.h"
#include "ink/engine/public/types/status.h"
#include "ink/engine/scene/graph/scene_graph.h"
#include "ink/engine/scene/types/text.h"
#include "ink/engine/service/dependencies.h"

namespace ink {

// Text box meshes are expected to be squares of size kTextBoxSize x
// kTextBoxSize. This is validated when rendering text and assumed
// when reading text from a proto.
static const int kTextBoxSize = 2047;

// TextTextureProvider owns the URIs and text metadata objects for text
// textures. It can request text rendering from the host when requested by
// TextureManager. It also listens for element mutations of text elements
// and requests updated textures when appropriate.
class TextTextureProvider : public ITextureProvider, SceneGraphListener {
 public:
  using SharedDeps =
      service::Dependencies<IPlatform, Camera, SceneGraph, GLResourceManager>;

  TextTextureProvider(std::shared_ptr<IPlatform> platform,
                      std::shared_ptr<Camera> camera,
                      std::shared_ptr<SceneGraph> scene_graph,
                      std::shared_ptr<GLResourceManager> gl_resource_manager);

  bool CanHandleTextureRequest(absl::string_view uri) const override;

  Status HandleTextureRequest(absl::string_view uri,
                              std::unique_ptr<ClientBitmap>* out,
                              proto::ImageInfo::AssetType* type) const override;

  // Call this method when the given text is about to be added to the scene
  // with the given UUID. Returns the texture URI that the mesh should use.
  // Height and width are needed in order to give the host a size for computing
  // the layout (e.g. wrapping text).
  std::string AddText(text::TextSpec text, UUID uuid, int width_screen,
                      int height_screen);

  // Update the text spec stored for the given UUID. Request an updated texture.
  void UpdateText(UUID uuid, text::TextSpec text);

  // Set out to the given URI's TextSpec. Return true if successful, false if
  // no text was found.
  bool GetTextSpec(absl::string_view uri, text::TextSpec* out) const;

  // Make a URI for text of the given UUID.
  static std::string MakeUri(UUID uuid) { return kUriPrefix + uuid; }

  void OnElementAdded(SceneGraph* graph, ElementId id) override {}
  void OnElementsRemoved(
      SceneGraph* graph,
      const std::vector<SceneGraphRemoval>& removed_elements) override {}

  void OnElementsMutated(
      SceneGraph* graph,
      const std::vector<ElementMutationData>& mutation_data) override;

  // TextTextureProvider is neither copyable nor movable.
  TextTextureProvider(const TextTextureProvider&) = delete;
  TextTextureProvider& operator=(const TextTextureProvider&) = delete;

  std::string ToString() const override { return "TextTextureProvider"; }

  // Use a dummy texture to make the given element invisible until UpdateText()
  // is called when editing is complete. Only one element can be edited at a
  // time. If another element is already being edited when this is called, the
  // second element will take precedence.
  void BeginEditing(const UUID& uuid);

  Status GetLayout(UUID uuid, proto::text::Layout* layout);

 private:
  static const char kUriPrefix[];

  // Update TextureManager with a new texture for the given UUID. Texture is
  // requested from the host or a dummy texture is used if this text is
  // currently being edited.
  void UpdateTexture(UUID uuid);
  UUID UuidFromUri(absl::string_view uri) const;
  // Return the width and height of the given UUID on screen.
  glm::ivec2 SizeScreen(UUID uuid,
                        std::shared_ptr<SceneGraph> scene_graph) const;

  std::shared_ptr<IPlatform> platform_;
  std::shared_ptr<Camera> camera_;
  std::weak_ptr<SceneGraph> scene_graph_;
  std::weak_ptr<TextureManager> texture_manager_;

  // UUID of the text element currently being edited by the client (shouldn't
  // be rendered by the engine).
  UUID currently_editing_ = kInvalidUUID;
  std::unordered_map<std::string, text::TextSpec> uri_to_text_;
};

}  // namespace ink

#endif  // INK_ENGINE_RENDERING_GL_MANAGERS_TEXT_TEXTURE_PROVIDER_H_
