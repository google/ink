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

#include "ink/engine/rendering/gl_managers/text_texture_provider.h"
#include "ink/engine/geometry/primitives/matrix_utils.h"

#include "third_party/absl/strings/substitute.h"

namespace ink {

const char TextTextureProvider::kUriPrefix[] = "text-new://";
const int kMaxLength = 2048;

TextTextureProvider::TextTextureProvider(
    std::shared_ptr<IPlatform> platform, std::shared_ptr<Camera> camera,
    std::shared_ptr<SceneGraph> scene_graph,
    std::shared_ptr<GLResourceManager> gl_resource_manager)
    : platform_(platform),
      camera_(camera),
      scene_graph_(scene_graph),
      texture_manager_(gl_resource_manager->texture_manager) {
  scene_graph->AddListener(this);
}

std::string TextTextureProvider::AddText(text::TextSpec text, UUID uuid,
                                         int width_screen, int height_screen) {
  std::string uri = MakeUri(uuid);
  uri_to_text_.emplace(uri, text);

  return uri;
}

void TextTextureProvider::UpdateText(UUID uuid, text::TextSpec text) {
  if (currently_editing_ == uuid) {
    // We were editing this text before, now the update ends the editing
    // session.
    currently_editing_ = kInvalidUUID;
  }
  std::string uri = MakeUri(uuid);
  uri_to_text_[uri] = text;
  UpdateTexture(uuid);
}

bool TextTextureProvider::GetTextSpec(absl::string_view uri,
                                      text::TextSpec* out) const {
  std::string key(uri);
  auto it = uri_to_text_.find(key);
  if (it == uri_to_text_.end()) {
    return false;
  }
  *out = it->second;
  return true;
}

bool TextTextureProvider::CanHandleTextureRequest(absl::string_view uri) const {
  std::string key(uri);
  return uri_to_text_.count(key) > 0;
}

Status TextTextureProvider::HandleTextureRequest(
    absl::string_view uri, std::unique_ptr<ClientBitmap>* out,
    proto::ImageInfo::AssetType* asset_type) const {
  text::TextSpec text;
  if (!GetTextSpec(uri, &text)) {
    return ErrorStatus("could not render unknown text $0", uri);
  }
  proto::text::Text text_proto;
  text::TextSpec::WriteToProto(&text_proto, text);

  UUID uuid = UuidFromUri(uri);
  if (currently_editing_ == uuid) {
    // Client is currently editing this text, put a dummy texture in until
    // editing is complete.
    *out = absl::make_unique<RawClientBitmap>(
        ImageSize(1, 1), ImageFormat::BITMAP_FORMAT_RGBA_8888);
    return OkStatus();
  }
  if (auto scene_graph = scene_graph_.lock()) {
    ElementId id = scene_graph->ElementIdFromUUID(uuid);

    if (id == kInvalidElementId) {
      return ErrorStatus("Can't render text for invalid URI $0", uri);
    }

    glm::vec2 screen_size = SizeScreen(uuid, scene_graph);
    screen_size = util::ScaleWithinMax(screen_size, kMaxLength);
    *out = platform_->RenderText(text_proto, std::round(screen_size.x),
                                 std::round(screen_size.y));
    *asset_type = proto::ImageInfo::DEFAULT;
  }
  return OkStatus();
}

void TextTextureProvider::OnElementsMutated(
    SceneGraph* graph, const std::vector<ElementMutationData>& mutation_data) {
  for (auto mutation : mutation_data) {
    if (mutation.mutation_type == ElementMutationType::kTransformMutation) {
      UpdateTexture(mutation.modified_element_data.uuid);
    }
  }
}

void TextTextureProvider::UpdateTexture(UUID uuid) {
  std::string uri = MakeUri(uuid);
  auto texture_manager = texture_manager_.lock();
  auto scene_graph = scene_graph_.lock();
  if (texture_manager && scene_graph) {
    if (currently_editing_ == uuid) {
      // Client is currently editing this text, put a dummy texture in until
      // editing is complete.
      RawClientBitmap r(ImageSize(1, 1), ImageFormat::BITMAP_FORMAT_RGBA_8888);
      texture_manager->GenerateTexture(uri, r);
      return;
    }
    text::TextSpec text;
    if (GetTextSpec(uri, &text)) {
      proto::text::Text text_proto;
      text::TextSpec::WriteToProto(&text_proto, text);

      auto size_screen = SizeScreen(uuid, scene_graph);

      std::unique_ptr<ClientBitmap> bitmap =
          platform_->RenderText(text_proto, size_screen.x, size_screen.y);
      if (bitmap) {
        texture_manager->GenerateTexture(uri, *bitmap);
      }
    }
  }
}

UUID TextTextureProvider::UuidFromUri(absl::string_view uri) const {
  if (CanHandleTextureRequest(uri)) {
    return UUID(uri.substr(strlen(kUriPrefix)));
  }
  return kInvalidUUID;
}

void TextTextureProvider::BeginEditing(const UUID& uuid) {
  currently_editing_ = uuid;
  UpdateTexture(uuid);
}

glm::ivec2 TextTextureProvider::SizeScreen(
    UUID uuid, std::shared_ptr<SceneGraph> scene_graph) const {
  ElementId id = scene_graph->ElementIdFromUUID(uuid);

  ElementMetadata metadata = scene_graph->GetElementMetadata(id);

  // Width and height of the text box in world coords
  glm::vec2 world_size =
      matrix_utils::GetScaleComponent(metadata.world_transform) *
      scene_graph->MbrObjCoords(id).Dim();
  glm::vec2 screen_size =
      camera_->ConvertVector(world_size, CoordType::kWorld, CoordType::kScreen);

  screen_size = util::ScaleWithinMax(screen_size, kMaxLength);
  return glm::ivec2(std::round(screen_size.x), std::round(screen_size.y));
}

}  // namespace ink
