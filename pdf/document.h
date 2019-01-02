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

#ifndef INK_PDF_DOCUMENT_H_
#define INK_PDF_DOCUMENT_H_

#include <cassert>
#include <memory>
#include <string>

#include "third_party/absl/strings/string_view.h"
#include "third_party/glm/glm/glm.hpp"
#include "third_party/pdfium/public/cpp/fpdf_scopers.h"
#include "third_party/pdfium/public/fpdf_save.h"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/public/types/client_bitmap.h"
#include "ink/engine/public/types/status.h"
#include "ink/pdf/image_object.h"
#include "ink/pdf/page.h"
#include "ink/pdf/path_object.h"
#include "ink/pdf/text_object.h"

namespace ink {
namespace pdf {

namespace internal {
// This thing takes i/o callbacks from pdfium's document serializer and stashes
// the bytes in a string.
template <typename String>
struct StringSaver : public FPDF_FILEWRITE {
  static int WriteBlockCallback(
      FPDF_FILEWRITE* pFileWrite, const void* data,
      unsigned long size) {
    StringSaver* pThis = static_cast<StringSaver*>(pFileWrite);
    pThis->buf->append(static_cast<const char*>(data), size);
    return 1;
  }

  explicit StringSaver(String* s) : buf(s) {
    FPDF_FILEWRITE::version = 1;
    FPDF_FILEWRITE::WriteBlock = WriteBlockCallback;
  }

 private:
  String* buf;
};
}  // namespace internal

// A Document is the top-level PDF object, representing the contents of a PDF
// document. Through this object, you can get and create Pages, and create
// objects (such as Text and Path instances) to add to existing Pages.
class Document {
 public:
  // Create a Document from the given serialized PDF.
  // If the return value is ok(), populates the given unique Document pointer.
  static Status CreateDocument(absl::string_view pdf_data,
                               std::unique_ptr<Document>* doc_ptr);

  // Create an empty Document.
  // If the return value is ok(), populates the given unique Document pointer.
  static Status CreateDocument(std::unique_ptr<Document>* doc_ptr);

  // Return the number of pages in this Document.
  int PageCount();

  // Sets *page_ptr to the page at the given index.
  Status GetPage(int index, std::unique_ptr<Page>* page_ptr);

  // Sets *out to a Page with the given dimensions. The MediaBox of the created
  // page will span 0,0->size.w,size.h.
  Status CreatePage(glm::vec2 size, std::unique_ptr<Page>* out);

  // Sets *out to the bounds of the page at the given index.
  // *out will be a Rect as described in Page::Bounds(), above.
  Status GetPageBounds(int index, Rect* out);

  // Serializes this Document into the given string.
  template <typename String>
  Status Write(String* out) const {
    out->clear();
    internal::StringSaver<String> saver(out);
    if (!FPDF_SaveAsCopy(doc_.get(), &saver, 0)) {
      out->clear();
      return ErrorStatus(
          absl::Substitute("pdfium could not save the document (error $0)",
                           FPDF_GetLastError()));
    }
    return OkStatus();
  }

  // Sets *out to a Text per the given specification.
  Status CreateText(absl::string_view utf8_text, absl::string_view font_name,
                    float font_size, std::unique_ptr<Text>* out);

  // Sets *out to a Path with the given start point.
  Status CreatePath(glm::vec2 start_point, std::unique_ptr<Path>* out);

  // Sets *out to an Image, copied from the given ClientBitmap.
  Status CreateImage(const ClientBitmap& ink_bitmap,
                     std::unique_ptr<Image>* out);

  // Creates a copy of this document, storing the copy in the given unique_ptr.
  Status CopyInto(std::unique_ptr<Document>* dest);

 private:
  Document(FPDF_DOCUMENT doc, std::string storage)
      : doc_(doc), doc_storage_(std::move(storage)) {
    assert(doc);
  }
  explicit Document(FPDF_DOCUMENT doc) : Document(doc, "") {}

  ScopedFPDFDocument doc_;
  // The pdfium API requires that the raw string from which the Document is
  // parsed have a lifetime at least as long as the Document's.
  std::string doc_storage_;
};

}  // namespace pdf
}  // namespace ink
#endif  // INK_PDF_DOCUMENT_H_
