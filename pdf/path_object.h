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

#ifndef INK_PDF_PATH_OBJECT_H_
#define INK_PDF_PATH_OBJECT_H_

#include <memory>

#include "third_party/glm/glm/glm.hpp"
#include "third_party/pdfium/public/fpdf_edit.h"
#include "third_party/pdfium/public/fpdfview.h"
#include "ink/engine/public/types/color.h"
#include "ink/engine/public/types/status.h"
#include "ink/pdf/page_object.h"

namespace ink {
namespace pdf {

enum class FillMode {
  kNoFill = 0,
  kAlternate = FPDF_FILLMODE_ALTERNATE,
  kWinding = FPDF_FILLMODE_WINDING
};

enum class StrokeMode { kNoStroke = 0, kStroke = 1 };

class Path : public PageObject {
 public:
  Path(FPDF_DOCUMENT owning_document, glm::vec2 start);
  Path(FPDF_DOCUMENT owning_document, FPDF_PAGEOBJECT path);
  Status SetFillMode(FillMode fill_mode);
  // The given components c are clamped 0 ≤ c ≤ 255.
  Status SetFillColor(Color c);
  Status SetStrokeMode(StrokeMode stroke_mode);
  // The given components c are clamped 0 ≤ c ≤ 255.
  Status SetStrokeColor(Color c);
  Status LineTo(glm::vec2 p);
  Status MoveTo(glm::vec2 p);
  Status Close();

  // Returns integers [0, 255].
  Status GetFillColor(Color* out) const;

  // Ink only understands an outline path expressed as an initial MoveTo
  // followed by a sequence of LineTos. If this Path has only those operations,
  // then the given vector of coordinates will be populated by that sequence.
  // Otherwise, this function will return an error status.
  Status GetCoordinates(std::vector<glm::vec2>* out) const;

 private:
  FillMode fill_mode_ = FillMode::kNoFill;
  StrokeMode stroke_mode_ = StrokeMode::kNoStroke;

  Status UpdateDrawMode();
};

}  // namespace pdf
}  // namespace ink
#endif  // INK_PDF_PATH_OBJECT_H_
