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

#ifndef INK_PUBLIC_CONTRIB_IMPORT_H_
#define INK_PUBLIC_CONTRIB_IMPORT_H_

#include "third_party/absl/strings/string_view.h"
#include "ink/engine/public/sengine.h"
#include "ink/engine/public/types/status.h"
#include "ink/proto/export_portable_proto.pb.h"

namespace ink {
namespace contrib {

// Used to specify page background renderer for imported documents.
enum class ImportedPageBackgroundType { BITMAP, ZOOMABLE_TILES };

// Add the given outline strokes and pages to the given SEngine's current scene
// and document.
//
// Caveats:
// This function resizes the current Document to match the layout bounds
// after the pages have been added to the page manager.
// This function removes all existing pages and adds new pages to the
// document.
//
// If page_background_type is ZOOMABLE_TILES, then the ZoomableRectRenderer
// will be used to render each page background.
//
// This API is new and provisional.
// Note that after this call there will be pages in the page manager (if pages
// were discovered) and that it is the caller's responsibility to set the page
// strategy.
//
// Set per_page_uri_format to a non-empty string containing a "$0" substring
// to load background textures for each page.  The image data for each page
// URI must be provided in advance or with the Host::RequestImage callback.
// For example: "texture://page$0" will generate:
// "texture://page0", "texture://page1", etc.
// Also updates the pages bounds to the final result of the page layout.
Status ImportFromExportedDocument(
    const proto::ExportedDocument& unsafe_exported_doc,
    ImportedPageBackgroundType page_background_type,
    absl::string_view per_page_uri_format, SEngine* engine);

// Import the given document with BITMAP page backgrounds and an empty
// per_page_uri-format.
Status ImportFromExportedDocument(
    const proto::ExportedDocument& unsafe_exported_doc, SEngine* engine);
}  // namespace contrib
}  // namespace ink

#endif  // INK_PUBLIC_CONTRIB_IMPORT_H_
