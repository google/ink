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

// Functions for writing exported Ink scenes into PDF documents, and for reading
// them back.

#ifndef INK_PDF_INK_PDF_IO_H_
#define INK_PDF_INK_PDF_IO_H_

#include <string>
#include <vector>

#include "third_party/absl/strings/string_view.h"
#include "ink/engine/public/types/status.h"
#include "ink/pdf/document.h"
#include "ink/proto/export_portable_proto.pb.h"

namespace ink {
namespace pdf {

extern const char kInkAnnotationIdentifierKeyV1[];
extern const char kInkAnnotationIdentifierKeyV2[];
extern const char kInkAnnotationIdentifierValue[];

// Render the given Ink export outlines into the given PDF Document.
Status Render(const ::ink::proto::ExportedDocument& exported_doc,
              pdf::Document* pdf_document);

// Given a PDF doc, read any Ink annotations on each page of the document into
// the exported_doc. The pdf_doc will be changed such that any ink annotations
// will have been removed.
// If the given ExportedDocument* is nullptr, then the given doc will just be
// stripped.
Status ReadAndStrip(pdf::Document* pdf_doc,
                    ::ink::proto::ExportedDocument* exported_doc);
}  // namespace pdf
}  // namespace ink
#endif  // INK_PDF_INK_PDF_IO_H_
