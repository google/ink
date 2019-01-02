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

#include "ink/engine/util/funcs/rand_funcs.h"

#include <cstdint>
#include <random>

namespace ink {

static std::mt19937_64 rng;

void Seed_random(uint64_t seed) { rng.seed(seed); }

uint64_t U64rand() {
  std::uniform_int_distribution<uint64_t> u64dist;
  return u64dist(rng);
}

double Drand(double f_min, double f_max) {
  std::uniform_real_distribution<double> dist(f_min, f_max);
  return dist(rng);
}

int Irand(int min, int max) {
  std::uniform_int_distribution<int> dist(min, max);
  return dist(rng);
}

double Perturb(double value, double p) {
  return Drand(value * (1 - p), value * (1 + p));
}

double RandNormal(double mean, double stddev) {
  std::normal_distribution<> nd(mean, stddev);
  return nd(rng);
}

}  // namespace ink
