/*
 * Copyright 2018 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef INK_ENGINE_INPUT_INPUT_MODEL_DATA_TEST_UTILS_H_
#define INK_ENGINE_INPUT_INPUT_MODEL_DATA_TEST_UTILS_H_

#include "ink/engine/input/input_model_params.h"
#include "ink/engine/public/types/input.h"

namespace ink {
namespace input {

InputModelParams MakeInputModelParams(
    double max_sample_hz,
    InputModelParams params = InputModelParams(InputType::Touch));

InputModelParams MakeInputModelParamsWithWobble(float wobble_slow_speed_cm,
                                                float wobble_fast_speed_cm,
                                                float wobble_timeout_ratio);

}  // namespace input
}  // namespace ink

#endif  // INK_ENGINE_INPUT_INPUT_MODEL_DATA_TEST_UTILS_H_
