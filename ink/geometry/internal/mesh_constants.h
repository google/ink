// Copyright 2024 Google LLC
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

#ifndef INK_GEOMETRY_INTERNAL_MESH_CONSTANTS_H_
#define INK_GEOMETRY_INTERNAL_MESH_CONSTANTS_H_

namespace ink::mesh_internal {

// The maximum number of attributes that a `Mesh` or `MutableMesh` vertex may
// carry.
inline constexpr int kMaxVertexAttributes = 16;

}  // namespace ink::mesh_internal

#endif  // INK_GEOMETRY_INTERNAL_MESH_CONSTANTS_H_
