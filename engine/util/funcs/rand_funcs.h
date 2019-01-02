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

#ifndef INK_ENGINE_UTIL_FUNCS_RAND_FUNCS_H_
#define INK_ENGINE_UTIL_FUNCS_RAND_FUNCS_H_

#include <cstdint>

namespace ink {

void Seed_random(uint64_t seed);

// Random number from [0 .. uint64_t max value]
uint64_t U64rand();

// Random double in [fMin .. fMax]
double Drand(double f_min, double f_max);

// Random int in [min .. max]
int Irand(int min, int max);

double Perturb(double value, double p);
double RandNormal(double mean, double stddev);
}  // namespace ink

#endif  // INK_ENGINE_UTIL_FUNCS_RAND_FUNCS_H_
