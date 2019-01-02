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

#ifndef INK_ENGINE_RENDERING_GL_MANAGERS_TEXTURE_INFO_H_
#define INK_ENGINE_RENDERING_GL_MANAGERS_TEXTURE_INFO_H_

#include <cstdint>
#include <string>

#include "third_party/absl/strings/string_view.h"

namespace ink {

typedef uint32_t TextureId;

// Provides a reference to a texture by uri that can be used to fetch the
// texture with the TextureManager.
struct TextureInfo {
  // An arbitrary string that specifies the resource or process required to
  // provide a bitmap. For example, it could be the URI of a bundled Android
  // resource, a specification for a dynamically-generated grid, or a PDF page
  // number.
  std::string uri;

  // A cache for an id assigned by TextureManager (which maps uri -> id ->
  // texture).  May hold kBadTextureId when id for uri is unknown. Is auto
  // updated by "TextureManager::getTexture()".
  mutable TextureId texture_id;

  void Reset(absl::string_view new_uri) {
    uri = std::string(new_uri);
    texture_id = kBadTextureId;
  }

  // The texture for this mesh is at "uri"
  explicit TextureInfo(absl::string_view uri)
      : TextureInfo(uri, kBadTextureId) {}

  // The texture for this mesh is at "uri" and TextureManager id is known.
  TextureInfo(absl::string_view uri, TextureId existing_texture_id)
      : uri(uri), texture_id(existing_texture_id) {}

  static const TextureId kBadTextureId = static_cast<TextureId>(-1);
};

}  // namespace ink
#endif  // INK_ENGINE_RENDERING_GL_MANAGERS_TEXTURE_INFO_H_
