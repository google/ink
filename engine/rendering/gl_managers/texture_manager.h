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

#ifndef INK_ENGINE_RENDERING_GL_MANAGERS_TEXTURE_MANAGER_H_
#define INK_ENGINE_RENDERING_GL_MANAGERS_TEXTURE_MANAGER_H_

#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "geo/render/ion/gfx/graphicsmanager.h"
#include "third_party/absl/strings/substitute.h"
#include "third_party/absl/synchronization/mutex.h"
#include "ink/engine/processing/runner/task_runner.h"
#include "ink/engine/public/host/iplatform.h"
#include "ink/engine/public/types/client_bitmap.h"
#include "ink/engine/public/types/itexture_request_handler.h"
#include "ink/engine/rendering/gl_managers/client_bitmap_pool.h"
#include "ink/engine/rendering/gl_managers/texture.h"
#include "ink/engine/rendering/gl_managers/texture_info.h"
#include "ink/engine/rendering/gl_managers/texture_params.h"
#include "ink/engine/scene/frame_state/frame_state.h"
#include "ink/engine/scene/types/event_dispatch.h"
#include "ink/engine/util/time/logging_perf_timer.h"
#include "ink/engine/util/time/wall_clock.h"

namespace ink {

/**
 * The TextureManager owns one of these, the single point of truth for
 * everything in the engine that participates in tiled rendering.
 *
 * Reasonable default vaues are provided.
 */
struct TilePolicy {
  // Every tile shares this format.
  ImageFormat image_format = ImageFormat::BITMAP_FORMAT_RGB_888;

  // Side dimension of a tile in texels.
  size_t tile_side_length = 1024;

  // This is a hint to the texture manager, a "best effort" target for maximum
  // GPU RAM allocated for tile textures. It's possible to go momentarily beyond
  // this target because (1) we load textures into GPU before we evict stale
  // ones, and (2) we don't evict any tiles required to display what's currently
  // visible.
  size_t max_tile_ram = 1 << 27;  // 128MB

  // Size, in tiles, of tile bitmap pool.
  size_t bitmap_pool_size = 3;

  // Maximum number of tiles to fetch per frame. 0 = unlimited.
  // This default value seems to give good performance on linux, web, and
  // Android. iOS has not yet been tried.
  size_t max_tiles_fetched_per_frame = 1;

  // If true, draw a blue outline around each tile.
  bool debug_tiles = false;

  inline size_t BytesPerTile() const {
    return tile_side_length * tile_side_length *
           bytesPerTexelForFormat(image_format);
  }

