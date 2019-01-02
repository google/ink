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

#include "ink/engine/geometry/mesh/gl/vbo.h"

#include "ink/engine/gl.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/glerrors.h"
namespace ink {

VBO::~VBO() {
  if (handle_ != 0) {
    gl_->DeleteBuffers(1, &handle_);
    handle_ = 0;
  }
}

void VBO::Bind() { gl_->BindBuffer(target_, handle_); }

void VBO::Unbind() { gl_->BindBuffer(target_, 0); }

size_t VBO::GetSizeInBytes() { return size_in_bytes_; }
size_t VBO::GetCapacityInBytes() { return capacity_in_bytes_; }

VBO::VBO(ion::gfx::GraphicsManagerPtr gl, size_t capacity_in_bytes,
         GLenum usage, GLenum target)
    : gl_(gl), handle_(0), usage_(usage), target_(target) {
  InitBuffer();
  Resize(capacity_in_bytes);
}

void VBO::Resize(size_t capacity_in_bytes) {
  size_in_bytes_ = 0;
  capacity_in_bytes_ = capacity_in_bytes;
  BufferData(nullptr);
}

void VBO::InitBuffer() {
  gl_->GenBuffers(1, &handle_);
  GLASSERT_NO_ERROR(gl_);
  EXPECT(handle_ != 0);
}

void VBO::BufferData(void* data) {
  Bind();
  gl_->BufferData(target_, capacity_in_bytes_, data, usage_);
  Unbind();
}
}  // namespace ink
