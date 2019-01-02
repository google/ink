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

#include "ink/engine/processing/runner/service_definition.h"
#include "ink/engine/processing/runner/task_runner.h"
#include "ink/engine/processing/runner/web_task_runner.h"

namespace ink {

void DefineTaskRunner(service::DefinitionList* definitions) {
  definitions->DefineService<ITaskRunner, WebTaskRunner>();
}

}  // namespace ink
