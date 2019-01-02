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

#include "ink/pdf/document.h"

#include <array>
#include <cstdint>
#include <vector>

#include "third_party/absl/memory/memory.h"
#include "third_party/absl/strings/substitute.h"
#include "third_party/pdfium/public/fpdf_ppo.h"
#include "third_party/pdfium/public/fpdfview.h"
#include "ink/pdf/internal.h"

static const char* kNoPassword = nullptr;

namespace ink {
namespace pdf {

static constexpr int kMaxImageDimension = 2000;

// static
Status Document::CreateDocument(absl::string_view pdf_data,
                                std::unique_ptr<Document>* doc_ptr) {
  FPDF_InitLibrary();  // Looking at the source, seems idempotent.

  std::string storage(pdf_data);
  FPDF_DOCUMENT pdf_document =
      FPDF_LoadMemDocument(storage.data(), storage.size(), kNoPassword);
  if (!pdf_document) {
    return ErrorStatus(
        absl::Substitute("pdfium could not read the given data (error $0)",
                         FPDF_GetLastError()));
  }
  *doc_ptr =
      std::unique_ptr<Document>(new Document(pdf_document, std::move(storage)));
  return OkStatus();
}

// static
Status Document::CreateDocument(std::unique_ptr<Document>* doc_ptr) {
  FPDF_InitLibrary();  // Looking at the source, seems idempotent.
  FPDF_DOCUMENT pdf_document = FPDF_CreateNewDocument();
  if (!pdf_document) {
    return ErrorStatus(
        absl::Substitute("pdfium could not create a new document (error $0)",
                         FPDF_GetLastError()));
  }
  *doc_ptr = std::unique_ptr<Document>(new Document(pdf_document));
  return OkStatus();
}

Status Document::CopyInto(std::unique_ptr<Document>* dest) {
  auto status = CreateDocument(dest);
  if (!status.ok()) {
    return status;
  }
  if (!FPDF_ImportPages((*dest)->doc_.get(), doc_.get(), nullptr, 0)) {
    dest->reset();
    return ErrorStatus(
        absl::Substitute("could not copy pages to new document (error $0)",
                         FPDF_GetLastError()));
  }
  return OkStatus();
}

Status Document::GetPage(int index, std::unique_ptr<Page>* page_ptr) {
  if (index < 0 || index >= PageCount()) {
    return ErrorStatus("requested page $0, but page count is $1", index,
                       PageCount());
  }
  FPDF_PAGE page = FPDF_LoadPage(doc_.get(), index);
  if (!page) {
    return ErrorStatus("pdfium could not load page $0", index);
  }
  *page_ptr = absl::make_unique<Page>(page, doc_.get());
  return OkStatus();
}

Status Document::CreatePage(glm::vec2 size, std::unique_ptr<Page>* out) {
  if (size.x * size.y <= 0) {
    return ErrorStatus("requested invalid size ($0,$1)", size.x, size.y);
  }
  FPDF_PAGE page = FPDFPage_New(doc_.get(), PageCount(), size.x, size.y);
  if (!page) {
    return ErrorStatus("pdfium could not create page $0", PageCount());
  }
  *out = absl::make_unique<Page>(page, doc_.get());
  return OkStatus();
}

Status Document::GetPageBounds(int index, Rect* out) {
  *out = Rect();
  if (index < 0 || index >= PageCount()) {
    return ErrorStatus("requested page $0, but page count is $1", index,
                       PageCount());
  }
  FPDF_PAGE page = FPDF_LoadPage(doc_.get(), index);
  if (!page) {
    return ErrorStatus("pdfium could not load page $0", index);
  }
  auto tmp = absl::make_unique<Page>(page, doc_.get());
  *out = tmp->Bounds();
  return OkStatus();
}

int Document::PageCount() { return FPDF_GetPageCount(doc_.get()); }

Status Document::CreateText(absl::string_view utf8_text,
                            absl::string_view font_name, float font_size,
                            std::unique_ptr<Text>* out) {
  assert(out);
  FPDF_PAGEOBJECT text_pageobject = FPDFPageObj_NewTextObj(
      doc_.get(), std::string(font_name).c_str(), font_size);
  if (!text_pageobject) {
    return ErrorStatus(StatusCode::INTERNAL, "Could not create text object");
  }
  auto wtext = internal::Utf8ToUtf16LE(utf8_text);
  wtext.push_back(0);
  RETURN_IF_PDFIUM_ERROR(FPDFText_SetText(
      text_pageobject, reinterpret_cast<FPDF_WIDESTRING>(wtext.data())));
  *out = absl::make_unique<Text>(doc_.get(), text_pageobject);
  return OkStatus();
}

Status Document::CreatePath(glm::vec2 start_point, std::unique_ptr<Path>* out) {
  FPDF_PAGEOBJECT path_pageobject =
      FPDFPageObj_CreateNewPath(start_point.x, start_point.y);
  if (!path_pageobject) {
    return ErrorStatus(StatusCode::INTERNAL, "Could not create path object");
  }
  *out = absl::make_unique<Path>(doc_.get(), path_pageobject);
  return OkStatus();
}

Status Document::CreateImage(const ClientBitmap& ink_bitmap,
                             std::unique_ptr<Image>* out) {
  FPDF_PAGEOBJECT image_object = FPDFPageObj_NewImageObj(doc_.get());
  if (!image_object) {
    return ErrorStatus(StatusCode::INTERNAL,
                       "could not create new pdf image object");
  }
  int width = ink_bitmap.sizeInPx().width;
  int height = ink_bitmap.sizeInPx().height;
  if (width > kMaxImageDimension || height > kMaxImageDimension) {
    return ErrorStatus(StatusCode::INVALID_ARGUMENT,
                       "max image dimension is $0, but given image is $1x$2",
                       kMaxImageDimension, width, height);
  }
  const int stride = width * 4;
  std::vector<uint8_t> buffer(stride * height);
  ScopedFPDFBitmap pdf_bitmap(FPDFBitmap_CreateEx(
      width, height, FPDFBitmap_BGRA, buffer.data(), stride));
  if (!pdf_bitmap) {
    return ErrorStatus(StatusCode::INTERNAL,
                       "could not create new pdf bitmap object");
  }
  ImageFormat format = ink_bitmap.format();
  const size_t bytes_per_pixel = bytesPerTexelForFormat(format);
  const uint8_t* in_channels =
      static_cast<const uint8_t*>(ink_bitmap.imageByteData());
  uint8_t* out_channels = buffer.data();
  for (int i = 0; i < width * height; i++, in_channels += bytes_per_pixel) {
    uint32_t pixel;
    if (!expandTexelToRGBA8888(format, in_channels,
                               in_channels + bytes_per_pixel, &pixel)) {
      return ErrorStatus(StatusCode::INTERNAL, "could not decode $0 pixel",
                         format);
    }
    *(out_channels++) = (pixel >> 8) & 0xFF;
    *(out_channels++) = (pixel >> 16) & 0xFF;
    *(out_channels++) = (pixel >> 24) & 0xFF;
    *(out_channels++) = pixel & 0xFF;
  }
  std::array<FPDF_PAGE, 1> dummy_affected_pages{nullptr};
  if (!FPDFImageObj_SetBitmap(dummy_affected_pages.data(), 0, image_object,
                              pdf_bitmap.get())) {
    return ErrorStatus(StatusCode::INTERNAL,
                       "could not copy bitmap into pdf image");
  }
  *out = absl::make_unique<Image>(doc_.get(), image_object);
  return OkStatus();
}

}  // namespace pdf
}  // namespace ink
