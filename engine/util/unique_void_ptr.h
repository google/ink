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

// A unique_ptr<void>.
#ifndef INK_ENGINE_UTIL_UNIQUE_VOID_PTR_H_
#define INK_ENGINE_UTIL_UNIQUE_VOID_PTR_H_

#include <functional>
#include <memory>

namespace ink {
namespace util {

typedef std::unique_ptr<void, std::function<void(void const*)>> unique_void_ptr;

template <typename T>
unique_void_ptr make_unique_void(T* t) {
  return unique_void_ptr(
      t, [](void const* data) { delete static_cast<T const*>(data); });
}

}  // namespace util
}  // namespace ink

#endif  // INK_ENGINE_UTIL_UNIQUE_VOID_PTR_H_
