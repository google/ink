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
#include "third_party/pdfium/public/fpdf_formfill.h"
#include "third_party/pdfium/public/fpdf_transformpage.h"
#include "ink/engine/geometry/algorithms/intersect.h"
#include "ink/engine/geometry/algorithms/transform.h"
#include "ink/engine/geometry/primitives/matrix_utils.h"
#include "ink/pdf/internal.h"
#include "ink/pdf/path_object.h"
#include "ink/pdf/text_object.h"

static constexpr int kRenderFlags = FPDF_ANNOT;

namespace ink {
namespace pdf {

Page::Page(FPDF_PAGE page, FPDF_DOCUMENT owner,
           std::shared_ptr<FormRenderer> form_renderer)
    : page_(page),
      owning_document_(owner),
      text_page_(nullptr),
      form_renderer_(std::move(form_renderer)),
      cached_bounds_(0, 0, 0, 0) {
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

int Page::RotationDegrees() const {
  int pdfium_rotation = FPDFPage_GetRotation(page_.get());
  switch (pdfium_rotation) {
    case 1:
      return 90;
    case 2:
      return 180;
    case 3:
      return 270;
    default:
      return 0;
  }
}

void Page::SetRotationDegrees(int degrees_clockwise) {
  dirty_ = true;
  switch (degrees_clockwise) {
    case 0:
      FPDFPage_SetRotation(page_.get(), 0);
      break;
    case 90:
      FPDFPage_SetRotation(page_.get(), 1);
      break;
    case 180:
      FPDFPage_SetRotation(page_.get(), 2);
      break;
    case 270:
      FPDFPage_SetRotation(page_.get(), 3);
      break;
    default:
      SLOG(SLOG_ERROR, "rotation must be in {0, 90, 180, 270}; got $0",
           degrees_clockwise);
  }
  MaybeGenerateContent();
}

float Page::RotationRadians() const {
  // PDF display rotations are multiples of 90 degrees, and clockwise, which is
  // why 90 gets -half_pi in standard counterclockwise math.
  switch (RotationDegrees()) {
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

void Page::AppendObject(const PageObject& obj) {
  dirty_ = true;
  FPDFPage_InsertObject(page_.get(), obj.WrappedObject());
}

int Page::PageObjectCount() const { return FPDFPage_CountObjects(page_.get()); }

StatusOr<std::unique_ptr<PageObject>> Page::GetPageObject(int i) const {
  auto obj = FPDFPage_GetObject(page_.get(), i);
  if (!obj) {
    return ErrorStatus(
        StatusCode::NOT_FOUND,
        "No such object (given index $0 in page with $1 object(s))", i,
        PageObjectCount());
  }
  auto const type = FPDFPageObj_GetType(obj);
  std::unique_ptr<PageObject> out;
  if (type == FPDF_PAGEOBJ_TEXT) {
    return {absl::make_unique<Text>(owning_document_, obj)};
  }
  if (type == FPDF_PAGEOBJ_PATH) {
    return {absl::make_unique<Path>(owning_document_, obj)};
  }
  return {absl::make_unique<PageObject>(owning_document_, obj)};
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

StatusOr<std::unique_ptr<Annotation>> Page::GetAnnotation(int index) const {
  if (index >= GetAnnotationCount()) {
    return ErrorStatus("given index $0 >= total number of annotations $1",
                       index, GetAnnotationCount());
  }
  FPDF_ANNOTATION annot = FPDFPage_GetAnnot(page_.get(), index);
  if (!annot) {
    return ErrorStatus("cannot retrieve annotation $0", index);
  }
  return absl::make_unique<Annotation>(owning_document_, annot);
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

StatusOr<std::unique_ptr<ClientBitmap>> Page::Render(float scale) const {
  MaybeGenerateContent();
  int w = static_cast<int>(std::round(scale * Bounds().Width()));
  int h = static_cast<int>(std::round(scale * Bounds().Height()));
  int num_pixels = w * h;
  if (num_pixels <= 0) {
    return ErrorStatus("invalid image size requested with area of $0", w * h);
  }
  // Correct aspect ratio for rotated pages.
  const int degrees = RotationDegrees();
  if (degrees == 90 || degrees == 270) {
    std::swap(w, h);
  }
  std::unique_ptr<ClientBitmap> out = absl::make_unique<RawClientBitmap>(
      ImageSize(w, h), ImageFormat::BITMAP_FORMAT_RGBA_8888);
  // Does not take ownership of buffer.
  FPDF_BITMAP bitmap =
      FPDFBitmap_CreateEx(w, h, FPDFBitmap_BGRA, out->imageByteData(), w * 4);
  FPDFBitmap_FillRect(bitmap, 0, 0, w, h, 0xFFFFFFFF);
  FPDF_RenderPageBitmap(bitmap, page_.get(), 0, 0, w, h, 0, kRenderFlags);
  form_renderer_->RenderTile(bitmap, page_.get(), w, h, 0, 0);
  FPDFBitmap_Destroy(bitmap);
  client_bitmap::ConvertBGRAToRGBA(out.get());
  return out;
}

Status Page::RenderTile(const Rect& source_region, ClientBitmap* out) const {
  assert(out);
  const ImageSize& bitmap_size = out->sizeInPx();
  if (bitmap_size.width != bitmap_size.height) {
    return ErrorStatus(StatusCode::INVALID_ARGUMENT,
                       "require square bitmap, got $0", bitmap_size);
  }
  if (out->bytesPerTexel() != 3) {
    return ErrorStatus(StatusCode::INVALID_ARGUMENT,
                       "require bitmap without alpha channel");
  }
  MaybeGenerateContent();
  int size = bitmap_size.width;

  // Does not take ownership of buffer.
  FPDF_BITMAP bitmap =
      FPDFBitmap_CreateEx(size, size, FPDFBitmap_BGR, out->imageByteData(),
                          size * out->bytesPerTexel());

  float scale = size / source_region.Width();

  // Our actual bounds when rendered depends on our display rotation.
  const Rect& bounds = geometry::Transform(
      Bounds(),
      matrix_utils::RotateAboutPoint(RotationRadians(), Bounds().Center()));

  // Scale the dimensions of the page, and determine the pixel offset of the
  // scaled page relative to the destination bitmap. In other words, we imagine
  // placing the dest bitmap at the upper left corner of the scaled page, then
  // moving the scaled page (tx, ty) pixels while keeping the dest stationary!
  int tx = std::round(scale * (bounds.Left() - source_region.Left()));
  int ty = std::round(scale * -(bounds.Top() - source_region.Top()));

  // Fill the tile with white.
  FPDFBitmap_FillRect(bitmap, 0, 0, size, size, 0xFFFFFFFF);
  const float page_width = std::round(scale * bounds.Width());
  float page_height = std::round(scale * bounds.Height());
  FPDF_RenderPageBitmap(bitmap, page_.get(), tx, ty, page_width, page_height, 0,
                        kRenderFlags);
  form_renderer_->RenderTile(bitmap, page_.get(), page_width, page_height, tx,
                             ty);
  FPDFBitmap_Destroy(bitmap);
  client_bitmap::ConvertBGRToRGB(out);

  return OkStatus();
}

StatusOr<TextPage*> Page::GetTextPage() {
  if (!text_page_) {
    FPDF_TEXTPAGE text_page = FPDFText_LoadPage(page_.get());
    if (!text_page) {
      return ErrorStatus("Cannot load text page from PDF");
    }
    text_page_ = absl::make_unique<TextPage>(text_page);
    INK_RETURN_UNLESS((text_page_)->GenerateIndex());
  }
  return text_page_.get();
}

Status Page::AddDebugRectangle(const Rect& r, Color stroke, Color fill,
                               const StrokeMode& s, const FillMode& f) {
  Path path(owning_document_, r.Leftbottom());
  INK_RETURN_UNLESS(path.LineTo(r.Lefttop()));
  INK_RETURN_UNLESS(path.LineTo(r.Righttop()));
  INK_RETURN_UNLESS(path.LineTo(r.Rightbottom()));
  INK_RETURN_UNLESS(path.Close());

  INK_RETURN_UNLESS(path.SetStrokeMode(s));
  INK_RETURN_UNLESS(path.SetFillMode(f));
  INK_RETURN_UNLESS(path.SetStrokeColor(stroke));
  INK_RETURN_UNLESS(path.SetFillColor(fill));
  AppendObject(path);
  return OkStatus();
}

}  // namespace pdf
}  // namespace ink
