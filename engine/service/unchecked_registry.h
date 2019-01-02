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

#ifndef INK_ENGINE_SERVICE_UNCHECKED_REGISTRY_H_
#define INK_ENGINE_SERVICE_UNCHECKED_REGISTRY_H_

#include <memory>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

#include "ink/engine/service/common_internal.h"
#include "ink/engine/service/definition_list.h"

namespace ink {
namespace service {

// This class is a registry of std::shared_ptrs to the service objects used in
// the engine. It is responsible for instantiating all of the services, using
// the information in the DefinitionList, as well as ensuring that the
// dependency graph is free of cycles.
//
// "Unchecked" refers to the fact that any service may be requested --
// contrasted with Registry, which performs compile-time checks to ensure that
// only declared dependencies may be accessed.
class UncheckedRegistry {
 public:
  // Constructs the registry, and instantiates all types in the definition list.
  explicit UncheckedRegistry(std::unique_ptr<DefinitionList> definition_list);
  UncheckedRegistry(const UncheckedRegistry&) = delete;
  UncheckedRegistry& operator=(const UncheckedRegistry&) = delete;

  // Fetches the requested service. Results in a run-time error if the service
  // is not defined.
  //
  // Note that the template type must be the interface, not the implementation,
  // as defined in the DefinitionList.
  template <typename T>
  T* Get() const;
  template <typename T>
  std::shared_ptr<T> GetShared() const;

 private:
  service_internal::TypePointerMap::const_iterator FindType(
      std::type_index type) const;
  void InstantiateType(std::type_index type);

  service_internal::TypePointerMap type_to_impl_;
  std::unique_ptr<DefinitionList> definition_list_;
};

template <typename T>
T* UncheckedRegistry::Get() const {
  auto it = FindType(std::type_index(typeid(T)));
  return static_cast<T*>(it->second.get());
}

template <typename T>
std::shared_ptr<T> UncheckedRegistry::GetShared() const {
  auto it = FindType(std::type_index(typeid(T)));
  return std::static_pointer_cast<T>(it->second);
}

}  // namespace service
}  // namespace ink

#endif  // INK_ENGINE_SERVICE_UNCHECKED_REGISTRY_H_
