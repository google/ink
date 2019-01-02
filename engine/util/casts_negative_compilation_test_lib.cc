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

#include "ink/engine/util/casts.h"

namespace ink {

namespace {

#if defined(TEST_FLOAT_TO_DOUBLE)

double d = ink::SafeNumericCast<float, double>(4.f);

#elif defined(TEST_INT_TO_DOUBLE)

double d = ink::SafeNumericCast<int32_t, double>(4);

#elif defined(TEST_INT_TO_FLOAT)

float f = ink::SafeNumericCast<int32_t, float>(4);

#elif defined(TEST_INT_TO_LONG)

int64_t i = ink::SafeNumericCast<int32_t, int64_t>(4);

#elif defined(TEST_LONG_TO_FLOAT)

float f = ink::SafeNumericCast<int64_t, float>(4l);

#elif defined(TEST_LONG_TO_DOUBLE)

double d = ink::SafeNumericCast<int64_t, double>(4.);

#endif

}  // namespace
}  // namespace ink
