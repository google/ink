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

#include "ink/geometry/internal/jni/vec_native.h"

#include "ink/geometry/angle.h"
#include "ink/geometry/vec.h"

using ::ink::Vec;

extern "C" {

VecNative_Vec VecNative_unitVec(float x, float y) {
  Vec unit_vec = Vec{x, y}.AsUnitVec();
  return {unit_vec.x, unit_vec.y};
}

float VecNative_absoluteAngleBetweenInDegrees(float first_vec_x,
                                              float first_vec_y,
                                              float second_vec_x,
                                              float second_vec_y) {
  return Vec::AbsoluteAngleBetween(Vec{first_vec_x, first_vec_y},
                                   Vec{second_vec_x, second_vec_y})
      .ValueInDegrees();
}

float VecNative_signedAngleBetweenInDegrees(float first_vec_x,
                                            float first_vec_y,
                                            float second_vec_x,
                                            float second_vec_y) {
  return Vec::SignedAngleBetween(Vec{first_vec_x, first_vec_y},
                                 Vec{second_vec_x, second_vec_y})
      .ValueInDegrees();
}

}  // extern "C"
