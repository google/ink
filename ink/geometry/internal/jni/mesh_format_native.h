// Copyright 2026 Google LLC
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

#ifndef INK_GEOMETRY_INTERNAL_JNI_MESH_FORMAT_NATIVE_H_
#define INK_GEOMETRY_INTERNAL_JNI_MESH_FORMAT_NATIVE_H_

#include <stdbool.h>
#include <stdint.h>

// C-compatible library header for Kotlin-native bindings.

#ifdef __cplusplus
extern "C" {
#endif

// The native_ptr parameter of these methods contains the raw pointer to a
// C++ MeshFormat object stored in Kotlin on MeshFormat.nativePointer.

// Returns whether this mesh format has the same packed representation and same
// packing scheme as another MeshFormat, such that they can be passed to the
// same shader that accepts packed attribute values. other_native_ptr is the raw
// pointer to a different MeshFormat object.
bool MeshFormatNative_isPackedEquivalent(int64_t native_ptr,
                                         int64_t other_native_ptr);

// Returns whether this mesh formats has the same unpacked representation as
// another MeshFormat. other_native_ptr is the raw pointer to a different
// MeshFormat object.
bool MeshFormatNative_isUnpackedEquivalent(int64_t native_ptr,
                                           int64_t other_native_ptr);

int MeshFormatNative_attributeCount(int64_t native_ptr);

// Frees a Kotlin MeshFormat.nativePointer.
void MeshFormatNative_free(int64_t native_ptr);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // INK_GEOMETRY_INTERNAL_JNI_MESH_FORMAT_NATIVE_H_
