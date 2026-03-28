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

#include "ink/geometry/internal/jni/mesh_format_native.h"

#include <cstdint>

#include "ink/geometry/internal/jni/mesh_format_native_helper.h"
#include "ink/geometry/mesh_format.h"

using ::ink::MeshFormat;
using ::ink::native::CastToMeshFormat;
using ::ink::native::DeleteNativeMeshFormat;

extern "C" {

bool MeshFormatNative_isPackedEquivalent(int64_t native_ptr,
                                         int64_t other_native_ptr) {
  return MeshFormat::IsPackedEquivalent(CastToMeshFormat(native_ptr),
                                        CastToMeshFormat(other_native_ptr));
}

bool MeshFormatNative_isUnpackedEquivalent(int64_t native_ptr,
                                           int64_t other_native_ptr) {
  return MeshFormat::IsUnpackedEquivalent(CastToMeshFormat(native_ptr),
                                          CastToMeshFormat(other_native_ptr));
}

int MeshFormatNative_attributeCount(int64_t native_ptr) {
  return CastToMeshFormat(native_ptr).Attributes().size();
}

void MeshFormatNative_free(int64_t native_ptr) {
  DeleteNativeMeshFormat(native_ptr);
}

}  // extern "C"
