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

#include <algorithm>
#include <limits>
#include <utility>
#include <variant>
#include <vector>

#include "absl/functional/overload.h"
#include "absl/log/absl_check.h"
#include "absl/status/statusor.h"
#include "absl/types/span.h"
#include "ink/brush/brush_coat.h"
#include "ink/brush/brush_family.h"
#include "ink/brush/internal/jni/brush_jni_helper.h"
#include "ink/jni/internal/jni_defines.h"
#include "ink/jni/internal/jni_string_util.h"
#include "ink/jni/internal/jni_throw_util.h"
#include "ink/types/duration.h"

namespace {

using ::ink::BrushCoat;
using ::ink::BrushFamily;
using ::ink::Duration32;
using ::ink::jni::CastToBrushCoat;
using ::ink::jni::CastToBrushFamily;
using ::ink::jni::CastToInputModel;
using ::ink::jni::DeleteNativeBrushFamily;
using ::ink::jni::DeleteNativeInputModel;
using ::ink::jni::JStringToStdString;
using ::ink::jni::NewNativeBrushCoat;
using ::ink::jni::NewNativeBrushFamily;
using ::ink::jni::NewNativeInputModel;
using ::ink::jni::ThrowExceptionFromStatus;

// 0 is reserved for internal use.
constexpr jint kSpringModel = 1;
// 2 is reserved (was previously the experimental "raw position" model).
constexpr jint kExperimentalNaiveModel = 3;
constexpr jint kSlidingWindowModel = 4;

BrushFamily::InputModel TypeToInputModel(jint input_model_value) {
  switch (input_model_value) {
    case kSpringModel:
      return BrushFamily::SpringModel();
    case kExperimentalNaiveModel:
      return BrushFamily::ExperimentalNaiveModel();
    case kSlidingWindowModel:
      return BrushFamily::SlidingWindowModel();
    default:
      ABSL_CHECK(false) << "Unknown input model value: " << input_model_value;
  }
}

jint InputModelType(const BrushFamily::InputModel& input_model) {
  constexpr auto visitor = absl::Overload{
      [](const BrushFamily::SpringModel&) { return kSpringModel; },
      [](const BrushFamily::ExperimentalNaiveModel&) {
        return kExperimentalNaiveModel;
      },
      [](const BrushFamily::SlidingWindowModel&) {
        return kSlidingWindowModel;
      },
  };
  return std::visit(visitor, input_model);
}

}  // namespace

extern "C" {

// Construct a native BrushFamily and return a pointer to it as a long.
JNI_METHOD(brush, BrushFamilyNative, jlong,
           create)(JNIEnv* env, jobject object,
                   jlongArray coat_native_pointer_array,
                   jlong input_model_pointer, jstring client_brush_family_id,
                   jstring developer_comment) {
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
      coats, CastToInputModel(input_model_pointer),
      BrushFamily::Metadata{
          .client_brush_family_id =
              JStringToStdString(env, client_brush_family_id),
          .developer_comment = JStringToStdString(env, developer_comment),
      });
  if (!brush_family.ok()) {
    ThrowExceptionFromStatus(env, brush_family.status());
    return 0;  // Unused return value.
  }

  return NewNativeBrushFamily(*std::move(brush_family));
}

JNI_METHOD(brush, BrushFamilyNative, void, free)
(JNIEnv* env, jobject object, jlong native_pointer) {
  DeleteNativeBrushFamily(native_pointer);
}

JNI_METHOD(brush, BrushFamilyNative, jstring, getClientBrushFamilyId)
(JNIEnv* env, jobject object, jlong native_pointer) {
  const BrushFamily& brush_family = CastToBrushFamily(native_pointer);
  return env->NewStringUTF(
      brush_family.GetMetadata().client_brush_family_id.c_str());
}

JNI_METHOD(brush, BrushFamilyNative, jstring, getDeveloperComment)
(JNIEnv* env, jobject object, jlong native_pointer) {
  const BrushFamily& brush_family = CastToBrushFamily(native_pointer);
  return env->NewStringUTF(
      brush_family.GetMetadata().developer_comment.c_str());
}

JNI_METHOD(brush, BrushFamilyNative, jlong, getBrushCoatCount)
(JNIEnv* env, jobject object, jlong native_pointer) {
  const BrushFamily& brush_family = CastToBrushFamily(native_pointer);
  return brush_family.GetCoats().size();
}

JNI_METHOD(brush, BrushFamilyNative, jlong, newCopyOfBrushCoat)
(JNIEnv* env, jobject object, jlong native_pointer, jint index) {
  const BrushFamily& brush_family = CastToBrushFamily(native_pointer);
  return NewNativeBrushCoat(brush_family.GetCoats()[index]);
}

JNI_METHOD(brush, BrushFamilyNative, jlong, newCopyOfInputModel)
(JNIEnv* env, jobject object, jlong native_pointer, jint index) {
  const BrushFamily& brush_family = CastToBrushFamily(native_pointer);
  return NewNativeInputModel(brush_family.GetInputModel());
}

JNI_METHOD(brush, InputModelNative, jlong, createNoParametersModel)
(JNIEnv* env, jobject object, jint type) {
  return NewNativeInputModel(TypeToInputModel(type));
}

JNI_METHOD(brush, InputModelNative, jlong, createSlidingWindowModel)
(JNIEnv* env, jobject object, jlong window_size_millis,
 jint upsampling_frequency_hz) {
  return NewNativeInputModel({BrushFamily::SlidingWindowModel{
      .window_size = Duration32::Millis(window_size_millis),
      .upsampling_period = upsampling_frequency_hz > 0
                               ? Duration32::Seconds(1) /
                                     static_cast<float>(upsampling_frequency_hz)
                               : Duration32::Infinite(),
  }});
}

JNI_METHOD(brush, InputModelNative, jlong,
           createSlidingWindowModelWithDefaultParameters)
(JNIEnv* env, jobject object) {
  return NewNativeInputModel({BrushFamily::SlidingWindowModel{}});
}

JNI_METHOD(brush, InputModelNative, void, free)
(JNIEnv* env, jobject object, jlong native_pointer) {
  DeleteNativeInputModel(native_pointer);
}

JNI_METHOD(brush, InputModelNative, jint, getType)
(JNIEnv* env, jobject object, jlong native_pointer) {
  return InputModelType(CastToInputModel(native_pointer));
}

JNI_METHOD(brush, InputModelNative, jlong, getSlidingWindowDurationMillis)
(JNIEnv* env, jobject object, jlong native_pointer) {
  return static_cast<jlong>(std::get<BrushFamily::SlidingWindowModel>(
                                CastToInputModel(native_pointer))
                                .window_size.ToMillis());
}

JNI_METHOD(brush, InputModelNative, jint, getSlidingUpsamplingFrequencyHz)
(JNIEnv* env, jobject object, jlong native_pointer) {
  float upsampling_period_seconds = std::get<BrushFamily::SlidingWindowModel>(
                                        CastToInputModel(native_pointer))
                                        .upsampling_period.ToSeconds();
  return static_cast<jint>(
      std::min(1.0f / upsampling_period_seconds,
               static_cast<float>(std::numeric_limits<jint>::max())));
}

}  // extern "C"
