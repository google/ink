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

// DO NOT INCLUDE THIS FILE FROM A HEADER
// THIS SHOULD ONLY BE INCLUDED IN .cc FILES IN sketchology/pdf

#ifndef INK_PDF_INTERNAL_H_
#define INK_PDF_INTERNAL_H_

#include <functional>

#include "third_party/absl/strings/string_view.h"
#include "ink/engine/public/types/status.h"
#include "ink/engine/public/types/status_or.h"

// Many pdfium routines return false on failure with no error message. This
// macro creates a programmer-friendly error message for such conditions, which
// are unexpected.
#define RETURN_IF_PDFIUM_ERROR(EXPR)                      \
  do {                                                    \
    if (!(EXPR)) {                                        \
      return ErrorStatus("unexpected failure to " #EXPR); \
    }                                                     \
  } while (0);

// Wrap a pdfium function call with a Status, returning the result.
#define RETURN_PDFIUM_STATUS(EXPR)                        \
  do {                                                    \
    if ((EXPR)) {                                         \
      return OkStatus();                                  \
    } else {                                              \
      return ErrorStatus("unexpected failure to " #EXPR); \
    }                                                     \
  } while (0);

namespace ink {
namespace pdf {
namespace internal {

int Clamp(int v, int low, int high);

std::string Utf32ToUtf8(const std::vector<uint32_t>& utf32);

std::string Utf16LEToUtf8(const std::vector<char16_t>& utf16);

std::u16string Utf8ToUtf16LE(absl::string_view utf8);

// The way blobs and strings are fetched in pdfium is something like this:
// size_t len = FPDF_SomeBlobFunction(arg1, arg2, nullptr, 0);
// std::vector<uint8_t> buf(len);
// size_t fetched_len = FPDF_SomeBlobFunction(arg1, arg2, buf.data(), len);
// So we wrap that pattern.

// For example
// BlobFetcher f = [&arg1, &arg2](void *buf, size_t len) {
//     return FPDF_SomeBlobFunction(arg1, arg2, buf, len);
// };
using BlobFetcher = std::function<size_t(void*, size_t)>;

// Interprets the fetched buffer as UTF16LE, end re-encodes it as UTF8, storing
// the encoded result in *out.
StatusOr<std::string> FetchUtf16StringAsUtf8(BlobFetcher fetcher);

}  // namespace internal
}  // namespace pdf
}  // namespace ink

#endif  // INK_PDF_INTERNAL_H_
