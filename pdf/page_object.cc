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

#include "ink/pdf/page_object.h"

#include <cmath>

#include "third_party/absl/memory/memory.h"
#include "third_party/pdfium/public/fpdf_edit.h"

namespace ink {
namespace pdf {

void PageObject::Transform(double a, double b, double c, double d, double e,
                           double f) {
  FPDFPageObj_Transform(WrappedObject(), a, b, c, d, e, f);
}

void PageObject::Translate(double dx, double dy) {
  Transform(1, 0, 0, 1, dx, dy);
}

void PageObject::Rotate(double rads) {
  auto cosq = std::cos(rads);
  auto sinq = std::sin(rads);
  Transform(cosq, sinq, -sinq, cosq, 0, 0);
}

void PageObject::Scale(double sx, double sy) { Transform(sx, 0, 0, sy, 0, 0); }

std::unique_ptr<PageObjectMark> PageObject::AddMark(absl::string_view name) {
  std::string sname = static_cast<std::string>(name);
  return absl::make_unique<PageObjectMark>(
      owning_document_, obj_, FPDFPageObj_AddMark(obj_, sname.c_str()));
}

int PageObject::MarkCount() const { return FPDFPageObj_CountMarks(obj_); }

Status PageObject::GetMark(int i, std::unique_ptr<PageObjectMark>* out) const {
  auto mark = FPDFPageObj_GetMark(obj_, i);
  if (!mark) {
    return ErrorStatus(
        StatusCode::NOT_FOUND,
        "No such mark (given index $0 in object with $1 marks(s))", i,
        MarkCount());
  }
  *out = absl::make_unique<PageObjectMark>(owning_document_, obj_, mark);
  return OkStatus();
}

}  // namespace pdf
}  // namespace ink
