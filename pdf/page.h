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

#ifndef INK_PDF_PAGE_H_
#define INK_PDF_PAGE_H_

#include <memory>

#include "third_party/absl/base/attributes.h"
#include "third_party/absl/strings/string_view.h"
#include "third_party/pdfium/public/cpp/fpdf_scopers.h"
#include "third_party/pdfium/public/fpdfview.h"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/public/types/client_bitmap.h"
#include "ink/engine/public/types/color.h"
#include "ink/engine/public/types/status.h"
#include "ink/pdf/annotation.h"
#include "ink/pdf/page_object.h"
#include "ink/pdf/text_page.h"

namespace ink {
namespace pdf {

// A Page wraps the pdfium FPDF_PAGE handle, and provides convenience functions
// for rendering to ink-friendly bitmap structures. You can create pages via
// methods in document.h.
// The Page destructor handles cleanup of the wrapped object, including the
// "generate content" step required by the pdfium API, if the Page has been
// mutated during its life.
class Page {
 public:
  Page(FPDF_PAGE page, FPDF_DOCUMENT owner);
  ~Page() { MaybeGenerateContent(); }

  // Returns the number of degrees by which the page should be rotated clockwise
  // when displayed or printed. The value is always in {0, 90, 180, 270}.
  int Rotation() const;

  // Returns the number of radians by which the page should be rotated
  // counterclockwise when displayed or printed.
  // The value is always in {0, π/2, π, 3π/2}.
  float RotationRadians() const;

  // Return the intersection of the media box and the crop box.
  // The media box and crop box are described in section 7.7.3.3 of the spec.
  Rect Bounds() const;

  Rect CropBox() const;

  // Render this Page into a new ClientBitmap at the given scale. A scale of 1
  // means that 1 page unit gets 1 pixel. To render for a 300 DPI device, for
  // example, given default PDF user units, you'd want scale = 300.0/72.0. The
  // ClientBitmap created by this function has BITMAP_FORMAT_RGBA_8888.
  ABSL_MUST_USE_RESULT Status Render(float scale,
                                     std::unique_ptr<ClientBitmap>* out) const;

  // Given a bitmap, grab a region of the Page, and apply the given transform to
  // that region, rendering it to the given bitmap. The ClientBitmap provided to
  // this function must have BITMAP_FORMAT_RGBA_8888, and must be square. The
  // requested region is specified in a coordinate system where 0,0 is the
  // lower-left corner of the page, and width,height is the upper-right.
  ABSL_MUST_USE_RESULT Status RenderTile(const Rect& source_region,
                                         ClientBitmap* out) const;

  int GetAnnotationCount() const;

  bool HasTransparency() const;

  std::unique_ptr<StampAnnotation> CreateStampAnnotation();
  std::unique_ptr<TextAnnotation> CreateTextAnnotation(
      const Rect& bounds, absl::string_view utf8_text);

  void AppendObject(const PageObject& obj);

  int PageObjectCount() const;

  ABSL_MUST_USE_RESULT Status
  GetPageObject(int i, std::unique_ptr<PageObject>* out) const;

  // Takes ownership and deletes.
  ABSL_MUST_USE_RESULT Status
  RemovePageObject(std::unique_ptr<PageObject> object);

  ABSL_MUST_USE_RESULT Status
  GetAnnotation(int index, std::unique_ptr<Annotation>* out) const;
  // Attempt to delete the annotation at the given index.
  ABSL_MUST_USE_RESULT Status RemoveAnnotation(int index);

  // Page owns the TextPage and the caller must not store it beyond the lifetime
  // of this page.
  ABSL_MUST_USE_RESULT Status GetTextPage(TextPage** out);

  // Draws a rectangle path for debugging purposes
  ABSL_MUST_USE_RESULT Status AddDebugRectangle(const Rect& r, Color stroke,
                                                Color fill, const StrokeMode& s,
                                                const FillMode& f);

  FPDF_PAGE GetWrappedPage() const { return page_.get(); }

  // Page is neither copyable nor movable.
  Page(const Page&) = delete;
  Page& operator=(const Page&) = delete;

 private:
  void MaybeGenerateContent() const;

  ScopedFPDFPage page_;
  FPDF_DOCUMENT owning_document_;
  std::unique_ptr<TextPage> text_page_;

  mutable bool dirty_{false};

  mutable Rect cached_media_box_;
  mutable Rect cached_crop_box_;
  mutable Rect cached_bounds_;
  mutable int cached_rotation_;
};

}  // namespace pdf
}  // namespace ink
#endif  // INK_PDF_PAGE_H_
