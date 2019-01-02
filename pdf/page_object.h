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

#ifndef INK_PDF_PAGE_OBJECT_H_
#define INK_PDF_PAGE_OBJECT_H_

#include <cassert>
#include <memory>

#include "third_party/absl/strings/string_view.h"
#include "third_party/pdfium/public/fpdfview.h"
#include "ink/pdf/page_object_mark.h"

namespace ink {
namespace pdf {

// A PageObject is a wrapper around pdfium's FPDF_PAGEOBJECT handle, which
// itself represent an object in a page (not a page itself). Examples of page
// objects include text objects (see text_object.h) and path objects (see
// path_object.h). Any page object can have its own transform from its local
// coordinates into page-local coordinates, and can be marked with
// PageObjectMarks. You must create new PageObjects via methods in document.h.
class PageObject {
 public:
  PageObject(FPDF_DOCUMENT owning_document, FPDF_PAGEOBJECT obj)
      : owning_document_(owning_document), obj_(obj) {
    assert(obj);
  }
  virtual ~PageObject() {}

  std::unique_ptr<PageObjectMark> AddMark(absl::string_view name);
  int MarkCount() const;
  Status GetMark(int i, std::unique_ptr<PageObjectMark>* out) const;

  void Transform(double a, double b, double c, double d, double e, double f);
  void Translate(double dx, double dy);
  void Rotate(double rads);
  void Scale(double sx, double sy);

  FPDF_PAGEOBJECT WrappedObject() const { return obj_; }

 private:
  FPDF_DOCUMENT owning_document_;
  FPDF_PAGEOBJECT obj_;
};

}  // namespace pdf
}  // namespace ink
#endif  // INK_PDF_PAGE_OBJECT_H_
