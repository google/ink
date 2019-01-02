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

// Implementations of the ink::Str() function, which stringifies things.
// Sketchology-authored types can implement
//    std::string ToString() const;
// to be automatically stringified in logging statements, etc.
// If you want to provide stringification for your enum class, then implement
//     template <>
//     std::string Str(const MyEnumClass& e) { ... }
#ifndef INK_ENGINE_UTIL_DBG_STR_H_
#define INK_ENGINE_UTIL_DBG_STR_H_

#include <inttypes.h>
#include <array>
#include <cstdint>
#include <iterator>
#include <list>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

#include "third_party/absl/meta/type_traits.h"
#include "third_party/absl/strings/str_format.h"
#include "third_party/absl/strings/str_join.h"
#include "third_party/absl/strings/substitute.h"
#include "third_party/glm/glm/glm.hpp"
#include "third_party/glm/glm/gtc/quaternion.hpp"
#include "ink/proto/elements_portable_proto.pb.h"
#include "ink/proto/geometry_portable_proto.pb.h"
#include "ink/proto/mutations_portable_proto.pb.h"

namespace ink {

// Forward declarations for the structs and functions in string_internal.
template <typename T>
std::string Str(const T& t);
template <typename... T>
std::string Substitute(absl::string_view, const T&...);

namespace strings_internal {
// A "has predicate" is a false_type unless it has a function  with the given
// name, in which case it is a true_type.
// CREATE_HAS_PREDICATE(FooBar) -> hasFooBar<T>::value gives true or false
#define CREATE_HAS_PREDICATE(name)                                \
  template <typename T, typename = void>                          \
  struct has##name : std::false_type {};                          \
                                                                  \
  template <typename T>                                           \
  struct has##name<T, decltype(std::declval<T>().name(), void())> \
      : std::true_type {};

CREATE_HAS_PREDICATE(GetTypeName);
CREATE_HAS_PREDICATE(DebugString);

#undef CREATE_HAS_PREDICATE

// Convenience function for stringifying an iterator range.
template <typename InputIt>
std::string MakeContainerString(InputIt begin, InputIt end) {
  using ValueType = typename std::iterator_traits<InputIt>::value_type;
  auto formatter = [](::string* out, ValueType t) { out->append(Str(t)); };
  return absl::StrJoin(begin, end, ", ", formatter);
}

// The default uses the object's ToString() method.
template <typename T, typename = void>
struct StrTemplateResolver {
  static std::string ToString(const T& t) { return t.ToString(); }
};

// This matches any reflection-enabled protos that don't have specializations.
template <typename T>
struct StrTemplateResolver<
    T, typename absl::enable_if_t<hasGetTypeName<T>::value &&
                                  hasDebugString<T>::value>> {
  static std::string ToString(const T& t) {
    auto s = t.DebugString();
    if (!s.empty()) s.pop_back();  // remove trailing newline
    return absl::Substitute("<$0 $1>", t.GetTypeName(), s);
  }
};

// This matches any lite runtime protos that don't have specializations.
template <typename T>
struct StrTemplateResolver<
    T, typename absl::enable_if_t<hasGetTypeName<T>::value &&
                                  !hasDebugString<T>::value>> {
  static std::string ToString(const T& t) {
    return absl::Substitute("<$0 instance>", t.GetTypeName());
  }
};

// This matches the fundamental integral types: short, int, long, long long, and
// their unsigned counterparts, as well as signed char. Note that this list does
// not include bool and unsigned char (as it normally would), as they have their
// own specializations of Str().
template <typename T>
struct StrTemplateResolver<
    T, typename absl::enable_if_t<std::is_integral<T>::value>> {
  static std::string ToString(T t) { return absl::Substitute("$0", t); }
};

// This matches the fundamental floating-point types: float, double, and
// long double.
template <typename T>
struct StrTemplateResolver<
    T, typename absl::enable_if_t<std::is_floating_point<T>::value>> {
  static std::string ToString(T t) {
#ifdef INK_EXTENDED_LOG_PRECISION
    return absl::StrFormat("%.8f", t);
#else
    return absl::StrFormat("%.2f", t);
#endif
  }
};

// Partial template specializations for STL containers.
template <typename T>
struct StrTemplateResolver<std::vector<T>> {
  static std::string ToString(const std::vector<T>& v) {
    return absl::StrCat("[", MakeContainerString(v.begin(), v.end()), "]");
  }
};
template <typename T>
struct StrTemplateResolver<std::list<T>> {
  static std::string ToString(const std::list<T>& l) {
    return absl::StrCat("[", MakeContainerString(l.begin(), l.end()), "]");
  }
};
template <typename T, int N>
struct StrTemplateResolver<std::array<T, N>> {
  static std::string ToString(const std::array<T, N>& a) {
    return absl::StrCat("[", MakeContainerString(a.begin(), a.end()), "]");
  }
};
template <typename T>
struct StrTemplateResolver<std::set<T>> {
  static std::string ToString(const std::set<T>& s) {
    return absl::StrCat("{", MakeContainerString(s.begin(), s.end()), "}");
  }
};
template <typename T>
struct StrTemplateResolver<std::unordered_set<T>> {
  static std::string ToString(const std::unordered_set<T>& u) {
    return absl::StrCat("unordered{", MakeContainerString(u.begin(), u.end()),
                        "}");
  }
};

}  // namespace strings_internal

std::string AddressStr(const void* p);

template <typename T>
std::string HexStr(const T& t) {
  return absl::StrCat(absl::Hex(t));
}

// This matches any type that doesn't have a specialization, and uses the
// StrTemplateResolver to determine how to handle it.
template <typename T>
std::string Str(const T& t) {
  return strings_internal::StrTemplateResolver<T>::ToString(t);
}

// Given a format string containing $0, $1,...,$N and N+1 arguments, stringifies
// each of the arguments and substitutes them into the format string.
// See http://en.cppreference.com/w/cpp/language/parameter_pack for this
// exotic syntax.
template <typename... T>
std::string Substitute(absl::string_view format, const T&... args) {
  return absl::Substitute(format, Str(args)...);
}

// absl::string_view doesn't provide a ToString method, so we specialize this
// template here.
template <>
inline std::string Str<absl::string_view>(const absl::string_view& t) {
  return std::string(t);
}

template <>
std::string Str(char* const& s);

template <>
std::string Str(const char* const& s);

template <>
std::string Str(const glm::mat4& mat);

template <>
std::string Str(const glm::ivec2& v);

template <>
std::string Str(const glm::vec2& v);

template <>
std::string Str(const glm::vec3& v);

template <>
std::string Str(const glm::vec4& v);

template <>
std::string Str(const glm::dvec2& v);

template <>
std::string Str(const glm::dvec3& v);

template <>
std::string Str(const glm::dvec4& v);

template <>
std::string Str(const unsigned char& c);

template <>
std::string Str(const std::string& s);

#ifdef HAS_GLOBAL_STRING
template <>
std::string Str(const string& s);
#endif

template <>
std::string Str(const bool& b);

template <>
std::string Str(const proto::Point& p);

template <>
std::string Str(const proto::Rect& r);

template <>
std::string Str(const proto::mutations::Mutation& mutations);

template <>
std::string Str(const proto::mutations::Mutation::Chunk& chunk);

template <>
std::string Str(const proto::mutations::AddElement& add_element);

template <>
std::string Str(
    const proto::mutations::RemoveElement& remove_element);

template <>
std::string Str(const proto::mutations::SetElementTransform&
                    set_element_transform);

template <>
std::string Str(
    const proto::mutations::SetWorldBounds& set_world_bounds);

template <>
std::string Str(const proto::mutations::SetBorder& set_border);

template <>
std::string Str(const proto::mutations::SetGrid& set_grid);

template <>
std::string Str(const proto::Border& border);

template <>
std::string Str(const proto::GridInfo& grid);

template <>
std::string Str(const proto::Element& e);

template <>
std::string Str(const proto::ElementAttributes& a);

template <>
std::string Str(const proto::ElementBundle& bundle);

template <>
std::string Str(const proto::AffineTransform& t);

template <>
std::string Str(
    const proto::mutations::SetVisibility& set_visibility);

template <>
std::string Str(const proto::mutations::SetOpacity& set_opacity);

template <>
std::string Str(
    const proto::mutations::ChangeZOrder& change_z_order);

}  // namespace ink

#undef MESSAGE_TYPE
#undef MESSAGE_LITE_TYPE

#endif  // INK_ENGINE_UTIL_DBG_STR_H_
