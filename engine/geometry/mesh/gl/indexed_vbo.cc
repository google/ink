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

#include "ink/engine/geometry/mesh/gl/indexed_vbo.h"

#include "ink/engine/geometry/mesh/gl/vbo.h"
#include "ink/engine/gl.h"
#include "ink/engine/util/dbg/errors.h"
namespace ink {

void IndexedVBO::Bind() {
  index_vbo_.Bind();
  vertex_vbo_.Bind();
}

void IndexedVBO::Unbind() {
  index_vbo_.Unbind();
  vertex_vbo_.Unbind();
}

VBO* IndexedVBO::GetIndices() { return &index_vbo_; }
VBO* IndexedVBO::GetVertices() { return &vertex_vbo_; }
}  // namespace ink