  TilePolicy() {}
  inline TilePolicy(const TilePolicy& s) = default;
  std::string ToString() const {
    return Substitute(
        "tile_side_length:$0 max_tile_ram:$1 bitmap_pool_size:$2 "
        "max_tiles_fetched_per_frame:$3 image_format: $4",
        tile_side_length, max_tile_ram, bitmap_pool_size,
        max_tiles_fetched_per_frame, image_format);
  }
};

class TextureListener : public EventListener<TextureListener> {
 public:
  virtual void OnTextureLoaded(const TextureInfo& info) = 0;
  virtual void OnTextureEvicted(const TextureInfo& info) = 0;
};

// Manages textures that are cached on the gpu.  Textures are accessed via
// TextureInfo, which must contain the uri identifying the texture.  Requests
// that the host send missing textures.
//
// URIs of the form "sketchology://.*" are reserved for internal use.
// Current users:
//   * ImageBackground: URIs of the form "sketchology://background_[0-9]*"
//   * Text: URIs of the form "text://..."
class TextureManager : public FrameStateListener,
                       public std::enable_shared_from_this<TextureManager> {
 public:
  TextureManager(ion::gfx::GraphicsManagerPtr gl,
                 std::shared_ptr<IPlatform> platform,
                 std::shared_ptr<FrameState> frame_state,
                 std::shared_ptr<ITaskRunner> task_runner);
  ~TextureManager() override {}

  // Generates a Texture for "clientImage" that can be retrieved through
  // the returned TextureInfo, or another TextureInfo with the same uri.
  TextureInfo GenerateTexture(const std::string& uri,
                              const ClientBitmap& client_bitmap,
                              TextureParams params = TextureParams());

  // Generates a 1x1 transparent texture for the given rejected texture URI.
  TextureInfo GenerateRejectedTexture(const std::string& uri);

  // Attempts to bind texture specified by "textureInfo" if in cache,
  // otherwise requests texture from host.  Return indicates if binding
  // succeeded.
  bool Bind(const TextureInfo& texture_info, GLuint to_location);

  bool Bind(const TextureInfo& texture_info) {
    return Bind(texture_info, GL_TEXTURE0);
  }

  // Removes the texture mapped to from "textureInfo" from the cache.
  // Also removes the given texture from pending fetches.
  void Evict(const TextureInfo& texture_info);

  // Removes all textures from GPU and cache.
  void EvictAll();

  // Returns if texture is cached.  Sets result in "texture".  If texture
  // is not in cache, requests it from host.
  bool GetTexture(const TextureInfo& texture_info, Texture** texture);

  // Whether we have image data for this Texture.
  bool HasTexture(const TextureInfo& texture_info) const;

  // Indicates whether this URI has been requested but not yet generated.
  // WARNING: This must not be used to determine if it is safe to access/modify
  // uris_to_request_ or requested_uris_ -- you must instead acquire a
  // mutex lock, call IsLoadingInternal(), perform your operations, and only
  // then release the mutex lock.
  bool IsLoading(const std::string& uri) const
      LOCKS_EXCLUDED(requested_uris_mutex_);

  void OnFrameEnd() override;

  void AddListener(TextureListener* listener) {
    listener->RegisterOnDispatch(dispatch_);
  }
  void RemoveListener(TextureListener* listener) {
    listener->Unregister(dispatch_);
  }

  /**
   * Insert a texture provider into the texture manager's chain of handlers, if
   * any. The given ID can be used to remove the given handler as needed. The
   * texture manager takes ownership of the given handler. If a handler with
   * the given ID already exists, it is replaced and destroyed.
   * @param handler_id An arbitrary non-empty string used to later identify the
   * given texture provider for removal.
   * @param handler The texture request handler to add to this texture manager's
   * chain of handlers.
   */
  void AddTextureRequestHandler(
      const std::string& handler_id,
      std::shared_ptr<ITextureRequestHandler> handler);

  /**
   * Gets the texture handler identified by the given id. If no such id is
   * found, returns nullptr.
   * @param handler_id An arbitrary non-empty string associated with a texture
   * handler added via AddTextureRequestHandler.
   */
  ITextureRequestHandler* GetTextureRequestHandler(
      const std::string& handler_id) const;

  /**
   * Removes the texture handler identified by the given id. If no such id is
   * found, nothing happens.
   * @param handler_id An arbitrary non-empty string associated with a texture
   * handler added via AddTextureRequestHandler.
   */
  void RemoveTextureRequestHandler(const std::string& handler_id);

  TilePolicy GetTilePolicy() const { return TilePolicy(tile_policy_); }

  void SetTilePolicy(const TilePolicy& new_policy);

  // Notifies the host to send texture data for uri, if it hasn't already been
  // loaded or requested. Returns true if the texture was actually requested.
  // It is the host's responsibility to (potentially asynchronously) call
  // "generateTexture". While waiting for a call to "generateTexture" on "uri",
  // will not send host additional requests for the same uri.
  bool MaybeStartClientImageRequest(absl::string_view uri)
      LOCKS_EXCLUDED(requested_uris_mutex_);

 private:
  // Indicates whether this URI has been requested but not yet generated.
  bool IsLoadingInternal(const std::string& uri) const
      EXCLUSIVE_LOCKS_REQUIRED(requested_uris_mutex_);

  // Returns true if texture is cached.  Sets result in "texture".
  bool GetTextureImpl(const TextureInfo& texture_info, Texture** texture);

  // Updates textureInfo.textureId and returns true if the texture is present.
  bool EnsureTextureId(const TextureInfo& texture_info);

  /**
   * Tries each registered ITextureProvider with the given uri. If any succeeds,
   * a task to fetch the texture associated with the given URI has been
   * enqueued, and TextureFetchInitiated returns true; otherwise it returns
   * false.
   */
  bool TextureFetchInitiated(const std::string& uri);

  // Returns a ClientBitmap suitable for rendering a tile conforming to the
  // current TilePolicy.
  std::unique_ptr<ClientBitmap> GetTileBitmap() const;

  size_t CurrentTileRamUsage() const;

  /**
   * If we are over the RAM limit for cached tiles, evict as necessary.
   */
  void EvictStaleTiles();

  // Removes the given uri from uris_to_request_ and requested_uris_.
  void ClearInflightRequest(absl::string_view uri);

  // If the given URI is evictable, marks it as "fresh", i.e., not to be evicted
  // within the window of frames specified my the tile cache policy.
  void MarkAsFresh(absl::string_view uri);

  // For mocking
  virtual std::unique_ptr<Texture> CreateTexture();

  TextureId next_id_;
  ion::gfx::GraphicsManagerPtr gl_;
  std::shared_ptr<IPlatform> platform_;
  std::shared_ptr<FrameState> frame_state_;
  std::shared_ptr<ITaskRunner> task_runner_;
  std::shared_ptr<EventDispatch<TextureListener>> dispatch_;
  TilePolicy tile_policy_;

  // Implementation notes:
  //
  // Keeps track of cached textures with the mapping uri -> id -> Texture*
  //
  // Invariant: values of "uriToId_" are unique and in one to one
  // correspondence with keys of "cachedTextures_".
  //
  // The ids are generated by the TextureManager and are guaranteed to be
  // unique. They do NOT correspond to openGL texture ids (which are stored in
  // Texture).
  std::unordered_map<std::string, TextureId> uri_to_id_;
  std::unordered_map<TextureId, std::unique_ptr<Texture>> id_to_texture_;

  // Guards the structures that keep track of requested and in-flight uris.
  mutable absl::Mutex requested_uris_mutex_;

  // URIs that will be requested at the end of the next frame.
  std::set<std::string> uris_to_request_ GUARDED_BY(requested_uris_mutex_);

  // URIs that have been requested but not yet arrived.
  std::set<std::string> requested_uris_ GUARDED_BY(requested_uris_mutex_);

  using ProviderPacket =
      std::pair<std::string, std::shared_ptr<ITextureRequestHandler>>;
  std::vector<ProviderPacket> texture_handlers_;

  // We keep track of all loaded tile uris, in order to consider them for
  // auto-eviction.
  std::set<std::string> tile_texture_uris_;

  // We track all tiles requested during each frame so we can cancel stale
  // in-flight requests and apply a distance metric to eviction candidates.
  std::set<std::string> frame_tile_requests_;

  // Lazily constructed when a tile is first requested.
  mutable std::unique_ptr<ClientBitmapPool> bitmap_pool_;

  std::unique_ptr<LoggingPerfTimer> fetch_timer_;
  std::unique_ptr<LoggingPerfTimer> generate_texture_timer_;

  friend class TextureFetchTask;
};

}  // namespace ink

#endif  // INK_ENGINE_RENDERING_GL_MANAGERS_TEXTURE_MANAGER_H_
