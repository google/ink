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

#ifndef INK_ENGINE_SCENE_TYPES_TEXT_H_
#define INK_ENGINE_SCENE_TYPES_TEXT_H_

#include <string>
#include <tuple>

#include "third_party/absl/types/optional.h"
#include "third_party/absl/types/variant.h"
#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/public/types/status.h"
#include "ink/engine/public/types/uuid.h"
#include "ink/engine/util/security.h"
#include "ink/proto/text_portable_proto.pb.h"

namespace ink {
namespace text {

enum class PostscriptFont {
  kUndefined,
  kHelveticaRegular,
  kHelveticaOblique,
  kHelveticaBold,
  kHelveticaBoldOblique,
  kCourierRegular,
  kCourierOblique,
  kCourierBold,
  kCourierBoldOblique,
  kTimesRegular,
  kTimesOblique,
  kTimesBold,
  kTimesBoldOblique,
  kSymbol,
};

using Font = absl::variant<PostscriptFont, std::string, uint32_t>;

enum class font_type_tag { kPostscriptFont, kName, kAssetId, kResourceId };

enum class Alignment { kUndefined, kLeft, kCentered, kRight };

struct TextSpec {
  std::string text_utf8;

  font_type_tag font_tag;
  Font font;
  // Font size expressed as a fraction of the width of the text box.
  float font_size_fraction;
  glm::vec4 color{0, 0, 0, 0};
  Alignment alignment;

  glm::vec4 shadow_color{0, 0, 0, 0};
  // Shadow sizes expressed as a fraction of the width of the text box.
  float shadow_radius_fraction = 0;
  float shadow_dx_fraction = 0;
  float shadow_dy_fraction = 0;

  // Layout information for this text (when provided). Kept in proto form
  // because it's not used within ink.
  absl::optional<proto::text::Layout> layout;

  TextSpec() : color(0, 0, 0, 1), alignment(Alignment::kLeft) {
    font = PostscriptFont::kHelveticaRegular;
    font_tag = font_type_tag::kPostscriptFont;
  }

  TextSpec(const TextSpec&) = default;
  TextSpec& operator=(const TextSpec&) = default;

  static ABSL_MUST_USE_RESULT Status
  ReadFromProto(const proto::text::Text& unsafe_proto, TextSpec* result);

  static void WriteToProto(proto::text::Text* proto, const TextSpec& text);

  bool operator==(const TextSpec& other) const {
    return text_utf8 == other.text_utf8 && font_tag == other.font_tag &&
           font == other.font &&
           font_size_fraction == other.font_size_fraction &&
           color == other.color && alignment == other.alignment &&
           shadow_color == other.shadow_color &&
           shadow_radius_fraction == other.shadow_radius_fraction &&
           shadow_dx_fraction == other.shadow_dx_fraction &&
           shadow_dy_fraction == other.shadow_dy_fraction;
  }
};

}  // namespace text
}  // namespace ink

#endif  // INK_ENGINE_SCENE_TYPES_TEXT_H_
