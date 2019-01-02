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

#include "ink/engine/scene/default_services.h"
#include "ink/public/contrib/extensions/extension_points.h"

namespace ink {
namespace extensions {

void PreConstruct(int argc, void** argp) {}

std::unique_ptr<service::DefinitionList> GetServiceDefinitions() {
  return ink::DefaultServiceDefinitions();
}

void PostConstruct(SEngine* sengine) {}

util::unique_void_ptr Run(SEngine* sengine, void* arg) { return nullptr; }

}  // namespace extensions
}  // namespace ink
