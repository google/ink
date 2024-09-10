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

#include <string>
#include <utility>
#include <vector>

#include "absl/log/check.h"
#include "absl/status/statusor.h"
#include "absl/types/span.h"
#include "ink/brush/brush_coat.h"
#include "ink/brush/brush_family.h"
#include "ink/brush/internal/jni/brush_jni_helper.h"
#include "ink/jni/internal/jni_defines.h"
#include "ink/jni/internal/jni_throw_util.h"

extern "C" {

// Construct a native BrushFamily and return a pointer to it as a long.
JNI_METHOD(brush, BrushFamily, jlong, nativeCreateBrushFamily)
(JNIEnv* env, jobject thiz, jlongArray coat_native_pointer_array,
 jstring j_uri) {
  std::vector<ink::BrushCoat> coats;
  const jsize num_coats = env->GetArrayLength(coat_native_pointer_array);
  coats.reserve(num_coats);
  jlong* coat_pointers =
      env->GetLongArrayElements(coat_native_pointer_array, nullptr);
  CHECK(coat_pointers);
  for (jsize i = 0; i < num_coats; ++i) {
    coats.push_back(ink::CastToBrushCoat(coat_pointers[i]));
  }
  env->ReleaseLongArrayElements(coat_native_pointer_array, coat_pointers, 0);

  std::string uri = ink::JStringToStdString(env, j_uri);

  absl::StatusOr<ink::BrushFamily> brush_family =
      ink::BrushFamily::Create(coats, uri);
  if (!brush_family.ok()) {
    ink::jni::ThrowExceptionFromStatus(env, brush_family.status());
    return -1;  // Unused return value.
  }

  return reinterpret_cast<jlong>(
      new ink::BrushFamily(*std::move(brush_family)));
}

JNI_METHOD(brush, BrushFamily, void, nativeFreeBrushFamily)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  delete reinterpret_cast<ink::BrushFamily*>(native_pointer);
}

}  // extern "C"
