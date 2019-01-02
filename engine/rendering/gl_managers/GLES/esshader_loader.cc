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

#include "ink/engine/rendering/gl_managers/GLES/esshader_loader.h"

#include <cstddef>
#include <string>

#include "third_party/absl/strings/str_cat.h"
#include "third_party/absl/strings/string_view.h"
#include "ink/engine/rendering/gl_managers/bad_gl_handle.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/glerrors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"

namespace ink {

namespace {
bool IsGLES(const ion::gfx::GraphicsManagerPtr& gl) {
  const char* version_string = nullptr;
  version_string =
      reinterpret_cast<const char*>(gl->GetString(GL_SHADING_LANGUAGE_VERSION));
  if (!version_string) {
    SLOG(SLOG_ERROR, "Could not read GL_SHADING_LANGUAGE_VERSION");
    return true;
  }
  std::string glsl_version(version_string);
  bool looks_like_gles = glsl_version.find("GLSL ES") != std::string::npos ||
                         glsl_version.find("ES GLSL") != std::string::npos;
  SLOG(SLOG_GPU_OBJ_CREATION, "Interpreting \"$0\" as a $1 shader interpreter.",
       glsl_version, looks_like_gles ? "GLES" : "non-GLES");
  return looks_like_gles;
}

// Returns true if the fragment shader supports the `highp` precision format for
// floating point types.
bool FragmentShaderSupportsHighpFloat(const ion::gfx::GraphicsManagerPtr& gl) {
  GLint range[2];
  GLint precision;
  gl->GetShaderPrecisionFormat(GL_FRAGMENT_SHADER, GL_HIGH_FLOAT, range,
                               &precision);
  return !(range[0] == 0 && range[1] == 0 && precision == 0);
}

// Returns GLSL source that should be prepended to each shader.
//
// The data pointed to by the returned `string_view` remains valid for the
// lifetime of the program (as it is a string literal).
absl::string_view GetSourceToPrepend(const ion::gfx::GraphicsManagerPtr& gl) {
  // If we are compiling for desktop GL, we add defines so that we can reuse
  // GLES shader source without modification.
  absl::string_view kDesktopGLDefines =
      "#version 120\n"
      "#define lowp\n"
      "#define mediump\n"
      "#define highp\n"
      "#define precision\n"
      "#define INK_MAX_FRAGMENT_FLOAT_PRECISION\n"
      "#line 1\n";

  // We add a define for maximum float precision. GLES support for `highp` in
  // fragment shaders is optional, but we want to use it when available.
  absl::string_view kFragmentShaderPrecisionDefine =
      FragmentShaderSupportsHighpFloat(gl)
          ? "#define INK_MAX_FRAGMENT_FLOAT_PRECISION highp\n"
            "#line 1\n"
          : "#define INK_MAX_FRAGMENT_FLOAT_PRECISION mediump\n"
            "#line 1\n";

  return IsGLES(gl) ? kFragmentShaderPrecisionDefine : kDesktopGLDefines;
}

GLuint BuildShader(const ion::gfx::GraphicsManagerPtr& gl,
                   const std::string& shader_path,
                   const std::string& shader_source, GLenum shader_type) {
  static const absl::string_view kSourceToPrepend = GetSourceToPrepend(gl);
  string src = absl::StrCat(kSourceToPrepend, shader_source);
  const char* cstr = src.c_str();

  GLuint shader = gl->CreateShader(shader_type);
  gl->ShaderSource(shader, 1, &cstr, nullptr);
  gl->CompileShader(shader);

  GLint compile_res;
  gl->GetShaderiv(shader, GL_COMPILE_STATUS, &compile_res);
  if (compile_res != GL_TRUE) {
    GLchar msg[256];
    gl->GetShaderInfoLog(shader, sizeof(msg), nullptr, msg);
    RUNTIME_ERROR("compilation of $0 failed: $1", shader_path,
                  absl::string_view(msg));
  }
  return shader;
}
}  // namespace

namespace gles {

GLuint BuildProgram(const ion::gfx::GraphicsManagerPtr& gl,
                    const std::string& vert_shader_path,
                    const std::string& vert_shader_source,
                    const std::string& frag_shader_path,
                    const std::string& frag_shader_source) {
  const GLuint vert =
      BuildShader(gl, vert_shader_path, vert_shader_source, GL_VERTEX_SHADER);
  const GLuint frag =
      BuildShader(gl, frag_shader_path, frag_shader_source, GL_FRAGMENT_SHADER);

  GLEXPECT_NO_ERROR(gl);
  GLuint program = gl->CreateProgram();
  GLEXPECT(gl, program != 0);

  gl->AttachShader(program, vert);
  gl->AttachShader(program, frag);
  gl->LinkProgram(program);

  GLint link_res;
  gl->GetProgramiv(program, GL_LINK_STATUS, &link_res);
  if (link_res != GL_TRUE) {
    GLchar msg[256];
    gl->GetProgramInfoLog(program, sizeof(msg), nullptr, &msg[0]);
    auto err = gl->GetError();
    RUNTIME_ERROR("while linking $0/$1, error $2: $3\n", vert_shader_path,
                  frag_shader_path, HexStr(err), absl::string_view(msg));
  }
  return program;
}

}  // namespace gles
}  // namespace ink
