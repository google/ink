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

#ifndef INK_ENGINE_REALTIME_MODIFIERS_LINE_MODIFIER_FACTORY_H_
#define INK_ENGINE_REALTIME_MODIFIERS_LINE_MODIFIER_FACTORY_H_

#include <memory>

#include "ink/engine/brushes/brushes.h"
#include "ink/engine/realtime/modifiers/line_modifier.h"
#include "ink/engine/service/dependencies.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {

class LineModifierFactory {
 public:
  // Returns a new LineModifier appropriate for the supplied brush_params
  // and color.
  std::unique_ptr<LineModifier> Make(const BrushParams& brush_params,
                                     glm::vec4 rgba);
};

}  // namespace ink

#endif  // INK_ENGINE_REALTIME_MODIFIERS_LINE_MODIFIER_FACTORY_H_
