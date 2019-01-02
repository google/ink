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

#ifndef INK_ENGINE_SERVICE_DEFINITION_LIST_INTERNAL_H_
#define INK_ENGINE_SERVICE_DEFINITION_LIST_INTERNAL_H_

#include <memory>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>

#include "ink/engine/service/common_internal.h"
#include "ink/engine/service/dependencies.h"
#include "ink/engine/service/registry.h"

namespace ink {
namespace service {
namespace service_internal {

// Type-erased interface that the DefinitionList uses to instantiate services.
class Definition {
 public:
  virtual ~Definition() {}

  virtual std::shared_ptr<void> GetInstance(
      const TypePointerMap &type_map) const = 0;
  virtual TypeIndexSet DirectDependencies() const = 0;
};

// Definition for a service that already existed when definitions were set up.
template <typename Interface>
class ExistingServiceDefinition : public Definition {
 public:
  explicit ExistingServiceDefinition(std::shared_ptr<Interface> instance)
      : instance_(std::move(instance)) {}
  std::shared_ptr<void> GetInstance(
      const TypePointerMap &type_map) const override {
    return instance_;
  }
  TypeIndexSet DirectDependencies() const override { return TypeIndexSet(); }

 private:
  std::shared_ptr<Interface> instance_;
};

// These helper structs allow for compile-time constructor selection.

// Helper for services whose constructors take a Registry, e.g.:
// MyService(Registry<MyService> registry);
template <typename Implementation>
struct RegistryCtorHelper {
  using RegistryType = Registry<Implementation>;

  static_assert(std::is_constructible<Implementation, RegistryType>::value,
                "Implementation is not constructible from Registry.");
  static std::shared_ptr<Implementation> Construct(
      const TypePointerMap &type_map) {
    return std::make_shared<Implementation>(RegistryType(type_map));
  }
};

// Helper for services whose constructors take shared pointers, e.g.:
// MyService(std::shared_ptr<Dep> dep, std::shared_ptr<AnotherDep> another_dep);
template <typename... T>
struct SharedPtrCtorHelper;

template <typename Implementation, typename... SharedDeps>
struct SharedPtrCtorHelper<Implementation, Dependencies<SharedDeps...>> {
  static_assert(
      std::is_constructible<Implementation,
                            std::shared_ptr<SharedDeps>...>::value,
      "Implementation is not constructible; check that its dependencies have "
      "been declared correctly.");
  static std::shared_ptr<Implementation> Construct(
      const TypePointerMap &type_map) {
    return std::make_shared<Implementation>(
        std::static_pointer_cast<SharedDeps>(
            type_map.find(std::type_index(typeid(SharedDeps)))->second)...);
  }
};

// This template class handles the actual construction of services.
template <typename Interface, typename Implementation>
class TypedDefinition : public Definition {
 public:
  using SharedDeps = typename SharedDepsType<Implementation>::type;
  using RegistryType = Registry<Implementation>;

  std::shared_ptr<void> GetInstance(
      const TypePointerMap &type_map) const override {
    // Select the helper type based on what arguments the constructor takes.
    using CtorHelper = typename std::conditional<
        std::is_constructible<Implementation, RegistryType>::value,
        RegistryCtorHelper<Implementation>,
        SharedPtrCtorHelper<Implementation, SharedDeps>>::type;

    // Call the construction method on the helper type.
    return CtorHelper::Construct(type_map);
  }

  TypeIndexSet DirectDependencies() const override {
    return GetSharedDeps<Implementation>();
  }
};

}  // namespace service_internal
}  // namespace service
}  // namespace ink

#endif  // INK_ENGINE_SERVICE_DEFINITION_LIST_INTERNAL_H_
