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

#ifndef INK_ENGINE_SERVICE_REGISTRY_H_
#define INK_ENGINE_SERVICE_REGISTRY_H_

#include "ink/engine/service/common_internal.h"
#include "ink/engine/service/dependencies.h"
#include "ink/engine/util/dbg/errors.h"

namespace ink {
namespace service {

// This class is a registry of std::shared_ptrs to service objects that are
// depended on by type Owner. Owner must accept a Registry<Owner> in its
// constructor.
//
// Example usage:
// class MyClass {
//  public:
//   using SharedDeps = service::Dependencies<MyService, MyOtherService>;
//
//   MyClass(Registry<MyClass> registry) : registry_(std::move(registry)) {}
//
//   void Foo() const {
//     registry_.Get<MyService>().DoSomething();
//     registry_.Get<MyOtherService>().DoSomethingElse();
//   }
//
//  private:
//   Registry<MyClass> registry_;
// };
template <typename Owner>
class Registry {
 public:
  explicit Registry(const service_internal::TypePointerMap &type_map) {
    for (const auto &type : service_internal::GetSharedDeps<Owner>()) {
      auto it = type_map.find(type);
      EXPECT(it != type_map.end());
      type_map_[type] = it->second;
    }
  }

  // Allows conversion from one registry to a registry of another type, so long
  // as the "from_registry" contains a superset of types in Owner.
  template <typename T>
  Registry(const Registry<T> &from_registry) {
    for (const auto &type : service_internal::GetSharedDeps<Owner>()) {
      auto it = from_registry.type_map_.find(type);
      EXPECT(it != from_registry.type_map_.end());
      type_map_[type] = it->second;
    }
  }

  // Fetches a reference to the requested service. A compile-time check ensures
  // that the requested service is one that Owner actually depends on.
  //
  // Note that the template type must be the interface, not the implementation,
  // as defined in the DefinitionList.
  template <typename Interface>
  Interface &Get() const {
    static_assert(service_internal::HasSharedDep<Owner, Interface>::value,
                  "Can't fetch a shared service that you don't depend on.");
    return *static_cast<Interface *>(
        type_map_.find(std::type_index(typeid(Interface)))->second.get());
  }
  template <typename Interface>
  std::shared_ptr<Interface> GetShared() const {
    static_assert(service_internal::HasSharedDep<Owner, Interface>::value,
                  "Can't fetch a shared service that you don't depend on.");
    return std::static_pointer_cast<Interface>(
        type_map_.find(std::type_index(typeid(Interface)))->second);
  }

 private:
  service_internal::TypePointerMap type_map_;

  // For the implicit conversion constructor.
  template <typename U>
  friend class Registry;
};

}  // namespace service
}  // namespace ink

#endif  // INK_ENGINE_SERVICE_REGISTRY_H_
