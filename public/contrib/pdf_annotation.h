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

#ifndef INK_PUBLIC_CONTRIB_PDF_ANNOTATION_H_
#define INK_PUBLIC_CONTRIB_PDF_ANNOTATION_H_

#include <string>

#include "third_party/absl/strings/string_view.h"
#include "ink/engine/public/sengine.h"
#include "ink/engine/public/types/status.h"

namespace ink {
namespace contrib {
namespace pdf {
// Loads the given bytes as a PDF document, possibly having existing Ink
// annotations in it. Clears any existing engine state, and begins annotating
// the given PDF.
Status LoadPdfForAnnotation(absl::string_view pdf_bytes, SEngine* engine);

// Renders the current scene into a copy of the currently loaded PDF, returning
// the newly annotated PDF in its serialized form.
Status GetAnnotatedPdf(const SEngine& engine, std::string* out);

// Renders the current scene into a copy of the currently loaded PDF, and sends
// the result to the host via IEngineListener.
void SendAnnotatedPdfToHost(const SEngine& engine);
}  // namespace pdf
}  // namespace contrib
}  // namespace ink

#endif  // INK_PUBLIC_CONTRIB_PDF_ANNOTATION_H_
