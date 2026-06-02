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

#ifndef INK_STORAGE_INTERNAL_JNI_BRUSH_FAMILY_NATIVE_HELPERS_H_
#define INK_STORAGE_INTERNAL_JNI_BRUSH_FAMILY_NATIVE_HELPERS_H_

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "absl/base/nullability.h"
#include "ink/brush/brush_family.h"
#include "ink/storage/brush.h"

namespace ink::native {

using MultipleBrushFamilies = std::vector<std::unique_ptr<BrushFamily>>;

inline int64_t NewNativeMultipleBrushFamilies(
    MultipleBrushFamilies&& families) {
  return reinterpret_cast<int64_t>(
      new MultipleBrushFamilies(std::forward<MultipleBrushFamilies>(families)));
}

inline MultipleBrushFamilies& CastToMutableMultipleBrushFamilies(
    int64_t native_pointer) {
  return *reinterpret_cast<MultipleBrushFamilies*>(native_pointer);
}

inline void DeleteMultipleBrushFamilies(int64_t native_pointer) {
  delete reinterpret_cast<MultipleBrushFamilies*>(native_pointer);
}

ClientTextureIdProviderAndBitmapReceiver
ClientTextureIdProviderAndBitmapReceiverFromNativeCallback(
    void* absl_nonnull pass_through,
    const char* absl_nullable (*absl_nonnull decode_texture_callback)(
        void* absl_nonnull pass_through, const char* absl_nonnull encoded_id,
        const int8_t* absl_nonnull bitmap, int bitmap_size));

TextureBitmapProvider TextureBitmapProviderFromNativeArrays(
    const char* absl_nonnull* absl_nullable texture_ids,
    const int8_t* absl_nullable* absl_nullable bitmaps,
    const int* absl_nullable bitmap_lengths, int num_textures);

}  // namespace ink::native

#endif  // INK_STORAGE_INTERNAL_JNI_BRUSH_FAMILY_NATIVE_HELPERS_H_
