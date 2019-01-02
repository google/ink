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

#ifndef INK_ENGINE_SCENE_GRID_ASSETS_GRID_TEXTURE_PROVIDER_H_
#define INK_ENGINE_SCENE_GRID_ASSETS_GRID_TEXTURE_PROVIDER_H_

#include "third_party/absl/strings/string_view.h"
#include "ink/engine/public/types/client_bitmap.h"
#include "ink/engine/public/types/itexture_request_handler.h"
#include "ink/engine/public/types/status.h"
#include "ink/proto/sengine_portable_proto.pb.h"

namespace ink {

// Handles texture requests for URIs of the form sketchology://grid_.* that
// match PNG assets in the same directory that have been processed with
// generate-grid-assets.sh.  This is used to provide consistent cross-platform
// grid images for common grid types (e.g. rules, squares, and dots).
//
// Custom grid images can be added to the engine via SEngine::addImageData and
// referenced by the given URI.
//
// If image data is added for a URI that this provider would handle, that image
// data is used instead.
class GridTextureProvider : public ITextureProvider {
 public:
  bool CanHandleTextureRequest(absl::string_view uri) const override;
  Status HandleTextureRequest(
      absl::string_view uri, std::unique_ptr<ClientBitmap>* out,
      proto::ImageInfo::AssetType* asset_type) const override;

  std::string ToString() const override { return "GridTextureProvider"; };
};

}  // namespace ink

#endif  // INK_ENGINE_SCENE_GRID_ASSETS_GRID_TEXTURE_PROVIDER_H_
