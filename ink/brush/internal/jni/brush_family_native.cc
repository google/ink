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

#include "ink/brush/internal/jni/brush_family_native.h"

#include <algorithm>
#include <cstdint>
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
#include "ink/brush/internal/jni/brush_native_helper.h"
#include "ink/types/duration.h"

using ::ink::BrushCoat;
using ::ink::BrushFamily;
using ::ink::Duration32;
using ::ink::native::CastToBrushCoat;
using ::ink::native::CastToBrushFamily;
using ::ink::native::CastToInputModel;
using ::ink::native::DeleteNativeBrushFamily;
using ::ink::native::DeleteNativeInputModel;
using ::ink::native::NewNativeBrushCoat;
using ::ink::native::NewNativeBrushFamily;
using ::ink::native::NewNativeInputModel;

namespace {

// 0 is reserved for internal use.
// 1 is reserved (was previously the "spring model").
// 2 is reserved (was previously the experimental "raw position" model).
constexpr int kPassthroughModel = 3;
constexpr int kSlidingWindowModel = 4;

BrushFamily::InputModel TypeToInputModel(int input_model_value) {
  switch (input_model_value) {
    case kPassthroughModel:
      return BrushFamily::PassthroughModel();
    case kSlidingWindowModel:
      return BrushFamily::SlidingWindowModel();
    default:
      ABSL_CHECK(false) << "Unknown input model value: " << input_model_value;
  }
}

int InputModelType(const BrushFamily::InputModel& input_model) {
  constexpr auto visitor = absl::Overload{
      [](const BrushFamily::PassthroughModel&) { return kPassthroughModel; },
      [](const BrushFamily::SlidingWindowModel&) {
        return kSlidingWindowModel;
      },
  };
  return std::visit(visitor, input_model);
}

}  // namespace

extern "C" {

int64_t BrushFamilyNative_create(
    void* jni_env_pass_through, const int64_t* coat_native_pointers,
    int num_coats, int64_t input_model_pointer,
    const char* client_brush_family_id, const char* developer_comment,
    void (*throw_from_status_callback)(void*, int, const char*)) {
  std::vector<BrushCoat> coats;
  coats.reserve(num_coats);
  ABSL_CHECK(coat_native_pointers != nullptr);
  for (int i = 0; i < num_coats; ++i) {
    coats.push_back(CastToBrushCoat(coat_native_pointers[i]));
  }

  absl::StatusOr<BrushFamily> brush_family = BrushFamily::Create(
      coats, CastToInputModel(input_model_pointer),
      BrushFamily::Metadata{
          .client_brush_family_id =
              client_brush_family_id ? client_brush_family_id : "",
          .developer_comment = developer_comment ? developer_comment : "",
      });
  if (!brush_family.ok()) {
    throw_from_status_callback(jni_env_pass_through,
                               static_cast<int>(brush_family.status().code()),
                               brush_family.status().message().data());
    return 0;
  }

  return NewNativeBrushFamily(*std::move(brush_family));
}

void BrushFamilyNative_free(int64_t native_pointer) {
  DeleteNativeBrushFamily(native_pointer);
}

const char* BrushFamilyNative_getClientBrushFamilyId(int64_t native_pointer) {
  const BrushFamily& brush_family = CastToBrushFamily(native_pointer);
  return brush_family.GetMetadata().client_brush_family_id.c_str();
}

const char* BrushFamilyNative_getDeveloperComment(int64_t native_pointer) {
  const BrushFamily& brush_family = CastToBrushFamily(native_pointer);
  return brush_family.GetMetadata().developer_comment.c_str();
}

int64_t BrushFamilyNative_getBrushCoatCount(int64_t native_pointer) {
  const BrushFamily& brush_family = CastToBrushFamily(native_pointer);
  return brush_family.GetCoats().size();
}

int64_t BrushFamilyNative_newCopyOfBrushCoat(int64_t native_pointer,
                                             int index) {
  const BrushFamily& brush_family = CastToBrushFamily(native_pointer);
  return NewNativeBrushCoat(brush_family.GetCoats()[index]);
}

int BrushFamilyNative_getInputModelType(int64_t native_pointer) {
  const BrushFamily& brush_family = CastToBrushFamily(native_pointer);
  return InputModelType(brush_family.GetInputModel());
}

int64_t BrushFamilyNative_newCopyOfInputModel(int64_t native_pointer) {
  const BrushFamily& brush_family = CastToBrushFamily(native_pointer);
  return NewNativeInputModel(brush_family.GetInputModel());
}

int BrushFamilyNative_calculateMinimumRequiredVersion(int64_t native_pointer) {
  const BrushFamily& brush_family = CastToBrushFamily(native_pointer);
  return brush_family.CalculateMinimumRequiredVersion().value();
}

bool BrushFamilyNative_hasFallbacks(int64_t native_pointer) {
  return CastToBrushFamily(native_pointer).HasFallbacks();
}

int64_t InputModelNative_createNoParametersModel(int type) {
  return NewNativeInputModel(TypeToInputModel(type));
}

int64_t InputModelNative_createSlidingWindowModel(int64_t window_size_millis,
                                                  int upsampling_frequency_hz) {
  return NewNativeInputModel({BrushFamily::SlidingWindowModel{
      .window_size = Duration32::Millis(window_size_millis),
      .upsampling_period = upsampling_frequency_hz > 0
                               ? Duration32::Seconds(1) /
                                     static_cast<float>(upsampling_frequency_hz)
                               : Duration32::Infinite(),
  }});
}

int64_t InputModelNative_createSlidingWindowModelWithDefaultParameters() {
  return NewNativeInputModel({BrushFamily::SlidingWindowModel{}});
}

void InputModelNative_free(int64_t native_pointer) {
  DeleteNativeInputModel(native_pointer);
}

int64_t InputModelNative_getSlidingWindowDurationMillis(
    int64_t native_pointer) {
  return static_cast<int64_t>(std::get<BrushFamily::SlidingWindowModel>(
                                  CastToInputModel(native_pointer))
                                  .window_size.ToMillis());
}

int InputModelNative_getSlidingUpsamplingFrequencyHz(int64_t native_pointer) {
  float upsampling_period_seconds = std::get<BrushFamily::SlidingWindowModel>(
                                        CastToInputModel(native_pointer))
                                        .upsampling_period.ToSeconds();
  return static_cast<int>(
      std::min(1.0f / upsampling_period_seconds,
               static_cast<float>(std::numeric_limits<int>::max())));
}

}  // extern "C"
