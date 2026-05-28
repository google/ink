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

#ifndef INK_STORAGE_INTERNAL_JNI_BYTE_VECTOR_NATIVE_HELPER_H_
#define INK_STORAGE_INTERNAL_JNI_BYTE_VECTOR_NATIVE_HELPER_H_

#include <cstdint>
#include <utility>
#include <vector>

namespace ink::native {

inline int64_t NewNativeByteVector(std::vector<int8_t>&& bytes) {
  return reinterpret_cast<int64_t>(
      new std::vector<int8_t>(std::forward<std::vector<int8_t>>(bytes)));
}

inline const std::vector<int8_t>& CastToByteVector(int64_t native_pointer) {
  return *reinterpret_cast<const std::vector<int8_t>*>(native_pointer);
}

inline void DeleteByteVector(int64_t native_pointer) {
  delete reinterpret_cast<std::vector<int8_t>*>(native_pointer);
}

}  // namespace ink::native

#endif  // INK_STORAGE_INTERNAL_JNI_BYTE_VECTOR_NATIVE_HELPER_H_
