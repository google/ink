// Copyright 2025 Google LLC
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
#ifndef INK_STORAGE_BITMAP_H_
#define INK_STORAGE_BITMAP_H_

#include "absl/status/statusor.h"
#include "ink/rendering/bitmap.h"
#include "ink/storage/proto/bitmap.pb.h"

namespace ink {

// Populates the `proto::Bitmap` by encoding the given Bitmap.
//
// The `bitmap_proto` need not be empty before calling these; they will
// effectively clear the proto first.
void EncodeBitmap(const Bitmap& bitmap, ink::proto::Bitmap& bitmap_proto);
// Decodes the `proto::Bitmap` into a Bitmap. Returns an error if the proto
// is invalid.
absl::StatusOr<VectorBitmap> DecodeBitmap(const proto::Bitmap& bitmap_proto);

proto::Bitmap::PixelFormat EncodePixelFormat(
    const Bitmap::PixelFormat& pixel_format);
absl::StatusOr<Bitmap::PixelFormat> DecodePixelFormat(
    const proto::Bitmap::PixelFormat& pixel_format_proto);
}  // namespace ink
#endif  // INK_STORAGE_BITMAP_H_
