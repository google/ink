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

#include "ink/engine/service/unchecked_registry.h"

#include <typeindex>

#include "ink/engine/service/common_internal.h"
#include "ink/engine/service/definition_list.h"
#include "ink/engine/util/dbg/errors.h"

namespace ink {
namespace service {

UncheckedRegistry::UncheckedRegistry(
    std::unique_ptr<service::DefinitionList> definition_list)
    : definition_list_(std::move(definition_list)) {
  for (auto type : definition_list_->GetDefinedTypes()) InstantiateType(type);
}

service_internal::TypePointerMap::const_iterator UncheckedRegistry::FindType(
    std::type_index type) const {
  auto it = type_to_impl_.find(type);
  if (it == type_to_impl_.end())
    RUNTIME_ERROR("Type $0 has not been instantiated!", type.name());
  return it;
}

void UncheckedRegistry::InstantiateType(std::type_index type) {
  auto it = type_to_impl_.find(type);
  if (it == type_to_impl_.end()) {
    // The nullptr serves as a sentinel to alert us that the type is currently
    // being constructed, which allows us to detect dependency cycles.
    it = type_to_impl_.emplace(type, nullptr).first;
    ASSERT(it != type_to_impl_.end());

    // Ensure that its dependencies have been instantiated.
    for (const auto &dep_type : definition_list_->GetDirectDependencies(type)) {
      InstantiateType(dep_type);
    }

    // The iterator (it) might have changed, as a recursive step may
    // have caused a rehash. Simply lookup and set the instance here.
    type_to_impl_[type] = definition_list_->GetInstance(type, type_to_impl_);
  } else if (it->second == nullptr) {
    RUNTIME_ERROR("Could not construct $0, circular dependency found.",
                  type.name());
  }
}

}  // namespace service
}  // namespace ink
