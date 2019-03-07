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

#include "ink/engine/rendering/gl_managers/texture_manager.h"

// NaCL is missing htonl
#ifdef __native_client__
#include <stdint.h>
uint32_t htonl(uint32_t hostlong) { return __builtin_bswap32(hostlong); }
#else
#include <arpa/inet.h>  // htonl
#endif

#include <algorithm>
#include <random>

#include "third_party/absl/algorithm/container.h"
#include "third_party/absl/memory/memory.h"
#include "third_party/absl/strings/numbers.h"
#include "third_party/absl/strings/substitute.h"
#include "third_party/absl/synchronization/mutex.h"
#include "ink/engine/rendering/gl_managers/bad_gl_handle.h"
#include "ink/engine/rendering/gl_managers/nine_patch_info.h"
#include "ink/engine/rendering/gl_managers/texture_params.h"
#include "ink/engine/rendering/page_tile_spec.h"
#include "ink/engine/rendering/zoom_spec.h"
#include "ink/engine/scene/types/event_dispatch.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"

namespace ink {

using std::unique_ptr;

namespace {
bool IsEvictableUri(absl::string_view uri) {
  // It's evictable if it is a tile, i.e., if it has a zoom parameter.
  return ZoomSpec::HasZoomSpecParam(uri);
}
}  // namespace

TextureManager::TextureManager(ion::gfx::GraphicsManagerPtr gl,
                               std::shared_ptr<IPlatform> platform,
                               std::shared_ptr<FrameState> frame_state,
                               std::shared_ptr<ITaskRunner> task_runner)
    : next_id_(1),
      gl_(gl),
      platform_(std::move(platform)),
      frame_state_(std::move(frame_state)),
      task_runner_(std::move(task_runner)),
      dispatch_(new EventDispatch<TextureListener>()) {
  frame_state_->AddListener(this);
  auto clock = std::make_shared<WallClock>();
  fetch_timer_ = absl::make_unique<LoggingPerfTimer>(clock, "Fetch Texture");
  generate_texture_timer_ =
      absl::make_unique<LoggingPerfTimer>(clock, "Generate Texture");
}

TextureInfo TextureManager::GenerateTexture(const std::string& uri,
                                            const ClientBitmap& client_bitmap,
                                            TextureParams params) {
  TextureId id;
  if (uri_to_id_.count(uri) > 0) {
    id = uri_to_id_[uri];
    // erase the old texture
    id_to_texture_.erase(id);
  } else {
    id = next_id_++;
  }

  ClearInflightRequest(uri);

  unique_ptr<Texture> tex = CreateTexture();
  tex->Load(client_bitmap, params);
  id_to_texture_[id] = std::move(tex);
  uri_to_id_[uri] = id;

  if (IsEvictableUri(uri)) {
    tile_texture_uris_.insert(uri);
  }

  // A texture has arrived; let's make sure it gets drawn.
  frame_state_->RequestFrame();

  TextureInfo info(uri, id);
  dispatch_->Send(&TextureListener::OnTextureLoaded, info);
  return info;
}

TextureInfo TextureManager::GenerateRejectedTexture(const std::string& uri) {
  RawClientBitmap bitmap({0}, ImageSize(1, 1), ImageFormat::BITMAP_FORMAT_A_8);
  TextureParams params;
  params.is_rejection = true;
  return GenerateTexture(uri, bitmap, params);
}

unique_ptr<Texture> TextureManager::CreateTexture() {
  return absl::make_unique<Texture>(gl_);
}

bool TextureManager::GetTextureImpl(const TextureInfo& texture_info,
                                    Texture** texture) {
  MarkAsFresh(texture_info.uri);
  if (EnsureTextureId(texture_info)) {
    ASSERT(id_to_texture_.count(texture_info.texture_id) > 0);
    *texture = id_to_texture_[texture_info.texture_id].get();
    return true;
  }
  return false;
}

void TextureManager::MarkAsFresh(absl::string_view uri) {
  if (IsEvictableUri(uri)) {
    frame_tile_requests_.insert(static_cast<std::string>(uri));
  }
}

bool TextureManager::GetTexture(const TextureInfo& texture_info,
                                Texture** texture) {
  bool success = GetTextureImpl(texture_info, texture);
  if (!success) MaybeStartClientImageRequest(texture_info.uri);
  return success;
}

bool TextureManager::Bind(const TextureInfo& texture_info, GLuint to_location) {
  Texture* texture = nullptr;
  bool success = GetTexture(texture_info, &texture);
  if (success) {
    SLOG(SLOG_GL_STATE,
         "Binding texture uri: $0, TextureId: $1, At location: $2, GL Texture "
         "Handle $3",
         texture_info.uri, texture_info.texture_id, to_location,
         texture->TextureId());
    texture->Bind(to_location);
  } else {
    SLOG(SLOG_DRAWING, "Texture $0 isn't ready yet.", texture_info.uri);
  }
  return success;
}

bool TextureManager::EnsureTextureId(const TextureInfo& texture_info) {
  // From mapping uri -> id -> texture, attempt lookup by id.  If this fails,
  // attempt lookup by uri.
  if (id_to_texture_.count(texture_info.texture_id) == 0) {
    if (uri_to_id_.count(texture_info.uri) > 0) {
      texture_info.texture_id = uri_to_id_.at(texture_info.uri);
    } else {
      texture_info.texture_id = TextureInfo::kBadTextureId;
      return false;
    }
  }
  return true;
}

void TextureManager::Evict(const TextureInfo& texture_info) {
  const auto& uri = texture_info.uri;
  ClearInflightRequest(uri);
  tile_texture_uris_.erase(uri);
  if (EnsureTextureId(texture_info)) {
    SLOG(SLOG_TEXTURES, "evicting $0", uri);
    uri_to_id_.erase(uri);
    id_to_texture_.erase(texture_info.texture_id);
    dispatch_->Send(&TextureListener::OnTextureEvicted, texture_info);
  }
}

void TextureManager::ClearInflightRequest(absl::string_view uri) {
  const std::string& key = static_cast<std::string>(uri);
  MutexLock lock(&requested_uris_mutex_);
  uris_to_request_.erase(key);
  requested_uris_.erase(key);
}

void TextureManager::EvictAll() {
  std::vector<std::string> uris_to_notify;
  for (const auto& uripair : uri_to_id_) {
    uris_to_notify.push_back(uripair.first);
  }
  {
    absl::MutexLock lock(&requested_uris_mutex_);
    uris_to_request_.clear();
    requested_uris_.clear();
  }
  id_to_texture_.clear();
  uri_to_id_.clear();
  for (const auto& uri : uris_to_notify) {
    dispatch_->Send(&TextureListener::OnTextureEvicted, TextureInfo(uri));
  }
}

namespace {

inline void SetRGBLightBlue(uint8_t* color_start) {
  *color_start = 0x77;
  *(color_start + 1) = 0x77;
  *(color_start + 2) = 0xFF;
}

void DrawTileOutline(ClientBitmap* bitmap) {
  const auto size = bitmap->sizeInPx().width;
  if (bitmap->bytesPerTexel() == 4) {
    uint32_t* buf = reinterpret_cast<uint32_t*>(bitmap->imageByteData());
    uint32_t light_blue = htonl(0x7777FFFF);
    std::fill_n(buf, size, light_blue);
    std::fill_n(buf + (size - 1) * size, size, light_blue);
    for (int i = 1; i < size; i++) {
      *(buf + size * i) = light_blue;
      *((buf + size * i) - 1) = light_blue;
    }
  } else {
    uint8_t* buf = static_cast<uint8_t*>(bitmap->imageByteData());
    for (int i = 0; i < size; i++) {
      SetRGBLightBlue(buf + (i * 3));
    }
    uint8_t* bottom = buf + (size - 1) * size * 3;
    for (int i = 0; i < size; i++) {
      SetRGBLightBlue(bottom + (i * 3));
    }
    uint8_t* scanline = buf + size * 3;
    for (int y = 1; y < size; y++) {
      SetRGBLightBlue(scanline);
      SetRGBLightBlue(scanline - 3);
      scanline += size * 3;
    }
  }
}
}  // namespace

/**
 * This Task performs an ITextureRequestHandler's bitmap rendering in the
 * engine's task thread. It borrows pointers to its owning TextureManager and
 * the relevant ITextureRequestHandler.
 */
class TextureFetchTask : public Task {
 public:
  TextureFetchTask(std::shared_ptr<TextureManager> owner,
                   std::shared_ptr<ITextureRequestHandler> provider,
                   std::string uri)
      : owner_(owner), handler_(provider), uri_(uri) {}

