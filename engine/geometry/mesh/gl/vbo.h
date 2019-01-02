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

#ifndef INK_ENGINE_GEOMETRY_MESH_GL_VBO_H_
#define INK_ENGINE_GEOMETRY_MESH_GL_VBO_H_

#include <cstddef>
#include <limits>
#include <vector>

#include "geo/render/ion/gfx/graphicsmanager.h"
#include "ink/engine/gl.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
namespace ink {

// Manages a vertex buffer object, which lives in graphics memory.
// Responsible for copying data to graphics memory.
class VBO {
 public:
  template <typename T>
  VBO(const ion::gfx::GraphicsManagerPtr& gl, const std::vector<T>& elements,
      GLenum usage, GLenum target)
      : gl_(gl), handle_(0), usage_(usage), target_(target) {
    InitBuffer();
    FitExactly(elements);
  }

  template <typename T>
  VBO(ion::gfx::GraphicsManagerPtr gl, const std::vector<T>& elements,
      GLenum usage)
      : VBO(gl, elements, usage, GL_ARRAY_BUFFER) {}

  VBO(ion::gfx::GraphicsManagerPtr gl, size_t capacity_in_bytes, GLenum usage,
      GLenum target);

  VBO(const ion::gfx::GraphicsManagerPtr& gl, size_t capacity_in_bytes,
      GLenum usage)
      : VBO(gl, capacity_in_bytes, usage, GL_ARRAY_BUFFER) {}

  // Disallow copy and assign.
  VBO(const VBO&) = delete;
  VBO& operator=(const VBO&) = delete;

  ~VBO();

  void Bind();
  void Unbind();

  size_t GetSizeInBytes();
  size_t GetCapacityInBytes();

  template <typename T>
  size_t GetTypedSize() {
    return NumBytesToNumType<T>(size_in_bytes_);
  }

  template <typename T>
  size_t GetTypedCapacity() {
    return NumBytesToNumType<T>(capacity_in_bytes_);
  }

  template <typename T>
  size_t GetTypedSizeExact() {
    return NumBytesToNumTypeExact<T>(size_in_bytes_);
  }

  template <typename T>
  size_t GetTypedCapacityExact() {
    return NumBytesToNumTypeExact<T>(capacity_in_bytes_);
  }

  // Deletes everything in the vbo as a side effect.
  void Resize(size_t capacity_in_bytes);

  // Does not affect the capacity of the VBO.
  void RemoveAll() { size_in_bytes_ = 0; }

  template <typename T>
  void GrowCapacityTyped(size_t target_capacity_in_type) {
    GrowCapacity(target_capacity_in_type * sizeof(T));
  }

  void GrowCapacity(size_t target_capacity_in_bytes) {
    // If targetCapacityInBytes is too large then the loop below may never
    // terminate, as vboCapacity will just overflow.
    size_t vbo_capacity = capacity_in_bytes_;
    EXPECT(target_capacity_in_bytes <
           (std::numeric_limits<size_t>::max() >> 1));
    while (vbo_capacity < target_capacity_in_bytes) {
      vbo_capacity *= 2;
    }
    Resize(vbo_capacity);
  }

  template <typename T>
  void FitExactly(const std::vector<T>& elements) {
    EXPECT(!elements.empty());
    size_in_bytes_ = elements.size() * sizeof(T);
    capacity_in_bytes_ = size_in_bytes_;
    BufferData(const_cast<T*>(&elements[0]));
  }

  template <typename T>
  void FitExactly(const T* elements, size_t size_in_bytes) {
    size_in_bytes_ = size_in_bytes;
    capacity_in_bytes_ = size_in_bytes_;
    BufferData(const_cast<void*>(static_cast<const void*>(elements)));
  }

  // Adds all the elements to the VBO.
  //
  // It assumes that if any data has been buffered, that elements is an
  // extension of that data, and we can append any elements beyond the current
  // size, if capacity allows.
  //
  // If capacity does not allow, we will buffer the entire elements list.
  template <typename T>
  void SetData(const std::vector<T>& elements) {
    EXPECT(!elements.empty());
    SetData(elements, GetTypedSizeExact<T>());
  }

 private:
  // Adds all the elements from elements[start] to elements[elements.size() -1]
  //
  // If we do not have sufficient space, we will grow the VBO and buffer all of
  // elements, ignoring start.
  template <typename T>
  void SetData(const std::vector<T>& elements, size_t start) {
    EXPECT(start < elements.size());
    size_t new_bytes_size = (elements.size() - start) * sizeof(T);

    if (!HasCapacity(size_in_bytes_ + new_bytes_size)) {
      new_bytes_size = size_in_bytes_ + new_bytes_size;
      RemoveAll();  // Sets sizeInBytes_ to 0.
      GrowCapacity(new_bytes_size);
      start = 0;
    }

    Bind();
    gl_->BufferSubData(target_, size_in_bytes_, new_bytes_size,
                       &elements[start]);
    Unbind();
    size_in_bytes_ += new_bytes_size;
  }

  bool HasCapacity(size_t target_size) {
    return target_size <= capacity_in_bytes_;
  }

  template <typename T>
  static size_t NumBytesToNumType(size_t num_bytes) {
    return num_bytes / sizeof(T);
  }

  template <typename T>
  static size_t NumBytesToNumTypeExact(size_t num_bytes) {
    EXPECT(num_bytes % sizeof(T) == 0);
    return NumBytesToNumType<T>(num_bytes);
  }

  void InitBuffer();
  void BufferData(void* data);

  ion::gfx::GraphicsManagerPtr gl_;
  GLuint handle_;
  GLenum usage_;
  GLenum target_;
  size_t size_in_bytes_;
  size_t capacity_in_bytes_;

  friend class VBOTestHelper;
};

}  // namespace ink
#endif  // INK_ENGINE_GEOMETRY_MESH_GL_VBO_H_
