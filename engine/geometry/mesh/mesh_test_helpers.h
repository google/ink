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

#ifndef INK_ENGINE_GEOMETRY_MESH_MESH_TEST_HELPERS_H_
#define INK_ENGINE_GEOMETRY_MESH_MESH_TEST_HELPERS_H_

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/mesh/mesh.h"
namespace ink {

// COLOR TOLERANCES:
//
// For data compressed in the format x11a7r6y11g7b6 (compression used by
// ShaderType::ColoredVertShader), each color channel is stored using as few as
// 6 bits of precision (either 6 or 7).  2^6 = 64, so our floating point numbers
// in [0,1] will be rounded to the nearest k/64 for some k.  This means that if
// my input were .50, the output could be as large as .5+ (1/64)/2 = 0.5078125,
// i.e. our error should be at most .0079.
static const float color_tolerance_colored_vert_shader = .01f;

// For the data compressed in the formats x12y12 and x32y32 (compression used by
// ShaderType::SingleColorShader and ShaderType::EraseShader), the color data is
// stored in a single uint32 on the Stroke proto, so we have 8 bits of precision
// per color channel.  2^8 = 256, so our maximum error is (1/256)/2 =
// 0.00390625.
//
// For data stored in the xyrgbauv32 format, we store colors at full precision
// in the vertex.  However, under the current settings (we could change this if
// desired) openctm compresses it down to 8 bits per channel as
// well, see
// third_party/openctm/files/lib/openctm.c&l=1112
static const float color_tolerance8bit_color = .005f;

// Constructs a triangle strip mesh from the given vertices. The vertices are
// expected to alternate between the left and right sides of the strip.
Mesh MakeTriangleStrip(std::vector<Vertex> vertices);

// Constructs a triangle strip in the shape of a ring.
Mesh MakeRingMesh(glm::vec2 center, float inner_radius, float outer_radius,
                  int subdivisions);

// Constructs a triangle strip in the shape of a sine wave.
Mesh MakeSineWaveMesh(glm::vec2 start, float amplitude, float frequency,
                      float length, float width, int subdivisions);

// Given a mesh, returns an equivalent mesh where the new object-matrix is the
// identity but the world coordinates of the vertices remain the same.
Mesh FlattenObjectMatrix(const Mesh& mesh);

// Converts an optimized mesh to a mesh with the equivalent contents.
//
// Warning: Given a Mesh a, and performing the transformation Mesh b =
// optMeshToMesh(OptimizedMesh(a)); may give b.objectMatrix != a.objectMatrix.
// However, a and b will have equivalent world coordinate positions, i.e.
//
// a.objectMatrix*a.verts[i].position == b.objectrixMatrix*b.verts[i].position
//
Mesh OptMeshToMesh(const OptimizedMesh& opt_mesh);

}  // namespace ink
#endif  // INK_ENGINE_GEOMETRY_MESH_MESH_TEST_HELPERS_H_
