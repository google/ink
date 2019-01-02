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

#ifndef INK_ENGINE_GEOMETRY_MESH_MESH_H_
#define INK_ENGINE_GEOMETRY_MESH_MESH_H_

#include <cstdint>
#include <memory>
#include <vector>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/mesh/shader_type.h"
#include "ink/engine/geometry/mesh/vertex.h"
#include "ink/engine/geometry/mesh/vertex_types.h"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/geometry/primitives/triangle.h"
#include "ink/engine/rendering/gl_managers/texture_info.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/time/time_types.h"
#include "ink/engine/util/unique_void_ptr.h"

namespace ink {

// Class to encapsulate various metadata regarding a Mesh's Shader.
//
// ShaderMetadata is readonly and cannot be created directly; use one of the
// static functions below to create a new ShaderMetadata.
//
// NOTE: ShaderMetadata is very loosely connected (you might even say
//       unconnected) with the shader that is actually used at render-time.
//       MeshRenderer::Draw uses complicated logic based on the ShaderMetadata
//       to determine the actual shader used.
class ShaderMetadata {
 public:
  // Creates a new ShaderMetadata for a shader that renders with the
  // default shader. (Probably renders with VertColoredShader.)
  static ShaderMetadata Default() { return ShaderMetadata(); }

  // Create a new ShaderMetadata for a shader that is animated. init_time is
  // used as the start time for animations parameterised by time.
  static ShaderMetadata Animated(FrameTimeS init_time);

  // Creates a new ShaderMetadata for a shader that renders animating Particles.
  // init_time is the start time for the animation. If cycling is true, then the
  // animation may never terminate because it cycles.
  static ShaderMetadata Particle(FrameTimeS init_time, bool cycling);

  // Creates a new ShaderMetadata for the Eraser.
  static ShaderMetadata Eraser();

  // Indicates that the shader is animated (but not a particle animation).
  bool IsAnimated() const { return is_animated_; }

  // Indicates that the shader animation may never terminate.
  bool IsCycling() const { return is_cycling_; }

  // Indicates that the shader creates particles that may animate.
  bool IsParticle() const { return is_particle_; }

  // For an animated (IsAnimated() == true || IsParticle() == true), returns the
  // start time for the animation.
  FrameTimeS InitTime() const { return init_time_; }

  bool IsEraser() const { return is_eraser_; }

 private:
  ShaderMetadata();
  bool is_particle_;
  bool is_animated_;
  bool is_cycling_;
  FrameTimeS init_time_;
  bool is_eraser_;
};

// Value type to represent a simple (non-optimized) triangle mesh.
// - A mesh is an ordered collection of 'Vertex' structs. The order is defined
//   naturally by the vector, or if idx is present, indirectly by the idx
//   vector.
// - A mesh may be indexed and it may have a texture.
// - A mesh will have a translation matrix (defaults to Identity)
//   and shader metadata (defaults to Blank()).
// - A mesh may have a unique pointer to some backend blob.
//
// While the invariants of this struct may be public, directly manipulating them
// is courting danger since the invariants are not well-documented. Prefer to
// use the methods provided and the various helper functions in shape_helpers.
struct Mesh {
  // The vertices of the mesh. A vertex may be used more (or less) than once
  // depending on the contents of the idx array. The "order" of the verts
  // depends on the contents of the idx array.  If the idx array is empty, then
  // the order is defined by the vertex array, itself.
  std::vector<Vertex> verts;

  // An array of indices into the 'verts' vector. If idx.size() > 0, then it
  // defines the size of the mesh and the order of the vertices in the
  // mesh. Vertices may be reused (as in a rectangle or triangle strip).
  std::vector<uint16_t> idx;

  // Index of vertices which are combined due to their spatial proximity.
  // Used primarily by the ColorLinearizer and is important mostly for the
  // watercolor brush.
  //
  std::vector<uint16_t> combined_idx;

  // The texture used to display all of the triangles in the mesh. The exact
  // texture coordinates are found in the verts.
  std::unique_ptr<TextureInfo> texture;

  // A translation matrix for the Mesh. Defaults to the Identity matrix.
  glm::mat4 object_matrix{1};

  ShaderMetadata shader_metadata = ShaderMetadata::Default();

  // A blob that can be passed to the backend. The caller should make
  // no assumptions about the format or contents of this data.
  util::unique_void_ptr backend_vert_data;

  // Clears the vertex data from the mesh. This only includes the verts, idx,
  // combined_idx, and backend_vert_data. Other fields are left unchanged and
  // must be cleared manually if desired.
  //
  void Clear();

