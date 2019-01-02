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

#include "ink/engine/util/dbg/str.h"

namespace ink {

std::string AddressStr(const void* p) { return absl::Substitute("$0", p); }

template <>
std::string Str(char* const& s) {
  return std::string(s);
}

template <>
std::string Str(const char* const& s) {
  return std::string(s);
}

template <>
std::string Str(const glm::mat4& mat) {
  auto format_matrix_row = [](const glm::mat4& mat, int row) {
    return absl::StrFormat("%10.4f %10.4f %10.4f %10.4f\n", mat[0][row],
                           mat[1][row], mat[2][row], mat[3][row]);
  };

  std::string s;
  for (int row = 0; row < 4; row++) {
    s += format_matrix_row(mat, row);
  }
  return s;
}

template <>
std::string Str(const glm::ivec2& v) {
  return absl::Substitute("($0, $1)", v.x, v.y);
}

template <>
std::string Str(const glm::vec2& v) {
  return Substitute("($0, $1)", v.x, v.y);
}

template <>
std::string Str(const glm::vec3& v) {
  return Substitute("($0, $1, $2)", v.x, v.y, v.z);
}

template <>
std::string Str(const glm::vec4& v) {
  return Substitute("($0, $1, $2, $3)", v.x, v.y, v.z, v.w);
}

template <>
std::string Str(const glm::dvec2& v) {
  return Substitute("($0, $1)", v.x, v.y);
}

template <>
std::string Str(const glm::dvec3& v) {
  return Substitute("($0, $1, $2)", v.x, v.y, v.z);
}

template <>
std::string Str(const glm::dvec4& v) {
  return Substitute("($0, $1, $2, $3)", v.x, v.y, v.z, v.w);
}

template <>
std::string Str(const unsigned char& c) {
  return absl::StrCat(absl::Hex(c, absl::kZeroPad2));
}

template <>
std::string Str(const std::string& s) {
  return s;
}

#ifdef HAS_GLOBAL_STRING
template <>
std::string Str(const string& s) {
  return std::string(s);
}
#endif

template <>
std::string Str(const bool& b) {
  return b ? "true" : "false";
}

template <>
std::string Str(const proto::Point& p) {
  return Substitute("<proto::Point ($0, $1)>", p.x(), p.y());
}

template <>
std::string Str(const proto::Rect& r) {
  return Substitute("<proto::Rect from ($0, $1) to ($2, $3)>", r.xlow(),
                    r.ylow(), r.xhigh(), r.yhigh());
}

template <>
std::string Str(const proto::mutations::Mutation& mutation) {
  std::string s("<Mutation");
  for (int i = 0; i < mutation.chunk_size(); i++) {
    s.append("\n  ");
    s.append(Str(mutation.chunk(i)));
  }
  s.append(">");
  return s;
}

template <>
std::string Str(const proto::mutations::Mutation::Chunk& chunk) {
  if (chunk.has_add_element()) {
    return Str(chunk.add_element());
  } else if (chunk.has_remove_element()) {
    return Str(chunk.remove_element());
  } else if (chunk.has_set_element_transform()) {
    return Str(chunk.set_element_transform());
  } else if (chunk.has_set_world_bounds()) {
    return Str(chunk.set_world_bounds());
  } else if (chunk.has_set_visibility()) {
    return Str(chunk.set_visibility());
  } else if (chunk.has_set_opacity()) {
    return Str(chunk.set_opacity());
  } else if (chunk.has_change_z_order()) {
    return Str(chunk.change_z_order());
  }
  return "";
}

template <>
std::string Str(const proto::mutations::AddElement& add_element) {
  if (add_element.below_element_with_uuid().empty()) {
    return Substitute("<AddElement $0>", add_element.element());
  }
  return Substitute("<AddElement $0 below $1>", add_element.element(),
                    add_element.below_element_with_uuid());
}

template <>
std::string Str(
    const proto::mutations::RemoveElement& remove_element) {
  return Substitute("<RemoveElement $0>", remove_element.uuid());
}

template <>
std::string Str(const proto::mutations::SetElementTransform&
                    set_element_transform) {
  return Substitute("<SetElementTransform $0 $1>", set_element_transform.uuid(),
                    set_element_transform.transform());
}

template <>
std::string Str(
    const proto::mutations::SetWorldBounds& set_world_bounds) {
  return Substitute("<SetWorldBounds $0>", set_world_bounds.bounds());
}

template <>
std::string Str(const proto::mutations::SetBorder& set_border) {
  return Substitute("<SetBorder $0>", set_border.border());
}

template <>
std::string Str(const proto::mutations::SetGrid& set_grid) {
  return Substitute("<SetGrid $0>", set_grid.grid());
}

template <>
std::string Str(
    const proto::mutations::SetVisibility& set_visibility) {
  return Substitute("<SetVisibility $0 = $1>", set_visibility.uuid(),
                    set_visibility.visibility());
}

template <>
std::string Str(const proto::mutations::SetOpacity& set_opacity) {
  return Substitute("<SetOpacity $0 = $1>", set_opacity.uuid(),
                    set_opacity.opacity());
}

template <>
std::string Str(
    const proto::mutations::ChangeZOrder& change_z_order) {
  return Substitute("<ChangeZOrder $0 below $1>", change_z_order.uuid(),
                    change_z_order.below_uuid());
}

template <>
std::string Str(const proto::ElementBundle& bundle) {
  std::string s = Substitute("<ElementBundle $0 ", bundle.uuid());
  if (bundle.has_element()) {
    s.append(Substitute("$0", bundle.element()));
  }
  s.append(">");
  return s;
}

template <>
std::string Str(const proto::Border& border) {
  return Substitute("uri: $0 scale: $1", border.uri(), border.scale());
}

template <>
std::string Str(const proto::GridInfo& grid) {
  return Substitute("uri: $0 rgba_premultiplier: $1 size_world: $2 origin: $3",
                    grid.uri(), HexStr(grid.rgba_multiplier()),
                    grid.size_world(), grid.origin());
}

template <>
std::string Str(const proto::Element& e) {
  return Substitute("<Element $0>", e.attributes());
}

template <>
std::string Str(const proto::ElementAttributes& a) {
  return Substitute(
      "<ElementAttributes selectable:$0 magic_erasable:$1 is_sticker:$2 "
      "is_text:$3 is_group:$4 is_zoomable:$5",
      a.selectable(), a.magic_erasable(), a.is_sticker(), a.is_text(),
      a.is_group(), a.is_zoomable());
}

template <>
std::string Str(const proto::AffineTransform& t) {
  return Substitute(
      "<AffineTransform tx: $0 ty: $1 scale_x: $2 scale_y: $3 "
      "rotation_radians: $4>",
      t.tx(), t.ty(), t.scale_x(), t.scale_y(), t.rotation_radians());
}

}  // namespace ink
