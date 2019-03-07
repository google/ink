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
static constexpr size_t kMaxPageCacheSize = 3;

namespace ink {
namespace pdf {

static constexpr int kMaxImageDimension = 2000;

// static
StatusOr<std::unique_ptr<Document>> Document::CreateDocument(
    absl::string_view pdf_data) {
  FPDF_InitLibrary();  // Looking at the source, seems idempotent.

  std::string storage(pdf_data);
  FPDF_DOCUMENT pdf_document =
      FPDF_LoadMemDocument(storage.data(), storage.size(), kNoPassword);
  if (!pdf_document) {
    return ErrorStatus(
        absl::Substitute("pdfium could not read the given data (error $0)",
                         FPDF_GetLastError()));
  }
  return std::unique_ptr<Document>(
      new Document(pdf_document, std::move(storage)));
}

// static
StatusOr<std::unique_ptr<Document>> Document::CreateDocument() {
  FPDF_InitLibrary();  // Looking at the source, seems idempotent.
  FPDF_DOCUMENT pdf_document = FPDF_CreateNewDocument();
  if (!pdf_document) {
    return ErrorStatus(
        absl::Substitute("pdfium could not create a new document (error $0)",
                         FPDF_GetLastError()));
  }
  return std::unique_ptr<Document>(new Document(pdf_document));
}

StatusOr<std::unique_ptr<Document>> Document::CreateCopy() {
  INK_ASSIGN_OR_RETURN(auto dest, CreateDocument());
  if (!FPDF_ImportPages(dest->doc_.get(), doc_.get(), nullptr, 0)) {
    return ErrorStatus(
        absl::Substitute("could not copy pages to new document (error $0)",
                         FPDF_GetLastError()));
  }
  return dest;
}

std::shared_ptr<Page> Document::CacheAndWrapPage(int index,
                                                 FPDF_PAGE pdfium_page) {
  absl::MutexLock lock(&page_cache_mutex_);
  while (page_cache_.size() > kMaxPageCacheSize - 1 &&
         page_cache_.back().second.unique()) {
    SLOG(SLOG_PDF, "evicting page $0", page_cache_.back().first);
    page_cache_.pop_back();
  }
  auto shared_page =
      std::make_shared<Page>(pdfium_page, doc_.get(), form_renderer_);
  page_cache_.emplace_front(std::make_pair(index, shared_page));
  return shared_page;
}

StatusOr<std::shared_ptr<Page>> Document::GetPage(int index) {
  if (index < 0 || index >= PageCount()) {
    return ErrorStatus("requested page $0, but page count is $1", index,
                       PageCount());
  }

  {
    absl::MutexLock lock(&page_cache_mutex_);
    for (auto it = page_cache_.begin(); it != page_cache_.end(); it++) {
      if (it->first == index) {
        SLOG(SLOG_PDF, "cache hit for page $0", index);
        const auto& result = it->second;
        page_cache_.splice(page_cache_.begin(), page_cache_, it);
        return result;
      }
    }
  }
  SLOG(SLOG_PDF, "cache miss for page $0", index);

  FPDF_PAGE page = FPDF_LoadPage(doc_.get(), index);
  if (!page) {
    return ErrorStatus("pdfium could not load page $0", index);
  }
  return CacheAndWrapPage(index, page);
}

StatusOr<std::shared_ptr<Page>> Document::CreatePage(glm::vec2 size) {
  if (size.x * size.y <= 0) {
    return ErrorStatus("requested invalid size ($0,$1)", size.x, size.y);
  }
  FPDF_PAGE page = FPDFPage_New(doc_.get(), PageCount(), size.x, size.y);
  if (!page) {
    return ErrorStatus("pdfium could not create page $0", PageCount());
  }
  return CacheAndWrapPage(PageCount() - 1, page);
}

StatusOr<Rect> Document::GetPageBounds(int index) {
  if (index < 0 || index >= PageCount()) {
    return ErrorStatus("requested page $0, but page count is $1", index,
                       PageCount());
  }
  FPDF_PAGE page = FPDF_LoadPage(doc_.get(), index);
  if (!page) {
    return ErrorStatus("pdfium could not load page $0", index);
  }
  Page tmp(page, doc_.get(), form_renderer_);
  return tmp.Bounds();
}

int Document::PageCount() { return FPDF_GetPageCount(doc_.get()); }

StatusOr<std::unique_ptr<Text>> Document::CreateText(
    absl::string_view utf8_text, absl::string_view font_name, float font_size) {
  FPDF_PAGEOBJECT text_pageobject = FPDFPageObj_NewTextObj(
      doc_.get(), std::string(font_name).c_str(), font_size);
  if (!text_pageobject) {
    return ErrorStatus(StatusCode::INTERNAL, "Could not create text object");
  }
  auto wtext = internal::Utf8ToUtf16LE(utf8_text);
  wtext.push_back(0);
  RETURN_IF_PDFIUM_ERROR(FPDFText_SetText(
      text_pageobject, reinterpret_cast<FPDF_WIDESTRING>(wtext.data())));
  return absl::make_unique<Text>(doc_.get(), text_pageobject);
}

StatusOr<std::unique_ptr<Path>> Document::CreatePath(glm::vec2 start_point) {
  FPDF_PAGEOBJECT path_pageobject =
      FPDFPageObj_CreateNewPath(start_point.x, start_point.y);
  if (!path_pageobject) {
    return ErrorStatus(StatusCode::INTERNAL, "Could not create path object");
  }
  return absl::make_unique<Path>(doc_.get(), path_pageobject);
}

StatusOr<std::unique_ptr<Image>> Document::CreateImage(
    const ClientBitmap& ink_bitmap) {
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
  return absl::make_unique<Image>(doc_.get(), image_object);
}

}  // namespace pdf
}  // namespace ink
