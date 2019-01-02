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

// Represents a persistent sketchology scene graph element, such as a stroke,
// a text area, or a bitmapped image.

#ifndef INK_ENGINE_SCENE_TYPES_ELEMENT_ID_H_
#define INK_ENGINE_SCENE_TYPES_ELEMENT_ID_H_

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

#include "third_party/absl/container/flat_hash_map.h"
#include "third_party/absl/container/flat_hash_set.h"
#include "ink/engine/util/dbg/errors.h"  // for RUNTIME_ERROR
#include "ink/engine/util/dbg/str.h"
#include "ink/engine/util/funcs/utils.h"

namespace ink {

// Enough bits to encode ElementType.
static const size_t kTypeBits = 2;
static const uint64_t kTypeMask = ~(0xFFFFFFFF << kTypeBits);

// NOTE: Make sure kTypeBits has a value >= log2(max elementtype).
enum ElementType {
  // No valid element has a type of 0 so an element_id of 0 is usefully invalid.
  INVALID = 0,

  // Strokes, flood fills, and bitmapped images are all polys.
  POLY,

  // A container of a set of other elements. Only POLY elements are supported
  // for now. GROUPs may not define strokes, fills, or any other drawable.
  GROUP
};

template <>
inline std::string Str(const ElementType& t) {
  switch (t) {
    case POLY:
      return "POLY";
    case GROUP:
      return "GROUP";
    default:
      return std::to_string(t);
  }
}

// A lightweight identifier for an element.
class ElementId {
 public:
  ElementId() {}
  explicit ElementId(uint32_t handle) : ElementId(INVALID, handle) {}
  ElementId(ElementType type, uint32_t handle) {
    handle_and_type_ = type | handle << kTypeBits;
  }

  inline ElementType Type() const {
    return static_cast<ElementType>((handle_and_type_ & kTypeMask));
  }

  inline uint32_t Handle() const { return handle_and_type_ >> kTypeBits; }

  std::string ToString() const { return std::to_string(Handle()); }

  std::string ToStringExtended() const {
    return Substitute("handle:$0, type:$1", Handle(), Type());
  }

  bool operator==(const ElementId& other) const {
    return handle_and_type_ == other.handle_and_type_;
  }

  bool operator!=(const ElementId& other) const { return !(other == *this); }

  bool operator<(const ElementId& other) const {
    return handle_and_type_ < other.handle_and_type_;
  }

 private:
  uint32_t handle_and_type_;

  friend class ElementIdHasher;

  template <typename H>
  friend H AbslHashValue(H h, const ink::ElementId& id) {
    return H::combine(std::move(h), id.handle_and_type_);
  }
};

// Make GroupId an alias to an ElementId. A GroupId should have type GROUP.
using GroupId = ElementId;

// No real element can have this id.
const ElementId kInvalidElementId = ElementId(INVALID, 0);

class ElementIdHasher {
 public:
  size_t operator()(const ink::ElementId& id) const {
    return hasher_(id.handle_and_type_);
  }

 private:
  absl::flat_hash_set<uint32_t>::hasher hasher_;
};

template <typename T>
using ElementIdHashMap = absl::flat_hash_map<ElementId, T, ElementIdHasher>;

template <typename T>
using GroupIdHashMap = absl::flat_hash_map<GroupId, T, ElementIdHasher>;

using ElementIdHashSet = absl::flat_hash_set<ElementId, ElementIdHasher>;
using GroupIdHashSet = absl::flat_hash_set<GroupId, ElementIdHasher>;

class ElementIdSource {
 public:
  explicit ElementIdSource(uint32_t starting_id)
      : next_increasing_id_(starting_id) {}

  ElementId CreatePolyId() {
    uint32_t n = next_increasing_id_;
    EXPECT(++next_increasing_id_ > n);
    return ElementId(POLY, n);
  }

  GroupId CreateGroupId() {
    uint32_t n = next_increasing_id_;
    EXPECT(++next_increasing_id_ > n);
    return ElementId(GROUP, n);
  }

 private:
  // The next available monotonically increasing numeric id portion of an
  // element_id.
  uint32_t next_increasing_id_;
};

// Comparators
namespace scene_element {
// comparison functions
inline bool LessByHandle(const ElementId& a, const ElementId& b) {
  return a.Handle() < b.Handle();
}
}  // namespace scene_element

// Handy "blow up because I can't handle this type of element" macro.
#define UNHANDLED_ELEMENT_TYPE(id) \
  RUNTIME_ERROR("Can't handle elements this type! ($0)", (id));

}  // namespace ink

#endif  // INK_ENGINE_SCENE_TYPES_ELEMENT_ID_H_
