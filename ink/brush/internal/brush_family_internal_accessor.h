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

#ifndef INK_STROKES_BRUSH_INTERNAL_BRUSH_FAMILY_INTERNAL_ACCESSOR_H_
#define INK_STROKES_BRUSH_INTERNAL_BRUSH_FAMILY_INTERNAL_ACCESSOR_H_

#include <string>
#include <utility>

#include "ink/brush/brush_family.h"

namespace ink {

class BrushFamilyInternalAccessor {
 public:
  static void SetOpaqueDecodedProtoBytesWithFallbacks(std::string opaque_bytes,
                                                      BrushFamily& family) {
    family.opaque_decoded_proto_bytes_with_fallbacks_ = std::move(opaque_bytes);
  }

  static const std::string& GetOpaqueDecodedProtoBytesWithFallbacks(
      const BrushFamily& family) {
    return family.opaque_decoded_proto_bytes_with_fallbacks_;
  }
};

}  // namespace ink

#endif  // INK_STROKES_BRUSH_INTERNAL_BRUSH_FAMILY_INTERNAL_ACCESSOR_H_
