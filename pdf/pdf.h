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

// Wrappers for pdfium objects providing lifetime-based resource management.
//
// References to the PDF spec can be looked up in
//
//  http://wwwimages.adobe.com/content/dam/acom/en/devnet/pdf/PDF32000_2008.pdf
//
// Nothing in this API is threadsafe.

#ifndef INK_PDF_PDF_H_
#define INK_PDF_PDF_H_

#include "third_party/absl/strings/string_view.h"

namespace ink {
namespace pdf {

// Call this during startup. The given directory will be searched first for
// non-embedded fonts.
void InitializePdfium(absl::string_view font_path);
void InitializePdfium();
// Call this before exit to avoid leak checkers whinging.
void DestroyPdfium();

}  // namespace pdf
}  // namespace ink
#endif  // INK_PDF_PDF_H_
