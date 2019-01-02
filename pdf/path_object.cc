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

#include "ink/pdf/path_object.h"

#include <cassert>

#include "third_party/absl/strings/substitute.h"
#include "ink/pdf/internal.h"

namespace ink {
namespace pdf {

using glm::vec2;
using internal::Clamp;

Path::Path(FPDF_DOCUMENT owning_document, vec2 start)
    : PageObject(owning_document, FPDFPageObj_CreateNewPath(start.x, start.y)) {
}

Status Path::UpdateDrawMode() {
  RETURN_PDFIUM_STATUS(FPDFPath_SetDrawMode(WrappedObject(),
                                            static_cast<int>(fill_mode_),
                                            static_cast<bool>(stroke_mode_)));
}

Status Path::SetFillMode(FillMode fill_mode) {
  fill_mode_ = fill_mode;
  return UpdateDrawMode();
}

Status Path::SetStrokeMode(StrokeMode stroke_mode) {
  stroke_mode_ = stroke_mode;
  return UpdateDrawMode();
}

Status Path::LineTo(vec2 p) {
  RETURN_PDFIUM_STATUS(FPDFPath_LineTo(WrappedObject(), p.x, p.y));
}

Status Path::MoveTo(vec2 p) {
  RETURN_PDFIUM_STATUS(FPDFPath_MoveTo(WrappedObject(), p.x, p.y));
}

Status Path::Close() { RETURN_PDFIUM_STATUS(FPDFPath_Close(WrappedObject())); }

Status Path::SetFillColor(Color c) {
  RETURN_PDFIUM_STATUS(
      FPDFPath_SetFillColor(WrappedObject(), c.RedByteNonPremultiplied(),
                            c.GreenByteNonPremultiplied(),
                            c.BlueByteNonPremultiplied(), c.AlphaByte()));
}

Status Path::GetFillColor(Color* out) const {
  unsigned int r, g, b, a;
  RETURN_IF_PDFIUM_ERROR(
      FPDFPath_GetFillColor(WrappedObject(), &r, &g, &b, &a));
  *out = Color::FromNonPremultiplied(
      static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b),
      static_cast<uint8_t>(a));
  return OkStatus();
}

Status Path::GetCoordinates(std::vector<glm::vec2>* out) const {
  assert(out);
  const auto& path = WrappedObject();
  out->clear();
  int n = FPDFPath_CountSegments(path);
  if (n < 0) {
    return ErrorStatus("could not count segments");
  }
  out->reserve(n);
  for (int i = 0; i < n; i++) {
    FPDF_PATHSEGMENT segment = FPDFPath_GetPathSegment(path, i);
    if (!segment) {
      out->clear();
      return ErrorStatus("could not get path segment $0", i);
    }
    float x, y;
    if (!FPDFPathSegment_GetPoint(segment, &x, &y)) {
      out->clear();
      return ErrorStatus("could not get coordinates of path segment $0", i);
    }
    int type = FPDFPathSegment_GetType(segment);
    if (type == FPDF_SEGMENT_UNKNOWN) {
      out->clear();
      return ErrorStatus("could not get determine type of path segment $0", i);
    }
    if (i == 0 && type != FPDF_SEGMENT_MOVETO) {
      out->clear();
      return ErrorStatus(
          "expected FPDF_SEGMENT_MOVETO for first segment, but got $0", type);
    }
    if (i > 0 && type != FPDF_SEGMENT_LINETO) {
      out->clear();
      return ErrorStatus(
          "expected FPDF_SEGMENT_LINETO for segment $0, but got $1", i, type);
    }
    out->emplace_back(x, y);
  }
  return OkStatus();
}

Status Path::SetStrokeColor(Color c) {
  RETURN_PDFIUM_STATUS(
      FPDFPath_SetStrokeColor(WrappedObject(), c.RedByteNonPremultiplied(),
                              c.GreenByteNonPremultiplied(),
                              c.BlueByteNonPremultiplied(), c.AlphaByte()));
}

Path::Path(FPDF_DOCUMENT owning_document, FPDF_PAGEOBJECT path)
    : PageObject(owning_document, path) {
  assert(FPDFPageObj_GetType(WrappedObject()) == FPDF_PAGEOBJ_PATH);
}

}  // namespace pdf
}  // namespace ink
