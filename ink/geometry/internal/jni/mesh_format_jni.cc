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

#include <jni.h>

#include "ink/geometry/mesh_format.h"
#include "ink/jni/internal/jni_defines.h"

namespace {

using ::ink::MeshFormat;

MeshFormat* GetMeshFormat(jlong raw_ptr) {
  return reinterpret_cast<MeshFormat*>(raw_ptr);
}

}  // namespace

extern "C" {

JNI_METHOD(geometry, MeshFormat, jboolean, nativeIsPackedEquivalent)
(JNIEnv* env, jobject obj, jlong first_raw_ptr, jlong second_raw_ptr) {
  const MeshFormat* first = GetMeshFormat(first_raw_ptr);
  const MeshFormat* second = GetMeshFormat(second_raw_ptr);
  return MeshFormat::IsPackedEquivalent(*first, *second);
}

JNI_METHOD(geometry, MeshFormat, jboolean, nativeIsUnpackedEquivalent)
(JNIEnv* env, jobject obj, jlong first_raw_ptr, jlong second_raw_ptr) {
  const MeshFormat* first = GetMeshFormat(first_raw_ptr);
  const MeshFormat* second = GetMeshFormat(second_raw_ptr);
  return MeshFormat::IsUnpackedEquivalent(*first, *second);
}

JNI_METHOD(geometry, MeshFormat, void, nativeFree)
(JNIEnv* env, jobject obj, jlong raw_ptr) { delete GetMeshFormat(raw_ptr); }

}  // extern "C"
