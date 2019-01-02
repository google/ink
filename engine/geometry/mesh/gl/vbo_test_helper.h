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

#ifndef INK_ENGINE_GEOMETRY_MESH_GL_VBO_TEST_HELPER_H_
#define INK_ENGINE_GEOMETRY_MESH_GL_VBO_TEST_HELPER_H_

#include <memory>
#include <vector>

#include "geo/render/ion/gfx/graphicsmanager.h"
#include "ink/engine/geometry/mesh/gl/vbo.h"
#include "ink/engine/gl.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/glerrors.h"
#include "ink/engine/util/dbg/log.h"
#include "third_party/swiftshader/google/lib_wrapper.h"

namespace ink {

typedef void* (*GlMapBufferRangeFn)(GLenum, GLintptr, GLsizeiptr, GLbitfield);
typedef void (*GlUnmapBufferFn)(GLenum);

class VBOTestHelper {
 public:
  explicit VBOTestHelper(ion::gfx::GraphicsManagerPtr gl);
  ~VBOTestHelper() {}

  std::vector<float> ReadBufferAsFloats(VBO* vbo);

  // T must be copyable.
  template <typename T>
  std::vector<T> ReadBufferAsType(VBO* vbo) {
    GLEXPECT_NO_ERROR(gl_);
    vbo->Bind();
    GLEXPECT_NO_ERROR(gl_);
    SLOG(SLOG_INFO,
         "handle: $0 glMapBufferRange"  // GL_OK
         "(target:$1 offset:$2 capacity(byte): $3)",
         vbo->handle_, vbo->target_, 0, vbo->capacity_in_bytes_);
    void* gpu_data = glMapBufferRange_(vbo->target_, 0, vbo->capacity_in_bytes_,
                                       GL_MAP_READ_BIT);
    T* gpu_data_typed = reinterpret_cast<T*>(gpu_data);
    std::vector<T> data;
    for (size_t i = 0; i < vbo->GetTypedSize<T>(); i++) {
      data.push_back(*(gpu_data_typed++));
    }
    glUnmapBuffer_(vbo->target_);
    GLEXPECT_NO_ERROR(gl_);
    vbo->Unbind();
    GLEXPECT_NO_ERROR(gl_);
    return data;
  }

 private:
  ion::gfx::GraphicsManagerPtr gl_;
  // VBOTestHelper gets the implementation of glMapBufferRange and glUnmapBuffer
  // from SwiftShader's GLES dynamic library. When VBOTestHelper is destroyed,
  // the LibWrapper's destructor releases those functions.
  std::unique_ptr<swiftshader_wrapper::LibWrapper> wrapper_;
  GlMapBufferRangeFn glMapBufferRange_;
  GlUnmapBufferFn glUnmapBuffer_;
};

}  // namespace ink
#endif  // INK_ENGINE_GEOMETRY_MESH_GL_VBO_TEST_HELPER_H_
