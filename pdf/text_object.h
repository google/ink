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

#ifndef INK_PDF_TEXT_OBJECT_H_
#define INK_PDF_TEXT_OBJECT_H_

#include "third_party/pdfium/public/fpdf_edit.h"
#include "third_party/pdfium/public/fpdfview.h"
#include "ink/pdf/page_object.h"

namespace ink {
namespace pdf {

class Text : public PageObject {
 public:
  Text(FPDF_DOCUMENT owning_document, FPDF_PAGEOBJECT text)
      : PageObject(owning_document, text) {
    assert(FPDFPageObj_GetType(WrappedObject()) == FPDF_PAGEOBJ_TEXT);
  }
  void SetColor(int red, int green, int blue, int alpha);
};

}  // namespace pdf
}  // namespace ink
#endif  // INK_PDF_TEXT_OBJECT_H_
