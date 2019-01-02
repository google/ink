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

#ifndef INK_ENGINE_SERVICE_COMMON_INTERNAL_H_
#define INK_ENGINE_SERVICE_COMMON_INTERNAL_H_

#include <memory>
#include <typeindex>

#include "third_party/absl/container/flat_hash_map.h"
#include "third_party/absl/container/flat_hash_set.h"
#include "third_party/absl/meta/type_traits.h"
#include "ink/engine/service/dependencies.h"

namespace ink {
namespace service {
namespace service_internal {

using TypeIndexSet = absl::flat_hash_set<std::type_index>;
using TypePointerMap =
    absl::flat_hash_map<std::type_index, std::shared_ptr<void>>;

// This struct allows us to treat classes with no dependencies
// declaration as having declared an empty list of dependencies.
template <typename, typename = absl::void_t<>>
struct SharedDepsType {
  using type = Dependencies<>;
};

template <typename T>
struct SharedDepsType<T, absl::void_t<typename T::SharedDeps>> {
  using type = typename T::SharedDeps;
};

// This helper struct constructs a TypeIndexSet from a Dependencies declaration.
template <typename... T>
struct GetDepsAsTypeIndexSet;

template <typename... T>
struct GetDepsAsTypeIndexSet<Dependencies<T...>> {
  static TypeIndexSet Apply() {
    return TypeIndexSet({std::type_index(typeid(T))...});
  }
};

// Convenience function to get T::SharedDeps as a TypeIndexSet.
template <typename T>
TypeIndexSet GetSharedDeps() {
  return GetDepsAsTypeIndexSet<typename SharedDepsType<T>::type>::Apply();
}

// This struct allows for compile-time checking of whether a type is in the
// dependency list, similar to ::util::tuple::find
template <typename... T>
struct TypeInDepList;

template <typename TypeToFind>
struct TypeInDepList<TypeToFind> {
  static constexpr bool value = false;
};

template <typename TypeToFind, typename DepsHead, typename... DepsTail>
struct TypeInDepList<TypeToFind, DepsHead, DepsTail...> {
  static constexpr bool value = std::is_same<TypeToFind, DepsHead>::value ||
                                TypeInDepList<TypeToFind, DepsTail...>::value;
};

template <typename TypeToFind, typename... DepsTypes>
struct TypeInDepList<TypeToFind, Dependencies<DepsTypes...>> {
  static constexpr bool value = TypeInDepList<TypeToFind, DepsTypes...>::value;
};

// HasSharedDep<A, B>::value will be true iff A::SharedDeps contains B.
template <typename Object, typename Type>
struct HasSharedDep {
  static constexpr bool value =
      TypeInDepList<Type, typename Object::SharedDeps>::value;
};

}  // namespace service_internal
}  // namespace service
}  // namespace ink

#endif  // INK_ENGINE_SERVICE_COMMON_INTERNAL_H_
