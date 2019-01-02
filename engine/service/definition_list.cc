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

#include "ink/engine/service/definition_list.h"

#include <memory>
#include <typeindex>

#include "ink/engine/service/common_internal.h"
#include "ink/engine/util/dbg/errors.h"

namespace ink {
namespace service {

std::shared_ptr<void> DefinitionList::GetInstance(
    std::type_index type,
    const service_internal::TypePointerMap &type_map) const {
  auto definition_it = definitions_.find(type);
  if (definition_it != definitions_.end()) {
    for (const auto dep_type : definition_it->second->DirectDependencies()) {
      auto dep_it = type_map.find(dep_type);
      if (dep_it == type_map.end() || dep_it->second == nullptr) {
        RUNTIME_ERROR("Cannot instantiate service $0: unmet dependency: $1.",
                      type.name(), dep_type.name());
      }
    }
    return definition_it->second->GetInstance(type_map);
  }

  RUNTIME_ERROR("Service $0 is not defined", type.name());
}

service_internal::TypeIndexSet DefinitionList::GetDefinedTypes() const {
  service_internal::TypeIndexSet types;
  types.reserve(definitions_.size());
  for (const auto &pair : definitions_) types.insert(pair.first);
  return types;
}

service_internal::TypeIndexSet DefinitionList::GetDirectDependencies(
    std::type_index type) const {
  auto definition_it = definitions_.find(type);
  if (definition_it != definitions_.end())
    return definition_it->second->DirectDependencies();

  RUNTIME_ERROR("Service $0 is not defined", type.name());
}

}  // namespace service
}  // namespace ink
