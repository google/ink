// Copyright 2026 Google LLC
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

#include "ink/storage/internal/jni/brush_family_serialization_native_helper.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include "absl/base/nullability.h"
#include "absl/container/flat_hash_map.h"
#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "ink/storage/brush.h"

namespace ink::native {

ClientTextureIdProviderAndBitmapReceiver
ClientTextureIdProviderAndBitmapReceiverFromNativeCallback(
    void* absl_nonnull pass_through,
    const char* absl_nullable (*absl_nonnull decode_texture_callback)(
        void* absl_nonnull pass_through, const char* absl_nonnull encoded_id,
        const int8_t* absl_nonnull bitmap, int bitmap_size)) {
  return [decode_texture_callback, pass_through](
             absl::string_view encoded_id,
             absl::string_view bitmap) -> absl::StatusOr<std::string> {
    // In the case where there is no callback, the null implementation returns
    // the passed-in C string. Thus, this needs to persist through the return
    // at the end of this function.
    std::string encoded_id_str(encoded_id);
    const char* new_id = (*decode_texture_callback)(
        pass_through, encoded_id_str.c_str(),
        reinterpret_cast<const int8_t*>(bitmap.data()), bitmap.size());
    if (new_id == nullptr) {
      return absl::InternalError(
          "Exception occurred during or before onDecodeTexture.");
    }
    return std::string(new_id);
  };
}

TextureBitmapProvider TextureBitmapProviderFromNativeArrays(
    const char* absl_nonnull* absl_nullable texture_ids,
    const int8_t* absl_nullable* absl_nullable bitmaps,
    const int* absl_nullable bitmap_lengths, int num_textures) {
  auto texture_map =
      std::make_shared<absl::flat_hash_map<std::string, std::string>>();
  if (num_textures > 0) {
    ABSL_CHECK_NE(texture_ids, nullptr);
    ABSL_CHECK_NE(bitmaps, nullptr);
    ABSL_CHECK_NE(bitmap_lengths, nullptr);
    texture_map->reserve(num_textures);
    for (int i = 0; i < num_textures; ++i) {
      texture_map->insert(
          {std::string(texture_ids[i]),
           // These bitmaps are passed as int8_t* to make it clear that they are
           // not C-style strings (notably, they may contain null bytes in the
           // middle). But the serialization code follows the Protobuf usage /
           // convention of storing bytes in std::string.
           std::string(bitmaps[i] == nullptr
                           ? ""
                           : reinterpret_cast<const char*>(bitmaps[i]),
                       bitmap_lengths[i])});
    }
  }
  return [texture_map](
             absl::string_view texture_id) -> std::optional<std::string> {
    if (auto it = texture_map->find(texture_id); it != texture_map->end()) {
      return it->second;
    }
    return std::nullopt;
  };
}

}  // namespace ink::native
