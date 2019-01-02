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

#ifndef INK_ENGINE_PUBLIC_ITEXTURE_PROVIDER_H_
#define INK_ENGINE_PUBLIC_ITEXTURE_PROVIDER_H_

#include <memory>
#include <string>

#include "third_party/absl/strings/string_view.h"
#include "ink/engine/public/types/client_bitmap.h"
#include "ink/engine/public/types/status.h"
#include "ink/proto/sengine_portable_proto.pb.h"

namespace ink {

/**
 * An ITextureProvider can intercept a texture request before it gets sent to
 * the host.
 */
class ITextureRequestHandler {
 public:
  virtual ~ITextureRequestHandler() {}

  /**
   * Examines the given uri, and determines whether it can be fulfilled by this
   * provider.
   * @param uri The texture uri to inspect and possibly fulfill.
   * @return true if this provider can handle the texture request for a given
   * uri.
   */
  virtual bool CanHandleTextureRequest(absl::string_view uri) const = 0;

  virtual std::string ToString() const = 0;
};

class ITextureProvider : public ITextureRequestHandler {
 public:
  /**
   * Examines the given uri, determines whether it can be fulfilled by this
   * provider, and returns true if so or false otherwise. If true, the given
   * ClientBitmap is populated.
   * @param uri The texture uri to inspect and possibly fulfill.
   * @param out The ClientBitmap to populate on successful requests.
   * @return ok if this provider handled the texture request, error otherwise.
   */
  virtual Status HandleTextureRequest(
      absl::string_view uri, std::unique_ptr<ClientBitmap>* out,
      proto::ImageInfo::AssetType* asset_type) const = 0;
};

/**
 * An ITileProvider renders to a provided bitmap, whose dimensions and format
 * are managed by the texture manager.
 */
class ITileProvider : public ITextureRequestHandler {
 public:
  /**
   * Examines the given uri, determines whether it can be fulfilled by this
   * provider, and returns true if so or false otherwise. If true, the given
   * ClientBitmap is rendered into.
   * @param uri The texture uri to inspect and possibly fulfill.
   * @param out The ClientBitmap to render into on successful requests.
   * @return ok if this provider handled the texture request, error otherwise.
   */
  virtual Status HandleTileRequest(absl::string_view uri,
                                   ClientBitmap* out) const = 0;
};
}  // namespace ink

#endif  // INK_ENGINE_PUBLIC_ITEXTURE_PROVIDER_H_
