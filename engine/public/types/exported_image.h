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

#ifndef INK_ENGINE_PUBLIC_TYPES_EXPORTED_IMAGE_H_
#define INK_ENGINE_PUBLIC_TYPES_EXPORTED_IMAGE_H_

#include <cstdint>
#include <vector>

#include "third_party/glm/glm/glm.hpp"

namespace ink {

struct ExportedImage {
  glm::ivec2 size_px{0, 0};
  std::vector<uint8_t> bytes;
  uint64_t fingerprint;
};

}  // namespace ink

#endif  // INK_ENGINE_PUBLIC_TYPES_EXPORTED_IMAGE_H_
