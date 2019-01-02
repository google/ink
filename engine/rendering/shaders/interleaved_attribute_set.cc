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

#include "ink/engine/rendering/shaders/interleaved_attribute_set.h"

#include "ink/engine/rendering/gl_managers/bad_gl_handle.h"
#include "ink/engine/rendering/shaders/shader.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/glerrors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"

namespace ink {

using ink::kBadGLHandle;

ShaderAttribute::ShaderAttribute(std::string name, size_t size_in_bytes,
                                 GLint offset_in_bytes, GLenum type,
                                 size_t size_of_type, bool normalize)
    : name(name),
      size_in_bytes(size_in_bytes),
      type(type),
      size_of_type(size_of_type),
      offset_in_bytes(offset_in_bytes),
      normalize(normalize),
      gl_handle(kBadGLHandle) {
  EXPECT(offset_in_bytes >= 0);
  EXPECT(size_in_bytes > 0);
  EXPECT(size_in_bytes % size_of_type == 0);
}

ShaderAttribute::ShaderAttribute(std::string name, size_t size_in_bytes,
                                 GLint offset_in_bytes)
    : ShaderAttribute(name, size_in_bytes, offset_in_bytes, GL_FLOAT,
                      sizeof(float), false) {}

InterleavedAttributeSet::InterleavedAttributeSet(
    ion::gfx::GraphicsManagerPtr gl, size_t stride_in_bytes)
    : gl_(gl), stride_in_bytes_(stride_in_bytes) {}

void InterleavedAttributeSet::AddAttribute(std::string name,
                                           size_t size_in_bytes,
                                           size_t offset_in_bytes) {
  attributes_.push_back(ShaderAttribute(name, size_in_bytes, offset_in_bytes));
}

void InterleavedAttributeSet::AddAttribute(ShaderAttribute attr) {
  attributes_.emplace_back(std::move(attr));
}

void InterleavedAttributeSet::Use() const {
  for (const ShaderAttribute& attr : attributes_) {
    ASSERT(attr.gl_handle != kBadGLHandle);
    gl_->EnableVertexAttribArray(attr.gl_handle);
  }
  GLASSERT_NO_ERROR(gl_);
}

void InterleavedAttributeSet::Unuse() const {
  for (const ShaderAttribute& attr : attributes_) {
    ASSERT(attr.gl_handle != kBadGLHandle);
    gl_->DisableVertexAttribArray(attr.gl_handle);
  }
  GLASSERT_NO_ERROR(gl_);
}

void InterleavedAttributeSet::LoadAttributes(GLuint program) {
  GLEXPECT(gl_, program != kBadGLHandle);
  for (ShaderAttribute& attr : attributes_) {
    SLOG(SLOG_GL_STATE, "Adding attribute $0", attr.name);
    attr.gl_handle = gl_->GetAttribLocation(program, attr.name.c_str());
    GLEXPECT(gl_, attr.gl_handle != kBadGLHandle);
  }
  GLASSERT_NO_ERROR(gl_);
}

void InterleavedAttributeSet::CopyAttrHandles(
    const std::map<std::string, GLuint>& gl_attr_handles) {
  for (ShaderAttribute& attr : attributes_) {
    attr.gl_handle = gl_attr_handles.at(attr.name);
  }
}

void InterleavedAttributeSet::Bind(
    const PackedVertList& packed_vert_list) const {
  Bind(packed_vert_list.Ptr());
}

void InterleavedAttributeSet::BindVBO() const {
  Bind(reinterpret_cast<const void*>(0));
}

void InterleavedAttributeSet::Bind(const void* data_source) const {
  const char* data_source_as_bytes = reinterpret_cast<const char*>(data_source);
  for (const ShaderAttribute& attr : attributes_) {
    const void* data_source_with_offset = reinterpret_cast<const void*>(
        data_source_as_bytes + attr.offset_in_bytes);
#if INK_DEBUG && !(defined(__asmjs__) || defined(__wasm__))
    // glGetVertexAttribiv is not supported in emscripten on client-side arrays
    // even with GLES2 emulation enabled.
    // https://github.com/kripken/emscripten/blob/master/src/library_gl.js#L1505
    GLint has_been_enabled = 0;
    gl_->GetVertexAttribiv(attr.gl_handle, GL_VERTEX_ATTRIB_ARRAY_ENABLED,
                           &has_been_enabled);
    ASSERT(has_been_enabled == 1);
    // Log the gl call
    SLOG(SLOG_GL_STATE,
         "For shader $0, Args: gl handle: $1, count per vertex: $2, type "
         "of data $3, data normalized $4, strideInBytes $5, ptr location "
         "$6",
         attr.name, attr.gl_handle, attr.Size(), attr.type, attr.normalize,
         GetStrideInBytes(), AddressStr(data_source_with_offset));
#endif
    gl_->VertexAttribPointer(attr.gl_handle, attr.Size(), attr.type,
                             static_cast<GLboolean>(attr.normalize),
                             GetStrideInBytes(), data_source_with_offset);
  }
  GLASSERT_NO_ERROR(gl_);
}
}  // namespace ink
