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

#ifndef INK_PDF_PDF_CLIENT_BITMAP_H_
#define INK_PDF_PDF_CLIENT_BITMAP_H_

#include <cassert>

#include "third_party/pdfium/public/cpp/fpdf_scopers.h"
#include "third_party/pdfium/public/fpdfview.h"
#include "ink/engine/public/types/client_bitmap.h"

namespace ink {
namespace pdf {

class PdfClientBitmap : public ClientBitmap {
 public:
  explicit PdfClientBitmap(FPDF_BITMAP bitmap)
      : ClientBitmap(ImageSize(FPDFBitmap_GetWidth(bitmap),
                               FPDFBitmap_GetHeight(bitmap)),
                     ImageFormat::BITMAP_FORMAT_RGBA_8888),
        bitmap_(bitmap) {
    assert(bitmap);
  }

  const void* imageByteData() const override {
    return FPDFBitmap_GetBuffer(bitmap_.get());
  }

  void* imageByteData() override { return FPDFBitmap_GetBuffer(bitmap_.get()); }

 private:
  ScopedFPDFBitmap bitmap_;
};

}  // namespace pdf
}  // namespace ink
#endif  // INK_PDF_PDF_CLIENT_BITMAP_H_
