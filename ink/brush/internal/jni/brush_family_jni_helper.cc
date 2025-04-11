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
#include <vector>

#include "absl/log/absl_check.h"
#include "absl/status/statusor.h"
#include "absl/types/span.h"
#include "ink/brush/brush_coat.h"
#include "ink/brush/brush_family.h"
#include "ink/brush/internal/jni/brush_jni_helper.h"
#include "ink/jni/internal/jni_string_util.h"
#include "ink/jni/internal/jni_throw_util.h"

namespace {

using ::ink::BrushCoat;
using ::ink::BrushFamily;
using ::ink::jni::CastToBrushCoat;
using ::ink::jni::JStringView;
using ::ink::jni::ThrowExceptionFromStatus;

}  // namespace

namespace ink {

jlong CreateBrushFamily(JNIEnv* env, ::ink::BrushFamily::InputModel input_model,
                        jlongArray coat_native_pointer_array,
                        jstring client_brush_family_id) {
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

  absl::StatusOr<BrushFamily> brush_family = BrushFamily::Create(
      coats, JStringView(env, client_brush_family_id).string_view(),
      input_model);
  if (!brush_family.ok()) {
    ThrowExceptionFromStatus(env, brush_family.status());
    return 0;  // Unused return value.
  }

  return reinterpret_cast<jlong>(new BrushFamily(*std::move(brush_family)));
}

}  // namespace ink
