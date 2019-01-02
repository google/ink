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

#ifndef INK_ENGINE_UTIL_FUNCS_STEP_UTILS_H_
#define INK_ENGINE_UTIL_FUNCS_STEP_UTILS_H_

#include <algorithm>
#include <cmath>

#include "third_party/glm/glm/glm.hpp"

namespace ink {
namespace util {

// Returns v if in [min,max], otherwise returns nearest endpoint.
// max, min, and v must all be the same type which is also the return type.
// (Currently supported only for arithmetic types to avoid expensive
// call-by-value for more complex types. Also note that std::clamp is
// defined for C++17, but it both takes and returns references.)
template <typename T>
typename std::enable_if<std::is_arithmetic<T>::value, T>::type Clamp(T min,
                                                                     T max,
                                                                     T v) {
  return std::max(std::min(max, v), min);
}

inline float Clamp0N(float max, float value) { return Clamp(0.0f, max, value); }

inline float Clamp01(float v) { return Clamp(0.0f, 1.0f, v); }


// Applies Clamp0N to v componentwise.
inline glm::vec2 Clamp0N(float max, glm::vec2 v) {
  v.x = Clamp0N(max, v.x);
  v.y = Clamp0N(max, v.y);
  return v;
}

// Applies clamp01 to v componentwise.
inline glm::vec3 Clamp01(glm::vec3 v) {
  v.x = Clamp01(v.x);
  v.y = Clamp01(v.y);
  v.z = Clamp01(v.z);
  return v;
}

// Applies clamp01 to v componentwise.
inline glm::vec4 Clamp01(glm::vec4 v) {
  v.x = Clamp01(v.x);
  v.y = Clamp01(v.y);
  v.z = Clamp01(v.z);
  v.w = Clamp01(v.w);
  return v;
}

// If the x or y component of v exceeds the max dimension, scale down v to fit
// within the max while maintaining the aspect ratio.
inline glm::vec2 ScaleWithinMax(glm::vec2 v, float max_dimension) {
  return v *
         std::fmin(1.0f, std::fmin(max_dimension / v.x, max_dimension / v.y));
}

// Linearly interpolates between "from" and "to". Extrapolates when "amount" not
// in [0,1].
template <typename T>
T Lerpnc(T from, T to, float amount) {
  return from + (to - from) * amount;
}

// Linearly interpolates between "from" and "to".  Clamps "amount" to be in
// [0,1].
template <typename T>
T Lerp(T from, T to, float amount) {
  amount = Clamp01(amount);
  return Lerpnc(from, to, amount);
}

// If "amount" between "min" and "max", linearly rescales "amount" to [0,1].
//
// Note: lerpnc and normalize are inverse functions
template <typename T>
T Normalize(T min, T max, T amount) {
  if (max - min == 0) {
    return T(0);
  }
  return T((amount - min) / (max - min));
}

// An ease-in-out smoothed version of lerp.  Interpolates between "from" and
// "to", changing slowly when "amount" is near "from" or "to" and approximately
// linearly otherwise.
template <typename T>
T Smoothstep(T from, T to, float amount) {
  amount = Clamp01(amount);
  // http://www.wolframalpha.com/input/?i=%28x%5E2%29+*+%283+-+2*x%29
  float y = std::pow(amount, 2) * (3 - 2 * amount);
  // auto y = 3*pow(amount, 3) - 2*pow(amount,2);
  return y * (to - from) + from;
}

// Inverse of an approximation of the smoothstep function above.
inline float Ismoothstep(float from, float to, float s) {
  // approximate smoothstep with:
  // s = 0.5 - cos(-a * pi) * 0.5
  // Approximation error:
  // http://www.wolframalpha.com/input/?i=%280.5+-+cos%28-x+*+pi%29+*+0.5%29+-+%28%28x%5E2%29+*+%283+-+2*x%29%29+from+0+to+1
  // as it's easier to invert...
  // -2*(s - 0.5) = cos(-a*pi)
  // acos(-2*(s-0.5)) = -a * pi
  // -acos(-2*(s-0.5))/pi = a
  // Error of approximate inverse:
  // http://www.wolframalpha.com/input/?i=Plot%5Bx+-+%28acos%28-2.0+*+%28%28%280.5+-+cos%28-x+*+pi%29+*+0.5%29%29+-+0.5%29%29+%2F+pi%29%2C+%7Bx%2C+0%2C+1%7D%2C+%7By%2C+-10%5E-9%2C10%5E-9%7D%5D
  float a = Normalize(from, to, s);
  a = std::acos(-2.0f * (s - 0.5f)) / M_PI;
  return a;
}

template <typename T>
T EaseIn(T from, T to, float amount) {
  amount = Clamp01(amount);
  float y = (amount * amount) * (0.6f + 0.4f * amount);
  return y * (to - from) + from;
}

template <typename T>
T EaseOut(T from, T to, float amount) {
  amount = Clamp01(amount);
  float y = 1.8f * amount - 0.8f * (amount * amount);
  return y * (to - from) + from;
}

// A smoothed version of lerp.  Increases linearly, but  as "amount" -> 1,
// overshoots "to" then oscillates until converging at "to".
//
// Implementation:
// http://www.wolframalpha.com/input/?i=sin%28pi*x%5E4%29*%281+-+x%5E2%29+%2B+x+from+0+to+1
//
template <typename T>
T Berp(T from, T to, float amount) {
  //    amount = (sin(amount * M_PI * (0.2f + 2.5f * pow(amount, 3))) * (1.0f -
  //    pow(amount, 2.2f)) + amount) * (1.0f + (1.2f * (1.0f - amount)));
  float x = amount;
  x = std::sin(M_PI * std::pow(x, 4.0f)) * (1.0f - std::pow(x, 2.0f)) + x;
  return from + (to - from) * x;
}

}  // namespace util
}  // namespace ink

#endif  // INK_ENGINE_UTIL_FUNCS_STEP_UTILS_H_
