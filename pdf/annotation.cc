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

#include "ink/pdf/annotation.h"

#include "third_party/absl/memory/memory.h"
#include "third_party/absl/strings/substitute.h"
#include "ink/pdf/internal.h"

static constexpr char kContentsKey[] = "Contents";

namespace ink {
namespace pdf {

Status Annotation::SetRect(const Rect& bounds) {
  FS_RECTF Rect{bounds.Left(), bounds.Top(), bounds.Right(), bounds.Bottom()};
  RETURN_PDFIUM_STATUS(FPDFAnnot_SetRect(annotation_.get(), &Rect));
}

Status Annotation::SetStringValue(absl::string_view key,
                                  absl::string_view value) {
  std::string skey(key);
  auto wvalue = internal::Utf8ToUtf16LE(value);
  // pdfium uses null-terminated UTF16LE
  wvalue.push_back(0);
  RETURN_PDFIUM_STATUS(FPDFAnnot_SetStringValue(
      annotation_.get(), reinterpret_cast<FPDF_BYTESTRING>(skey.data()),
      reinterpret_cast<FPDF_WIDESTRING>(wvalue.data())));
}

bool Annotation::HasKey(absl::string_view key) const {
  std::string skey(key);
  return FPDFAnnot_HasKey(annotation_.get(),
                          reinterpret_cast<FPDF_BYTESTRING>(skey.data()));
}

Status Annotation::GetStringValue(absl::string_view key,
                                  std::string* out) const {
  std::string skey(key);
  auto bkey = reinterpret_cast<FPDF_BYTESTRING>(skey.data());
  return internal::FetchUtf16StringAsUtf8(
      [this, &bkey](void* buf, size_t len) {
        return FPDFAnnot_GetStringValue(annotation_.get(), bkey, buf, len);
      },
      out);
}

Status Annotation::AppendObject(const PageObject& obj) {
  RETURN_PDFIUM_STATUS(
      FPDFAnnot_AppendObject(annotation_.get(), obj.WrappedObject()));
}

int StampAnnotation::GetPathCount() {
  return FPDFAnnot_GetObjectCount(annotation_.get());
}

Status StampAnnotation::GetPath(int index, std::unique_ptr<Path>* out) {
  out->reset();
  FPDF_PAGEOBJECT page_object = FPDFAnnot_GetObject(annotation_.get(), index);
  if (!page_object) {
    return ErrorStatus(absl::Substitute("no object found at index $0", index));
  }
  if (FPDFPageObj_GetType(page_object) != FPDF_PAGEOBJ_PATH) {
    return ErrorStatus(absl::Substitute("object $0 is not a path", index));
  }
  *out = absl::make_unique<Path>(owning_document_, page_object);
  return OkStatus();
}

TextAnnotation::TextAnnotation(FPDF_DOCUMENT owning_document, FPDF_PAGE page,
                               const Rect& bounds, absl::string_view utf8_text)
    : Annotation(owning_document, FPDFPage_CreateAnnot(page, FPDF_ANNOT_TEXT)) {
  SetRect(bounds);
  SetStringValue(kContentsKey, utf8_text);
}

}  // namespace pdf
}  // namespace ink
