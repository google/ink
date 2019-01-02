// Copyright 2018 Google LLC
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

#include "ink/engine/input/input_model_params_test_utils.h"

namespace ink {
namespace input {

InputModelParams MakeInputModelParams(double max_sample_hz,
                                      InputModelParams params) {
  params.max_sample_hz_ = max_sample_hz;
  params.predict_interval_ = 0.034;
  return params;
}

InputModelParams MakeInputModelParamsWithWobble(float wobble_slow_speed_cm,
                                                float wobble_fast_speed_cm,
                                                float wobble_timeout_ratio) {
  InputModelParams params(InputType::Touch);
  params.wobble_slow_speed_cm_ = wobble_slow_speed_cm;
  params.wobble_fast_speed_cm_ = wobble_fast_speed_cm;
  params.wobble_timeout_ratio_ = wobble_timeout_ratio;
  return params;
}

}  // namespace input
}  // namespace ink
