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

#include "ink/engine/scene/grid_assets/grid_texture_provider.h"

#include "third_party/absl/memory/memory.h"
#include "third_party/absl/strings/str_cat.h"
#include "third_party/absl/strings/strip.h"
#include "ink/engine/scene/grid_assets/grid_assets.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"
#include "ink/proto/bitmap_portable_proto.pb.h"
#include "third_party/zlib/zlib.h"

namespace ink {

bool GridTextureProvider::CanHandleTextureRequest(absl::string_view uri) const {
  if (uri.find_first_of("sketchology://grid/") == 0) {
    for (const FileToc* g = ink::grid_assets_create(); g->name != nullptr;
         g++) {
      if (uri == absl::StrCat("sketchology://grid/",
                              absl::StripSuffix(g->name, ".rawproto"))) {
        return true;
      }
    }
  }
  return false;
}

Status GridTextureProvider::HandleTextureRequest(
    absl::string_view uri, std::unique_ptr<ClientBitmap>* out,
    proto::ImageInfo::AssetType* asset_type) const {
  bool found = false;
  proto::Bitmap bitmap_proto;
  for (const FileToc* g = ink::grid_assets_create(); g->name != nullptr; g++) {
    if (uri == absl::StrCat("sketchology://grid/",
                            absl::StripSuffix(g->name, ".rawproto"))) {
      found = true;
      if (!bitmap_proto.ParseFromArray(g->data, g->size)) {
        return ErrorStatus("Failed to decode bitmap proto for $0", g->name);
      }
    }
  }
  if (!found) {
    return ErrorStatus(StatusCode::INVALID_ARGUMENT, "Asset not found for $0",
                       uri);
  }
  unsigned long n_bytes =
      bitmap_proto.width() * bitmap_proto.height() *
      bytesPerTexelForFormat(ImageFormat::BITMAP_FORMAT_RGBA_8888);
  std::vector<uint8_t> bitmap_bytes(n_bytes);
  int zres = uncompress(bitmap_bytes.data(), &n_bytes,
                        reinterpret_cast<const Bytef*>(
                            bitmap_proto.compressed_bitmap_blob().data()),
                        bitmap_proto.compressed_bitmap_blob().size());
  if (zres != Z_OK) {
    return ErrorStatus("Couldn't decompress $0 error $1", uri, zres);
  }
  if (n_bytes != bitmap_bytes.size()) {
    return ErrorStatus("Expected $0 bytes but only $1 bytes read",
                       bitmap_bytes.size(), n_bytes);
  }

  *out = absl::make_unique<RawClientBitmap>(
      bitmap_bytes, ImageSize(bitmap_proto.width(), bitmap_proto.height()),
      ImageFormat::BITMAP_FORMAT_RGBA_8888);
  *asset_type = proto::ImageInfo::GRID;
  return OkStatus();
}

}  // namespace ink
