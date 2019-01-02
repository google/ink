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

#include "ink/pdf/page.h"

#include <cmath>

#include "third_party/absl/memory/memory.h"
#include "third_party/glm/glm/glm.hpp"
#include "third_party/glm/glm/gtx/matrix_decompose.hpp"
#include "third_party/pdfium/public/fpdf_edit.h"
#include "third_party/pdfium/public/fpdf_transformpage.h"
#include "ink/engine/geometry/algorithms/intersect.h"
#include "ink/engine/geometry/algorithms/transform.h"
#include "ink/engine/geometry/primitives/matrix_utils.h"
#include "ink/pdf/internal.h"
#include "ink/pdf/path_object.h"
#include "ink/pdf/text_object.h"

// Bigger than this many pixels? No.
static constexpr int kMaxRenderBitmapPixels = 5000 * 5000;

static constexpr int kRenderFlags = FPDF_ANNOT;

namespace ink {
namespace pdf {

Page::Page(FPDF_PAGE page, FPDF_DOCUMENT owner)
    : page_(page),
      owning_document_(owner),
      text_page_(nullptr),
      cached_bounds_(0, 0, 0, 0),
      cached_rotation_(-1) {
  CHECK(page_ != nullptr);
  CHECK(owning_document_ != nullptr);
}

void Page::MaybeGenerateContent() const {
  if (dirty_) {
    dirty_ = false;
    SLOG(SLOG_PDF, "Generating content.");
    if (!FPDFPage_GenerateContent(page_.get())) {
      SLOG(SLOG_ERROR, "internal error; couldn't generate content");
    }
  }
}

int Page::Rotation() const {
  if (cached_rotation_ < 0) {
    switch (FPDFPage_GetRotation(page_.get())) {
      case 1:
        cached_rotation_ = 90;
        break;
      case 2:
        cached_rotation_ = 180;
        break;
      case 3:
        cached_rotation_ = 270;
        break;
      default:
        cached_rotation_ = 0;
    }
  }
  return cached_rotation_;
}

float Page::RotationRadians() const {
  // PDF display rotations are multiples of 90 degrees, and clockwise, which is
  // why 90 gets 3*half_pi in standard counterclockwise math.
  switch (Rotation()) {
    case 90:
      return -glm::half_pi<float>();
    case 180:
      return glm::pi<float>();
    case 270:
      return glm::half_pi<float>();
    default:
      return 0;
  }
}

namespace {
using RectFunction = FPDF_BOOL (*)(FPDF_PAGE, float*, float*, float*, float*);
void CallRectFunction(RectFunction f, FPDF_PAGE p, Rect* out) {
  float left, bottom, right, top;
  if (f(p, &left, &bottom, &right, &top)) {
    *out = Rect(left, bottom, right, top);
  }
}
}  // namespace

Rect Page::CropBox() const {
  if (cached_crop_box_.Area() == 0) {
    CallRectFunction(&FPDFPage_GetCropBox, page_.get(), &cached_crop_box_);
  }
  return cached_crop_box_;
}

Rect Page::Bounds() const {
  if (cached_bounds_.Area() == 0) {
    FS_RECTF result;
    if (FPDF_GetPageBoundingBox(page_.get(), &result)) {
      cached_bounds_ =
          Rect(result.left, result.bottom, result.right, result.top);
    }
  }
  return cached_bounds_;
}

std::unique_ptr<StampAnnotation> Page::CreateStampAnnotation() {
  dirty_ = true;
  return absl::make_unique<StampAnnotation>(owning_document_, page_.get());
}

std::unique_ptr<TextAnnotation> Page::CreateTextAnnotation(
    const Rect& bounds, absl::string_view utf8_text) {
  dirty_ = true;
  return absl::make_unique<TextAnnotation>(owning_document_, page_.get(),
                                           bounds, utf8_text);
}

void Page::AppendObject(const PageObject& obj) {
  dirty_ = true;
  FPDFPage_InsertObject(page_.get(), obj.WrappedObject());
}

int Page::PageObjectCount() const { return FPDFPage_CountObjects(page_.get()); }

Status Page::GetPageObject(int i, std::unique_ptr<PageObject>* out) const {
  auto obj = FPDFPage_GetObject(page_.get(), i);
  if (!obj) {
    return ErrorStatus(
        StatusCode::NOT_FOUND,
        "No such object (given index $0 in page with $1 object(s))", i,
        PageObjectCount());
  }
  auto const type = FPDFPageObj_GetType(obj);
  if (type == FPDF_PAGEOBJ_TEXT) {
    *out = absl::make_unique<Text>(owning_document_, obj);
    return OkStatus();
  }
  if (type == FPDF_PAGEOBJ_PATH) {
    *out = absl::make_unique<Path>(owning_document_, obj);
    return OkStatus();
  }
  *out = absl::make_unique<PageObject>(owning_document_, obj);
  return OkStatus();
}

Status Page::RemovePageObject(std::unique_ptr<PageObject> object) {
  if (!FPDFPage_RemoveObject(page_.get(), object->WrappedObject())) {
    return ErrorStatus(StatusCode::INVALID_ARGUMENT,
                       "Could not remove given page object.");
  }
  FPDFPageObj_Destroy(object->WrappedObject());
  dirty_ = true;
  return OkStatus();
}

Status Page::GetAnnotation(int index, std::unique_ptr<Annotation>* out) const {
  if (index >= GetAnnotationCount()) {
    return ErrorStatus("given index $0 >= total number of annotations $1",
                       index, GetAnnotationCount());
  }
  FPDF_ANNOTATION annot = FPDFPage_GetAnnot(page_.get(), index);
  if (!annot) {
    return ErrorStatus("cannot retrieve annotation $0", index);
  }
  *out = absl::make_unique<Annotation>(owning_document_, annot);
  return OkStatus();
}

Status Page::RemoveAnnotation(int index) {
  if (index >= GetAnnotationCount()) {
    return ErrorStatus("given index $0 >= total number of annotations $1",
                       index, GetAnnotationCount());
  }
  if (!FPDFPage_RemoveAnnot(page_.get(), index)) {
    return ErrorStatus("could not remove annotation $0 (error $1)", index,
                       FPDF_GetLastError());
  }
  dirty_ = true;
  return OkStatus();
}

int Page::GetAnnotationCount() const {
  return FPDFPage_GetAnnotCount(page_.get());
}

bool Page::HasTransparency() const {
  return FPDFPage_HasTransparency(page_.get());
}

Status Page::Render(float scale, std::unique_ptr<ClientBitmap>* out) const {
  MaybeGenerateContent();
  int w = static_cast<int>(std::round(scale * Bounds().Width()));
  int h = static_cast<int>(std::round(scale * Bounds().Height()));
  int num_pixels = w * h;
  if (num_pixels <= 0) {
    return ErrorStatus("invalid image size requested with area of $0", w * h);
  }
  // Correct aspect ratio for rotated pages.
  if (Rotation() == 90 || Rotation() == 270) {
    std::swap(w, h);
  }
  *out = absl::make_unique<RawClientBitmap>(
      ImageSize(w, h), ImageFormat::BITMAP_FORMAT_RGBA_8888);
  // Does not take ownership of buffer.
  FPDF_BITMAP bitmap = FPDFBitmap_CreateEx(w, h, FPDFBitmap_BGRA,
                                           (*out)->imageByteData(), w * 4);
  FPDFBitmap_FillRect(bitmap, 0, 0, w, h, 0xFFFFFFFF);
  FPDF_RenderPageBitmap(bitmap, page_.get(), 0, 0, w, h, 0, kRenderFlags);
  FPDFBitmap_Destroy(bitmap);
  client_bitmap::ConvertBGRAToRGBA(out->get());
  return OkStatus();
}

Status Page::RenderTile(const Rect& source_region, ClientBitmap* out) const {
  assert(out);
  MaybeGenerateContent();
  const ImageSize& bitmap_size = out->sizeInPx();
  if (bitmap_size.width != bitmap_size.height) {
    return ErrorStatus(
        Substitute("require square region, got $0", bitmap_size));
  }
  int size = bitmap_size.width;

  // Does not take ownership of buffer.
  const int pdfium_format =
      out->bytesPerTexel() == 3 ? FPDFBitmap_BGR : FPDFBitmap_BGRA;
  FPDF_BITMAP bitmap =
      FPDFBitmap_CreateEx(size, size, pdfium_format, out->imageByteData(),
                          size * out->bytesPerTexel());

  const auto& bounds = Bounds();

  float scale = size / source_region.Width();

  // Translate the desired region into pixel space.
  float tx = -source_region.Left();
  // Flip Y axis, because the pdfium render API is positive Y goes down.
  float ty = -(bounds.Height() - source_region.Top());

  // Our actual bounds when rendered depends on our display rotation.
  const Rect& rotated_bounds = geometry::Transform(
      bounds,
      matrix_utils::RotateAboutPoint(RotationRadians(), bounds.Center()));

  // Fill the tile with white.
  FPDFBitmap_FillRect(bitmap, 0, 0, size, size, 0xFFFFFFFF);

  FS_RECTF clip{0, 0, static_cast<float>(size), static_cast<float>(size)};
  // The pdf renderer requires these offsets.
  tx += rotated_bounds.Left();
  if (Rotation() % 180 == 90) {
    ty += rotated_bounds.Bottom();
  } else {
    ty -= rotated_bounds.Bottom();
  }
  // The pdfium API doesn't specify how your matrix is supposed to have been
  // composed. Trial and error reveals that your translation needs to be scaled
  // by the scale factor.
  const FS_MATRIX matrix{scale, 0, 0, scale, scale * tx, scale * ty};
  FPDF_RenderPageBitmapWithMatrix(bitmap, page_.get(), &matrix, &clip,
                                  kRenderFlags);
  FPDFBitmap_Destroy(bitmap);
  if (out->bytesPerTexel() == 4) {
    client_bitmap::ConvertBGRAToRGBA(out);
  } else {
    client_bitmap::ConvertBGRToRGB(out);
  }

  return OkStatus();
}

Status Page::GetTextPage(TextPage** out) {
  if (!text_page_) {
    FPDF_TEXTPAGE text_page = FPDFText_LoadPage(page_.get());
    if (!text_page) {
      return ErrorStatus("Cannot load text page from PDF");
    }
    text_page_ = absl::make_unique<TextPage>(text_page);
    INK_RETURN_UNLESS((text_page_)->GenerateIndex());
  }
  *out = text_page_.get();
  return OkStatus();
}

Status Page::AddDebugRectangle(const Rect& r, Color stroke, Color fill,
                               const StrokeMode& s, const FillMode& f) {
  Path path(owning_document_, r.Leftbottom());
  path.LineTo(r.Lefttop());
  path.LineTo(r.Righttop());
  path.LineTo(r.Rightbottom());
  path.Close();

  INK_RETURN_UNLESS(path.SetStrokeMode(s));
  INK_RETURN_UNLESS(path.SetFillMode(f));
  INK_RETURN_UNLESS(path.SetStrokeColor(stroke));
  INK_RETURN_UNLESS(path.SetFillColor(fill));
  AppendObject(path);
  return OkStatus();
}

}  // namespace pdf
}  // namespace ink
