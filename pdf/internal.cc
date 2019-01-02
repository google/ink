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

#include "ink/pdf/internal.h"

#include <cmath>

#include "third_party/absl/base/config.h"  // endian checks
#include "third_party/utf8/utf8/unchecked.h"

namespace ink {
namespace pdf {
namespace internal {
int Clamp(int v, int low, int high) { return std::min(std::max(v, low), high); }

std::string Utf16LEToUtf8(const std::vector<char16_t>& utf16) {
#ifdef ABSL_IS_BIG_ENDIAN
  std::vector<char16_t> host_order(utf16);
  std::transform(host_order.begin(), host_order.end(), host_order.begin(),
                 &util::little_endian::ToHost16);
#else
  const std::vector<char16_t>& host_order = utf16;
#endif  // ABSL_IS_BIG_ENDIAN
  std::string result;
  utf8::unchecked::utf16to8(host_order.begin(), host_order.end(),
                            std::back_inserter(result));
  return result;
}

std::u16string Utf8ToUtf16LE(absl::string_view utf8) {
  std::u16string result;
  utf8::unchecked::utf8to16(utf8.begin(), utf8.end(),
                            std::back_inserter(result));
#ifdef ABSL_IS_BIG_ENDIAN
  std::transform(result.begin(), result.end(), result.begin(),
                 &util::little_endian::FromHost16);
#endif  // ABSL_IS_BIG_ENDIAN
  return result;
}
std::string Utf32ToUtf8(const std::vector<uint32_t>& utf32) {
  std::string result;
  utf8::unchecked::utf32to8(utf32.begin(), utf32.end(),
                            std::back_inserter(result));
  return result;
}

Status FetchUtf16StringAsUtf8(BlobFetcher fetcher, std::string* out) {
  size_t expected_length = fetcher(nullptr, 0);
  if (expected_length % 2 != 0) {
    return ErrorStatus(
        "expected an even size in bytes because decoding 16 byte values");
  }
  // Size is returned in bytes, but we are writing into char16s, so we only need
  // half of the "size" in char16s.
  std::vector<char16_t> utf16LE(expected_length / 2);
  if (expected_length != fetcher(&utf16LE[0], expected_length)) {
    return ErrorStatus("could not read expected number of bytes");
  }
  *out = internal::Utf16LEToUtf8(utf16LE);
  // remove terminating 0
  out->pop_back();
  return OkStatus();
}

}  // namespace internal
}  // namespace pdf
}  // namespace ink
