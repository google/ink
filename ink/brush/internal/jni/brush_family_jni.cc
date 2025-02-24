// Copyright 2024-2025 Google LLC
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

#include <utility>
#include <variant>
#include <vector>

#include "absl/log/absl_check.h"
#include "absl/status/statusor.h"
#include "absl/types/span.h"
#include "ink/brush/brush_coat.h"
#include "ink/brush/brush_family.h"
#include "ink/brush/internal/jni/brush_jni_helper.h"
#include "ink/jni/internal/jni_defines.h"
#include "ink/jni/internal/jni_string_util.h"
#include "ink/jni/internal/jni_throw_util.h"

namespace {

using ::ink::BrushCoat;
using ::ink::BrushFamily;
using ::ink::CastToBrushCoat;
using ::ink::CastToBrushFamily;
using ::ink::jni::JStringView;
using ::ink::jni::ThrowExceptionFromStatus;

}  // namespace

extern "C" {

// Construct a native BrushFamily and return a pointer to it as a long.
JNI_METHOD(brush, BrushFamilyNative, jlong, create)
(JNIEnv* env, jobject object, jlongArray coat_native_pointer_array,
 jstring client_brush_family_id, jboolean use_spring_model_v2) {
  std::vector<BrushCoat> coats;
  const jsize num_coats = env->GetArrayLength(coat_native_pointer_array);
  coats.reserve(num_coats);
  jlong* coat_pointers =
      env->GetLongArrayElements(coat_native_pointer_array, nullptr);
  ABSL_CHECK(coat_pointers != nullptr);
  for (jsize i = 0; i < num_coats; ++i) {
    coats.push_back(CastToBrushCoat(coat_pointers[i]));
  }
  env->ReleaseLongArrayElements(
      coat_native_pointer_array, coat_pointers,
      // No need to copy back the array, which is not modified.
      JNI_ABORT);

  ink::BrushFamily::InputModel input_model = ink::BrushFamily::SpringModelV1();
  if (use_spring_model_v2) {
    input_model = ink::BrushFamily::SpringModelV2();
  }

  absl::StatusOr<ink::BrushFamily> brush_family = ink::BrushFamily::Create(
      coats, JStringView(env, client_brush_family_id).string_view(),
      input_model);
  if (!brush_family.ok()) {
    ThrowExceptionFromStatus(env, brush_family.status());
    return -1;  // Unused return value.
  }

  return reinterpret_cast<jlong>(
      new ink::BrushFamily(*std::move(brush_family)));
}

JNI_METHOD(brush, BrushFamilyNative, void, free)
(JNIEnv* env, jobject object, jlong native_pointer) {
  delete reinterpret_cast<BrushFamily*>(native_pointer);
}

JNI_METHOD(brush, BrushFamilyNative, jstring, getClientBrushFamilyId)
(JNIEnv* env, jobject object, jlong native_pointer) {
  const BrushFamily& brush_family = CastToBrushFamily(native_pointer);
  return env->NewStringUTF(brush_family.GetClientBrushFamilyId().c_str());
}

JNI_METHOD(brush, BrushFamilyNative, jboolean, usesSpringModelV2)
(JNIEnv* env, jobject object, jlong native_pointer) {
  const BrushFamily& brush_family = CastToBrushFamily(native_pointer);
  return std::holds_alternative<BrushFamily::SpringModelV2>(
      brush_family.GetInputModel());
}

}  // extern "C"
