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

#ifndef INK_PDF_ANNOTATION_H_
#define INK_PDF_ANNOTATION_H_

#include <cassert>
#include <memory>
#include <string>

#include "third_party/absl/strings/string_view.h"
#include "third_party/pdfium/public/cpp/fpdf_scopers.h"
#include "third_party/pdfium/public/fpdf_annot.h"
#include "third_party/pdfium/public/fpdfview.h"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/pdf/page_object.h"
#include "ink/pdf/path_object.h"

namespace ink {
namespace pdf {

class Annotation {
 public:
  Annotation(FPDF_DOCUMENT owning_document, FPDF_ANNOTATION annotation)
      : owning_document_(owning_document), annotation_(annotation) {
    assert(owning_document);
    assert(annotation);
  }
  virtual ~Annotation() {}
  virtual Status SetRect(const Rect& bounds);
  // Set an arbitrary key/value pair in the Annot dictionary.
  virtual Status SetStringValue(absl::string_view key, absl::string_view value);
  virtual bool HasKey(absl::string_view key) const;
  virtual Status GetStringValue(absl::string_view key, std::string* out) const;
  virtual Status AppendObject(const PageObject& obj);

 protected:
  FPDF_DOCUMENT owning_document_;
  ScopedFPDFAnnotation annotation_;
};

class StampAnnotation : public Annotation {
 public:
  explicit StampAnnotation(FPDF_DOCUMENT owning_document, FPDF_PAGE page)
      : Annotation(owning_document,
                   FPDFPage_CreateAnnot(page, FPDF_ANNOT_STAMP)) {}
  int GetPathCount();
  Status GetPath(int index, std::unique_ptr<Path>* out);
};

class TextAnnotation : public Annotation {
 public:
  TextAnnotation(FPDF_DOCUMENT owning_document, FPDF_PAGE page,
                 const Rect& bounds, absl::string_view utf8_text);
};

}  // namespace pdf
}  // namespace ink
#endif  // INK_PDF_ANNOTATION_H_
