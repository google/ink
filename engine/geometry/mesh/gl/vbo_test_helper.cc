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

#include "ink/engine/geometry/mesh/gl/vbo_test_helper.h"
namespace ink {

VBOTestHelper::VBOTestHelper(ion::gfx::GraphicsManagerPtr gl)
    : gl_(gl),
      wrapper_(swiftshader_wrapper::LibWrapper::Open("libGLESv2.so.2")),
      glm_map_buffer_range_(reinterpret_cast<GlMapBufferRangeFn>(
          wrapper_->Get("glMapBufferRange"))),
      gl_unmap_buffer_(
          reinterpret_cast<GlUnmapBufferFn>(wrapper_->Get("glUnmapBuffer"))) {
  CHECK(glm_map_buffer_range_);
  CHECK(gl_unmap_buffer_);
}

std::vector<float> VBOTestHelper::ReadBufferAsFloats(VBO* vbo) {
  return ReadBufferAsType<float>(vbo);
}
}  // namespace ink
