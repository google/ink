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

#ifndef INK_ENGINE_REALTIME_MOCK_ELEMENT_MANIPULATION_TOOL_RENDERER_H_
#define INK_ENGINE_REALTIME_MOCK_ELEMENT_MANIPULATION_TOOL_RENDERER_H_

#include "testing/base/public/gmock.h"
#include "ink/engine/realtime/element_manipulation_tool_renderer.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {
namespace tools {

class MockElementManipulationToolRenderer
    : public ElementManipulationToolRendererInterface {
 public:
  MOCK_CONST_METHOD3(Draw, void(const Camera &cam, FrameTimeS draw_time,
                                glm::mat4 transform));
  MOCK_METHOD5(Update,
               void(const Camera &cam, FrameTimeS draw_time, Rect element_mbr,
                    RotRect region, glm::mat4 transform));
  MOCK_METHOD1(Enable, void(bool enabled));
  MOCK_METHOD0(Synchronize, void(void));
  MOCK_METHOD4(SetElements,
               void(const Camera &cam, const std::vector<ElementId> &elements,
                    Rect element_mbr, RotRect region));
};

}  // namespace tools
}  // namespace ink

#endif  // INK_ENGINE_REALTIME_MOCK_ELEMENT_MANIPULATION_TOOL_RENDERER_H_
