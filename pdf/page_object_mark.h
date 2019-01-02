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

#ifndef INK_PDF_PAGE_OBJECT_MARK_H_
#define INK_PDF_PAGE_OBJECT_MARK_H_

#include "third_party/absl/strings/string_view.h"
#include "third_party/pdfium/public/fpdfview.h"
#include "ink/engine/public/types/status.h"

namespace ink {
namespace pdf {

// A PageObjectMark is a named key-value store associated with a PageObject. The
// Mark need not have any values (referred to as "params" in the pdfium SDK),
// but must have a name.
class PageObjectMark {
 public:
  PageObjectMark(FPDF_DOCUMENT owning_document,
                 FPDF_PAGEOBJECT owning_pageobject, FPDF_PAGEOBJECTMARK mark)
      : owning_document_(owning_document),
        owning_pageobject_(owning_pageobject),
        mark_(mark) {
    assert(owning_document);
    assert(owning_pageobject);
    assert(mark);
  }

  // The name should be an ASCII-only character sequence.
  Status GetName(std::string* out) const;

  Status GetIntParam(absl::string_view key, int* value) const;
  Status SetIntParam(absl::string_view key, int value);

  // You can store and retrieve arbitrary byte blobs here; these are not
  // constrained to unicode text.
  Status GetStringParam(absl::string_view key, std::string* value) const;
  Status SetStringParam(absl::string_view key, absl::string_view value);

 private:
  Status ExpectParamType(absl::string_view key,
                         FPDF_OBJECT_TYPE expected_type) const;

  FPDF_DOCUMENT owning_document_;
  FPDF_PAGEOBJECT owning_pageobject_;
  FPDF_PAGEOBJECTMARK mark_;
};

}  // namespace pdf
}  // namespace ink

#endif  // INK_PDF_PAGE_OBJECT_MARK_H_