  bool RequiresPreExecute() const override { return false; }
  void PreExecute() override {}

  void Execute() override {
    auto shared_handler = handler_.lock();
    if (shared_handler == nullptr) {
      SLOG(SLOG_WARNING, "texture provider gc'ed before render");
      return;
    }
    Status result;
    if (auto* tile_provider =
            dynamic_cast<ITileProvider*>(shared_handler.get())) {
      auto owner = owner_.lock();
      if (owner == nullptr) {
        SLOG(SLOG_WARNING, "texture manager gc'ed before render");
        return;
      }
      if (!owner->IsLoading(uri_)) {
        SLOG(SLOG_TEXTURES, "$0 cancelled before render", uri_);
        return;
      }
      bitmap_ = owner->GetTileBitmap();
      owner->fetch_timer_->Begin();
      SLOG(SLOG_TEXTURES, "requesting $0", uri_);
      result = tile_provider->HandleTileRequest(uri_, bitmap_.get());
      if (result && owner->GetTilePolicy().debug_tiles) {
        DrawTileOutline(bitmap_.get());
      }
      owner->fetch_timer_->End();
    } else if (auto* texture_provider =
                   dynamic_cast<ITextureProvider*>(shared_handler.get())) {
      result =
          texture_provider->HandleTextureRequest(uri_, &bitmap_, &asset_type_);
    } else {
      result = ErrorStatus("could not use $0", shared_handler->ToString());
    }
    if (!result) {
      SLOG(SLOG_ERROR, "$0", result);
    }
  }

