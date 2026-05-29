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

#include "ink/storage/internal/jni/byte_vector_native_helper.h"

#include <cstdint>
#include <vector>

#include "absl/log/absl_check.h"
#include "google/protobuf/message_lite.h"

namespace ink::native {

int64_t NewNativeByteVectorFromProto(const google::protobuf::MessageLite& src) {
  auto* bytes = new std::vector<int8_t>(src.ByteSizeLong());
  ABSL_CHECK(src.SerializeToArray(bytes->data(), bytes->size()));
  return reinterpret_cast<int64_t>(bytes);
}

}  // namespace ink::native