  // Appends the vertices of the other mesh to this mesh, respecting the
  // translation matrix and idx vector of the other matrix in the process.
  //
  // NOTE: it is a runtime error if either Mesh is empty. It is a runtime error
  //       if this.idx.empty() != other.idx.empty().
  void Append(const Mesh& other);

  // Converts to non-indexed vertex representation (with repeat vertices).
  void Deindex();

  // This generates a simple index with no deduping of vertices.
  void GenIndex();

  // Re-orders the indices within each triangle such that its points are
  // oriented counter-clockwise (i.e. its signed area is non-negative). This
  // takes linear time over the number of triangles in the mesh.
  void NormalizeTriangleOrientation();

  // Returns the number of triangles in the mesh.
  int NumberOfTriangles() const {
    ASSERT(idx.size() % 3 == 0);
    return idx.size() / 3;
  }

  // Returns a const reference to the vertex that corresponds to the
  // (vertex_index)th position in the (triangle_index)th triangle. Note that a
  // single vertex may belong to any number of triangles.
  const Vertex& GetVertex(int triangle_index, int vertex_index) const {
    ASSERT(0 <= triangle_index && triangle_index < NumberOfTriangles() &&
           0 <= vertex_index && vertex_index < 3);
    return verts[idx[3 * triangle_index + vertex_index]];
  }

  // Returns the geometric triangle at the given index.
  geometry::Triangle GetTriangle(int triangle_index) const {
    return {GetVertex(triangle_index, 0).position,
            GetVertex(triangle_index, 1).position,
            GetVertex(triangle_index, 2).position};
  }

  // Translates from object coords to world coords.
  // (Position based, translations matter)
  glm::vec2 ObjectPosToWorld(const glm::vec2& object_pos) const;

  Mesh();
  explicit Mesh(const std::vector<Vertex>& verts);
  ~Mesh();

  // NOTE: This will create a new TextureInfo for the new Mesh, and that
  // the new Mesh will not have a ptr to other.backend_vert_data;
  Mesh(const Mesh& other);

  // NOTE: This will create a new TextureInfo and backend_vert_data will be
  // reset and will not take ownership of other.backend_vert_data.
  Mesh& operator=(const Mesh& other);
};

struct OptimizedMesh {
  const ShaderType type;
  std::vector<uint16_t> idx;
  PackedVertList verts;
  std::unique_ptr<TextureInfo> texture;

  // object->world
  glm::mat4 object_matrix{1};

  // Bounds rectangle, in object coords, encompassing all vertices.
  Rect mbr;

  // Base color for a single-colored mesh.
  const glm::vec4 color{0, 0, 0, 0};
  // The base color is first multiplied component-wise by the
  // mul vec4, then added component-wise to the add vec4.
  glm::vec4 mul_color_modifier{1, 1, 1, 1};
  glm::vec4 add_color_modifier{0, 0, 0, 0};

  util::unique_void_ptr backend_vert_data;

 public:
  // Constructs an OptimizedMesh from the given Mesh, packing the vertices
  // according the shader type. The texture info will be copied, and the color
  // will be taken from the first vertex in the given mesh.
  // NOTE: The vertices' positions may be re-scaled as part of the packing
  // process, and as such the object matrix may change.
  // NOTE: The indices of the constructed OptimizedMesh will be normalized,
  // similar to Mesh::NormalizeTriangleOrientation().
  // WARNING: Attempting to construct an OptimizedMesh from an empty Mesh
  // results in a run-time error.
  OptimizedMesh(ShaderType type, const Mesh& mesh);

  // Constructs an OptimizedMesh, as above, except that the given envelope is
  // used for rescaling and packing the vertices.
  // WARNING: This is intended as a performance enhancement for cases in which
  // the envelope of the mesh is already known. Specifiying an envelope that is
  // too large or too small may result in data loss during vertex packing.
  OptimizedMesh(ShaderType type, const Mesh& mesh, Rect envelope);

  OptimizedMesh(const OptimizedMesh& other);
  ~OptimizedMesh();

  void Validate() const;

  // Clears the Verts only in the idx / verts fields (backend_vert_data GPU data
  // is left untouched). This means that ToMesh() will no longer be available.
  void ClearCpuMemoryVerts();

  Mesh ToMesh() const;

  static VertFormat VertexFormat(ShaderType shader_type);

  // Convenience method to retrieve this mesh's current world bounds.
  Rect WorldBounds() const;

 private:
  OptimizedMesh& operator=(const OptimizedMesh& other) = delete;
};

}  // namespace ink
#endif  // INK_ENGINE_GEOMETRY_MESH_MESH_H_
