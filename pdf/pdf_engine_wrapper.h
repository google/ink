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

#ifndef INK_PDF_PDF_ENGINE_WRAPPER_H_
#define INK_PDF_PDF_ENGINE_WRAPPER_H_

#include <list>
#include <memory>
#include <string>

#include "third_party/absl/strings/string_view.h"
#include "ink/engine/public/types/client_bitmap.h"
#include "ink/engine/public/types/iselection_provider.h"
#include "ink/engine/public/types/itexture_request_handler.h"
#include "ink/engine/public/types/status.h"
#include "ink/pdf/document.h"
#include "ink/pdf/page.h"

namespace ink {
namespace pdf {

class PdfEngineWrapper : public ITileProvider, public ISelectionProvider {
 public:
  explicit PdfEngineWrapper(std::unique_ptr<Document> doc);
  ~PdfEngineWrapper() override {}

  bool CanHandleTextureRequest(absl::string_view uri) const override;
  Status HandleTileRequest(absl::string_view uri,
                           ClientBitmap* out) const override;

  static std::string CreateUriFormatString(
      absl::string_view page_number_format);

  // Implements ISelectionProvider().
  // Returns Error status if a page cannot be found and/or opened, if the text
  // page cannot be acquired, or if the text page cannot GenerateIndex().
  // Otherwise, returns OkStatus().
  S_WARN_UNUSED_RESULT Status GetSelection(
      glm::vec2 start_world, glm::vec2 end_world,
      const PageManager& page_manager, std::vector<Rect>* out) const override;

  bool IsInText(glm::vec2 world,
                const PageManager& page_manager) const override;

  std::vector<Rect> GetCandidateRects(
      glm::vec2 world, const PageManager& page_manager) const override;
  Document* PdfDocument() const { return doc_.get(); }

  std::string ToString() const override;

 private:
  std::unique_ptr<Document> doc_;
};

}  // namespace pdf
}  // namespace ink

#endif  // INK_PDF_PDF_ENGINE_WRAPPER_H_
