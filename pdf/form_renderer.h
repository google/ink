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
#ifndef INK_PDF_FORM_RENDERER_H_
#define INK_PDF_FORM_RENDERER_H_

#include "third_party/pdfium/public/cpp/fpdf_scopers.h"
#include "third_party/pdfium/public/fpdf_formfill.h"
#include "third_party/pdfium/public/fpdfview.h"

namespace ink {
namespace pdf {

// Renders forms including annotations, text fields, checkboxes.
class FormRenderer : public FPDF_FORMFILLINFO {
 public:
  explicit FormRenderer(FPDF_DOCUMENT doc);

  // FormRenderer is neither copyable nor movable.
  FormRenderer(const FormRenderer&) = delete;
  FormRenderer& operator=(const FormRenderer&) = delete;

  void NotifyAfterPageLoad(FPDF_PAGE page);
  void NotifyBeforePageClose(FPDF_PAGE page);

  /** Renders any filled-in form elements on the given page.
   * @param bitmap The pdfium bitmap to render to.
   * @param page The pdfium page possibly containing form elements to render.
   * @param page_width The scaled width to render the page at.
   * @param page_height The scaled height to render the page at.
   * @param tx The x offset to apply to the *page*, relative to the bitmap's
   * left side.
   * @param ty The y offset to apply to the *page*, relative to the bitmap's
   * top, with positive y going down.
   */
  void RenderTile(FPDF_BITMAP bitmap, FPDF_PAGE page, int page_width,
                  int page_height, int tx, int ty) const;

 private:
  ScopedFPDFFormHandle form_handle_;
};

}  // namespace pdf
}  // namespace ink

#endif  // INK_PDF_FORM_RENDERER_H_
