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

#include "ink/engine/util/dbg/glerrors_internal.h"

#include <cstdint>

#include "ink/engine/gl.h"
#include "ink/engine/util/dbg/errors_internal.h"

namespace ink {

static std::string CreateGLErrorMsg(uint32_t gl_err_code, const char* func,
                                    const char* file, int line) {
  std::string msg = absl::Substitute("gl error 0x$0", absl::Hex(gl_err_code));
  return CreateRuntimeErrorMsg(msg, func, file, line);
}

static void GLExpectInternal(bool success, uint32_t gl_err_code,
                             const char* func, const char* file, int line) {
  if (!success) {
    Die(CreateGLErrorMsg(gl_err_code, func, file, line));
  }
}

void GLExpectInternal(const ion::gfx::GraphicsManagerPtr& gl, bool success,
                      const char* func, const char* file, int line) {
  auto err = gl->GetError();
  GLExpectInternal(success && err == GL_NO_ERROR, err, func, file, line);
}

void GLExpectNoErrorInternal(const ion::gfx::GraphicsManagerPtr& gl,
                             const char* func, const char* file, int line) {
  auto err = gl->GetError();
  GLExpectInternal(err == GL_NO_ERROR, err, func, file, line);
}
}  // namespace ink
