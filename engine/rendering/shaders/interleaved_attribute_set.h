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

#ifndef INK_ENGINE_RENDERING_SHADERS_INTERLEAVED_ATTRIBUTE_SET_H_
#define INK_ENGINE_RENDERING_SHADERS_INTERLEAVED_ATTRIBUTE_SET_H_

#include <cstddef>
#include <map>
#include <string>
#include <vector>

#include "geo/render/ion/gfx/graphicsmanager.h"
#include "ink/engine/geometry/mesh/vertex_types.h"
#include "ink/engine/gl.h"
#include "ink/engine/util/dbg/errors.h"

namespace ink {

// Holds the memory layout info used by GL to extract an attribute from an array
// of raw bytes when converting to an array of gl vertices in
// glVertexAttribPointer.
struct ShaderAttribute {
  // Must match the attribute name in open gl shader.
  std::string name;

  // Memory used for this attribute per gl vertex in raw byte array.
  size_t size_in_bytes;

  GLenum type;

  // ex sizeof(float)
  size_t size_of_type;

  // Distance from start of each gl vertex to this attribute in raw byte array.
  GLuint offset_in_bytes;

  bool normalize;

  // The result from glGetAttributeLocation, set in "loadAttributes."  Has
  // value "kBadGlHandle" (see bad_gl_handle.h) until bound, or if binding
  // results in an error.
  GLuint gl_handle;

  ShaderAttribute(std::string name, size_t size_in_bytes,
                  GLint offset_in_bytes);

  ShaderAttribute(std::string name, size_t size_in_bytes, GLint offset_in_bytes,
                  GLenum type, size_t size_of_type, bool normalize);

  size_t Size() const { return size_in_bytes / size_of_type; }
};

// Specifies memory layout pattern to convert an array of raw bytes
// into an array of gl vertices (per attribute info and stride per gl vertex).
// Provides wrappers around gl calls managing attributes.
class InterleavedAttributeSet {
 public:
  explicit InterleavedAttributeSet(ion::gfx::GraphicsManagerPtr gl,
                                   size_t stride_in_bytes);
  void AddAttribute(std::string name, size_t size_in_bytes,
                    size_t offset_in_bytes);
  void AddAttribute(ShaderAttribute attr);

  // Invariant: "args" must be of even length, alternating between strings
  // (attribute names) and size_ts (the size of said attribute).
  //
  // Creates an InterleavedAttributeSet with the list of attributes, assumed
  // to be laid out in sequence in memory, and with a total stride length equal
  // to the sum of the attribute sizes.
  //
  // Example:
  // createPacked("scalarAttr", sizeof(float), "vec4attr", 4*sizeof(float))
  //
  // Will create a format with a stride of 20 bytes (5 floats) and two
  // attributes, the first at offset 0 and of size 4 bytes, and the second at an
  // offset of 4 bytes with a size of 16 bytes.
  template <typename... Args>
  static InterleavedAttributeSet CreatePacked(
      const ion::gfx::GraphicsManagerPtr& gl, std::string attribute_name,
      size_t attribute_size_in_bytes, Args... args) {
    InterleavedAttributeSet ans(gl, 0);
    CreatePackedHelper(&ans, attribute_name, attribute_size_in_bytes, args...);
    return ans;
  }

  // Use this layout info to draw the byte array at "dataSource.ptr()".
  // Will cause an error if "dataSource" is empty.
  void Bind(const PackedVertList& data_source) const;

  // Use this layout info to draw the byte array starting at "dataSource[0]".
  template <typename T>
  void Bind(const std::vector<T>& data_source) const {
    EXPECT(data_source.size() > 0);
    Bind(reinterpret_cast<const void*>(&data_source[0]));
  }

  // Use layout info for drawing the currently bound byte array in GPU memory.
  void BindVBO() const;

  // Caller must call glUseProgram before calling use!
  void Use() const;

  void Unuse() const;

  // Must be called before any gl calls (i.e. "use" or "bind*").
  // Sets "glHandle" on each ShaderAttribute.
  void LoadAttributes(GLuint program);

  // Alternative to loadAttributes above for when the attributes have already
  // been loaded and the handles are available.
  void CopyAttrHandles(const std::map<std::string, GLuint>& gl_attr_handles);

  size_t GetStrideInBytes() const { return stride_in_bytes_; }

 private:
  void Bind(const void* data_source) const;

  template <typename... Args>
  static void CreatePackedHelper(InterleavedAttributeSet* result,
                                 std::string attribute_name,
                                 size_t attribute_size_in_bytes, Args... args) {
    result->AddAttribute(attribute_name, attribute_size_in_bytes,
                         result->stride_in_bytes_);
    result->stride_in_bytes_ += attribute_size_in_bytes;
    CreatePackedHelper(result, args...);
  }

  static void CreatePackedHelper(InterleavedAttributeSet* result) {}

  // GL function call wrapper.
  ion::gfx::GraphicsManagerPtr gl_;

  // The number of bytes used to represent each gl vertex in the raw byte array.
  size_t stride_in_bytes_;

  // The attributes needed to draw each gl vertex and their memory layout in the
  // raw byte array.
  std::vector<ShaderAttribute> attributes_;

  friend class InterleavedAttributeSetTestHelper;
};

}  // namespace ink
#endif  // INK_ENGINE_RENDERING_SHADERS_INTERLEAVED_ATTRIBUTE_SET_H_
