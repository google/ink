// Copyright 2024 Google LLC
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

#ifndef INK_RENDERING_TEXTURE_BITMAP_STORE_H_
#define INK_RENDERING_TEXTURE_BITMAP_STORE_H_

#include "absl/base/nullability.h"
#include "absl/status/statusor.h"
#include "ink/rendering/bitmap.h"
#include "ink/types/uri.h"

namespace ink {

// Interface for loading texture bitmap data for texture URIs.
class TextureBitmapStore {
 public:
  virtual ~TextureBitmapStore() = default;

  // Retrieve a `Bitmap` for the given texture URI. This may be called
  // synchronously during rendering, so loading of texture files from disk and
  // decoding them into bitmap data should be done in advance. The result may be
  // cached by consumers, so this should return a deterministic result for a
  // given input.
  virtual absl::StatusOr<absl::Nonnull<const Bitmap*>> GetTextureBitmap(
      const Uri& texture_uri) const = 0;
};

}  // namespace ink

#endif  // INK_RENDERING_TEXTURE_BITMAP_STORE_H_
