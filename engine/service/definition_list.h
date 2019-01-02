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

#ifndef INK_ENGINE_SERVICE_DEFINITION_LIST_H_
#define INK_ENGINE_SERVICE_DEFINITION_LIST_H_

#include <memory>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "third_party/absl/memory/memory.h"
#include "ink/engine/service/common_internal.h"
#include "ink/engine/service/definition_list_internal.h"
#include "ink/engine/service/dependencies.h"
#include "ink/engine/util/dbg/errors.h"

namespace ink {
namespace service {

// This class contains the mapping from interfaces to implementation that is
// used for dependency injection.
class DefinitionList {
 public:
  // Creates a mapping from Interface to Implementation, replacing any existing
  // mapping for Interface.
  //
  // Implementation must declare a public type alias "SharedDeps", which lists
  // its dependencies (see dependencies.h), e.g.:
  // using SharedDeps = service::Dependencies<SomeClass, SomeOtherClass>;
  template <typename Interface, typename Implementation = Interface>
  void DefineService();

  // Creates a mapping from Interface to the given instance, replacing any
  // existing mapping for Interface. This should only be used in cases in which
  // the service is shared with something outside the engine, e.g. the host
  // controller.
  template <typename Interface>
  void DefineExistingService(std::shared_ptr<Interface> existing_service);

  // Returns an instance of the requested type
  // - For mappings created via DefineService(), this creates a new instance,
  //   fetching its dependencies from type_map.
  // - For mappings created via DefineExistingService(), this returns the
  //   existing instance.
  std::shared_ptr<void> GetInstance(
      std::type_index type,
      const service_internal::TypePointerMap &type_map) const;

  // Returns the type indices of all of the defined interface types.
  service_internal::TypeIndexSet GetDefinedTypes() const;

  // Returns the type indices of the direct dependencies of the interface type.
  // For interfaces defined via DefineExistingService(), an empty set will be
  // returned.
  service_internal::TypeIndexSet GetDirectDependencies(
      std::type_index type) const;

 private:
  absl::flat_hash_map<std::type_index,
                      std::unique_ptr<service_internal::Definition>>
      definitions_;
};

template <typename Interface, typename Implementation>
void DefinitionList::DefineService() {
  static_assert(std::is_class<Interface>::value, "Interface must be a class");
  static_assert(!std::is_abstract<Implementation>::value,
                "Implementation must be a concrete type");
  static_assert(std::is_base_of<Interface, Implementation>::value,
                "Implementation is not a subclass of Interface");
  std::type_index type(typeid(Interface));
  definitions_[type] = absl::make_unique<
      service_internal::TypedDefinition<Interface, Implementation>>();
}

template <typename Interface>
void DefinitionList::DefineExistingService(
    std::shared_ptr<Interface> existing_service) {
  static_assert(std::is_class<Interface>::value, "Interface must be a class");
  std::type_index type(typeid(Interface));
  definitions_[type] =
      absl::make_unique<service_internal::ExistingServiceDefinition<Interface>>(
          existing_service);
}

}  // namespace service
}  // namespace ink

#endif  // INK_ENGINE_SERVICE_DEFINITION_LIST_H_