  void OnPostExecute() override {
    if (bitmap_) {
      if (auto owner = owner_.lock()) {
        if (!owner->IsLoading(uri_)) {
          SLOG(SLOG_TEXTURES, "$0 evicted before transfer to GPU", uri_);
          return;
        }
        owner->generate_texture_timer_->Begin();
        SLOG(SLOG_TEXTURES, "uploading $0 to GPU", uri_);
        owner->GenerateTexture(uri_, *bitmap_, TextureParams(asset_type_));
        owner->generate_texture_timer_->End();
      } else {
        SLOG(SLOG_WARNING, "texture manager gc'ed before texture generated");
      }
    }
  }

 private:
  std::weak_ptr<TextureManager> owner_;
  std::weak_ptr<ITextureRequestHandler> handler_;
  std::string uri_;
  std::unique_ptr<ClientBitmap> bitmap_;
  proto::ImageInfo::AssetType asset_type_{proto::ImageInfo::DEFAULT};
};

bool TextureManager::TextureFetchInitiated(const std::string& uri) {
  for (const auto& provider_pair : texture_handlers_) {
    if (provider_pair.second->CanHandleTextureRequest(uri)) {
      SLOG(SLOG_TEXTURES, "request for $0 queued for handling by $1", uri,
           provider_pair.first);
      task_runner_->PushTask(absl::make_unique<TextureFetchTask>(
          shared_from_this(), provider_pair.second, uri));
      return true;
    }
  }
  return false;
}

void TextureManager::OnFrameEnd() {
  std::vector<std::string> uris;
  {
    absl::MutexLock lock(&requested_uris_mutex_);
    if (!uris_to_request_.empty()) {
      // Move uris_to_request_'s elements; this avoids a potential issue where
      // RequestImage synchronously calls back to GenerateTexture.
      absl::c_copy(uris_to_request_, std::back_inserter(uris));
      uris_to_request_.clear();
      const size_t max_tiles = tile_policy_.max_tiles_fetched_per_frame;
      if (max_tiles != 0 && uris.size() > max_tiles) {
        // We're dropping some texture requests, so request another frame to
        // give the renderer another chance to request a needed texture.
        frame_state_->RequestFrame();
        SLOG(SLOG_TEXTURES, "dropping $0 texture requests",
             uris.size() - max_tiles);
        // Favor shorter URIs, as those are likely to be tiles that cover
        // larger areas (because of how zoomspecs are encoded).
        absl::c_sort(uris, [](const std::string& a, const std::string& b) {
          return a.size() < b.size();
        });
        uris.resize(max_tiles);
      }
      requested_uris_.insert(uris.begin(), uris.end());
    }
  }
  for (const std::string& uri : uris) {
    if (!TextureFetchInitiated(uri)) {
      platform_->RequestImage(uri);
    }
  }
  EvictStaleTiles();
  frame_tile_requests_.clear();
}

bool TextureManager::MaybeStartClientImageRequest(absl::string_view uri_view) {
  const auto& uri = static_cast<std::string>(uri_view);
  MarkAsFresh(uri);
  absl::MutexLock lock(&requested_uris_mutex_);
  if (uri_to_id_.find(uri) == uri_to_id_.end() && !IsLoadingInternal(uri)) {
    SLOG(SLOG_TEXTURES, "uris_to_request_ <- $0", uri);
    uris_to_request_.insert(uri);
    return true;
  }
  return false;
}

bool TextureManager::HasTexture(const TextureInfo& texture_info) const {
  return id_to_texture_.find(texture_info.texture_id) != id_to_texture_.end() ||
         uri_to_id_.find(texture_info.uri) != uri_to_id_.end();
}

bool TextureManager::IsLoading(const std::string& uri) const {
  absl::MutexLock lock(&requested_uris_mutex_);
  return IsLoadingInternal(uri);
}

bool TextureManager::IsLoadingInternal(const std::string& uri) const {
  return uris_to_request_.count(uri) > 0 || requested_uris_.count(uri) > 0;
}

void TextureManager::AddTextureRequestHandler(
    const std::string& handler_id,
    std::shared_ptr<ITextureRequestHandler> handler) {
  ASSERT(!handler_id.empty());
  RemoveTextureRequestHandler(handler_id);
  SLOG(SLOG_TEXTURES, "adding texture provider $0 to texture manager",
       handler_id);
  texture_handlers_.emplace_back(handler_id, std::move(handler));
}

ITextureRequestHandler* TextureManager::GetTextureRequestHandler(
    const std::string& handler_id) const {
  for (const auto& packet : texture_handlers_) {
    if (packet.first == handler_id) return packet.second.get();
  }
  return nullptr;
}

void TextureManager::RemoveTextureRequestHandler(
    const std::string& handler_id) {
  SLOG(SLOG_TEXTURES, "Removing texture provider $0", handler_id);
  texture_handlers_.erase(
      std::remove_if(texture_handlers_.begin(), texture_handlers_.end(),
                     [handler_id](const ProviderPacket& p) {
                       return p.first == handler_id;
                     }),
      texture_handlers_.end());
}

size_t TextureManager::CurrentTileRamUsage() const {
  return tile_texture_uris_.size() * tile_policy_.BytesPerTile();
}

namespace {
struct SortableTileUri {
  SortableTileUri(absl::string_view uri, int distance_from_recent_tile)
      : uri(uri), distance_from_recent_tile(distance_from_recent_tile) {}
  std::string ToString() const {
    return Substitute("$0@$1", uri, distance_from_recent_tile);
  }
  absl::string_view uri;
  uint32_t distance_from_recent_tile;
};
}  // namespace

void TextureManager::EvictStaleTiles() {
  if (frame_tile_requests_.empty()) {
    return;
  }

  {
    MutexLock lock(&requested_uris_mutex_);
    std::set<std::string> cancellable;
    absl::c_copy(requested_uris_,
                 std::inserter(cancellable, cancellable.end()));
    if (!cancellable.empty()) {
      // The difference between cancellable - fresh = stale.
      std::vector<std::string> stale;
      absl::c_set_difference(cancellable, frame_tile_requests_,
                             std::inserter(stale, stale.begin()));
      if (!stale.empty())
        SLOG(SLOG_TEXTURES, "\nCancellable: $0\nfresh: $1\nstale: $2",
             cancellable, frame_tile_requests_, stale);
      // Cancel in-flight requests for stale tiles.
      for (const auto& uri : stale) {
        if (!IsEvictableUri(uri)) continue;
        requested_uris_.erase(uri);
        SLOG(SLOG_TEXTURES, "cancelled $0", uri);
      }
    }
  }

  // When figuring out what to evict or cancel, we want those tiles "farthest"
  // from tiles requested this frame.
  // We use the most-zoomed tile requested during this frame as our reference.
  absl::optional<PageTileSpec> reference_tile;
  for (const std::string& uri : frame_tile_requests_) {
    auto maybe_frag = PageTileSpec::Parse(uri);
    if (!maybe_frag) {
      SLOG(SLOG_ERROR, "Cannot parse $0 as tile distance basis: $1", uri,
           maybe_frag.error_message());
      // Crash in debug.
      ASSERT(maybe_frag);
      return;
    }
    PageTileSpec frag = maybe_frag.ValueOrDie();
    if (!reference_tile ||
        frag.Zoom().Depth() > reference_tile->Zoom().Depth()) {
      reference_tile = frag;
    }
  }

  const size_t tile_size = tile_policy_.BytesPerTile();
  if (CurrentTileRamUsage() <= tile_policy_.max_tile_ram) {
    return;
  }
  const size_t bytes_over_budget =
      CurrentTileRamUsage() - tile_policy_.max_tile_ram;
  const size_t tiles_to_remove =
      std::max<size_t>(1, bytes_over_budget / tile_size);
  SLOG(SLOG_TEXTURES, "$0 over budget, plan to remove $1 $2-byte $3^2 tiles",
       bytes_over_budget, tiles_to_remove, tile_size,
       tile_policy_.tile_side_length);

  SLOG(SLOG_TEXTURES, "requested this frame: $0", frame_tile_requests_);

  std::vector<SortableTileUri> eviction_candidates;
  for (const auto& uri : tile_texture_uris_) {
    if (frame_tile_requests_.find(uri) != frame_tile_requests_.end()) {
      // It can happen that simply being zoomed out a particular amount can put
      // us over budget for tiles. But we can't evict anything currently
      // on-screen.
      continue;
    }
    auto frag = PageTileSpec::Parse(uri);
    if (!frag) {
      SLOG(SLOG_ERROR, "Cannot parse $0 as eviction candidate: $1", uri,
           frag.error_message());
      // Crash in debug.
      ASSERT(frag);
      return;
    }
    eviction_candidates.emplace_back(
        uri, reference_tile->DistanceFrom(frag.ValueOrDie()));
  }
  // Sort by distance descending.
  absl::c_sort(eviction_candidates, [](const SortableTileUri& a,
                                       const SortableTileUri& b) {
    return b.distance_from_recent_tile < a.distance_from_recent_tile;
  });
  SLOG(SLOG_TEXTURES, "eviction candidates: $0", eviction_candidates);

  // Evict!
  int removed = 0;
  for (size_t i = 0;
       i < eviction_candidates.size() && removed < tiles_to_remove; i++) {
    const auto& uri = eviction_candidates.at(i).uri;
    Evict(TextureInfo(uri));
    removed++;
  }
}  // namespace ink

void TextureManager::SetTilePolicy(const TilePolicy& new_policy) {
  tile_policy_ = new_policy;
  SLOG(SLOG_TEXTURES, "new tile policy $0", tile_policy_);
  EvictAll();
  bitmap_pool_.reset();
}

std::unique_ptr<ClientBitmap> TextureManager::GetTileBitmap() const {
  if (!bitmap_pool_) {
    bitmap_pool_ = absl::make_unique<ClientBitmapPool>(
        tile_policy_.bitmap_pool_size,
        ImageSize(tile_policy_.tile_side_length, tile_policy_.tile_side_length),
        tile_policy_.image_format);
  }
  return bitmap_pool_->TakeBitmap();
}

}  // namespace ink
