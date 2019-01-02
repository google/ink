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

#include "ink/pdf/pdf_engine_wrapper.h"

#include "third_party/absl/memory/memory.h"
#include "third_party/absl/strings/numbers.h"
#include "third_party/absl/strings/substitute.h"
#include "ink/engine/geometry/algorithms/transform.h"
#include "ink/engine/geometry/primitives/matrix_utils.h"
#include "ink/engine/rendering/zoom_spec.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"

namespace ink {
namespace pdf {

static constexpr size_t kMaxPageCacheSize = 3;
static constexpr char kUriPrefix[] = "pdf_page://";

/* static */ std::string PdfEngineWrapper::CreateUriFormatString(
    absl::string_view page_number_format) {
  return absl::Substitute("$0$1", kUriPrefix, page_number_format);
}

namespace {
bool IsPdfPageSpec(absl::string_view uri) { return uri.find(kUriPrefix) == 0; }
}  // namespace

PdfEngineWrapper::PdfEngineWrapper(std::unique_ptr<Document> doc)
    : doc_(std::move(doc)) {}

Status PdfEngineWrapper::FindOrOpenPage(int n, Page** page) const {
  for (const auto& p : page_cache_) {
    if (p.first == n) {
      SLOG(SLOG_PDF, "cache hit for page $0", n);
      *page = p.second.get();
      return OkStatus();
    }
  }
  SLOG(SLOG_PDF, "cache miss for page $0", n);
  if (page_cache_.size() > kMaxPageCacheSize - 1) {
    SLOG(SLOG_PDF, "evicting page $0", page_cache_.back().first);
    page_cache_.pop_back();
  }
  std::unique_ptr<Page> new_page;
  INK_RETURN_UNLESS(doc_->GetPage(n, &new_page));
  *page = new_page.get();
  page_cache_.emplace_front(std::make_pair(n, std::move(new_page)));
  return OkStatus();
}

Status PdfEngineWrapper::HandleTileRequest(absl::string_view uri,
                                           ClientBitmap* out) const {
  if (!IsPdfPageSpec(uri)) {
    return ErrorStatus("$0 is not a page spec", uri);
  }
  // skip the uri prefix
  uri = uri.substr(strlen(kUriPrefix));
  size_t ques = uri.find("?");
  if (ques == absl::string_view::npos) {
    return ErrorStatus("expected a ? to mark an uri param in <$0>", uri);
  }
  int32_t n;
  if (!absl::SimpleAtoi(uri.substr(0, ques), &n)) {
    return ErrorStatus("expected an integer page number at <$0>",
                       uri.substr(0, ques));
  }
  uri = uri.substr(ques + 1);
  ZoomSpec zoom_spec;
  INK_RETURN_UNLESS(ZoomSpec::FromUri(uri, &zoom_spec));

  Page* page;
  INK_RETURN_UNLESS(FindOrOpenPage(n, &page));

  const auto& target = zoom_spec.Apply(geometry::Transform(
      page->Bounds(), matrix_utils::RotateAboutPoint(page->RotationRadians(),
                                                     page->Bounds().Center())));
  return page->RenderTile(target, out);
}

bool PdfEngineWrapper::CanHandleTextureRequest(absl::string_view uri) const {
  return IsPdfPageSpec(uri);
}

std::string PdfEngineWrapper::ToString() const { return "PdfEngineWrapper"; }

Status PdfEngineWrapper::GetSelection(glm::vec2 start_world,
                                      glm::vec2 end_world,
                                      const PageManager& page_manager,
                                      std::vector<Rect>* out) const {
  out->clear();

  // Get the page indices of the start and end world coordinates.
  GroupId const start_id =
      page_manager.GetPageGroupForRect(Rect(start_world, start_world));
  if (start_id == kInvalidElementId) {
    return ErrorStatus("no page found for start coordinate $0", start_world);
  }
  int start_page = page_manager.GetPageInfo(start_id).page_index;

  GroupId const end_id =
      page_manager.GetPageGroupForRect(Rect(end_world, end_world));
  if (end_id == kInvalidElementId) {
    return ErrorStatus("no page found for end coordinate $0", end_world);
  }
  int end_page = page_manager.GetPageInfo(end_id).page_index;

  if (start_page > end_page) {
    std::swap(start_page, end_page);
    std::swap(start_world, end_world);
  }

  Page* page;
  TextPage* text_page;

  // Start point and end point are on the same page.
  if (start_page == end_page) {
    // Transform from world coordinates to page coordinates.
    INK_RETURN_UNLESS(FindOrOpenPage(start_page, &page));
    INK_RETURN_UNLESS(page->GetTextPage(&text_page));
    auto page_to_world =
        glm::translate(page_manager.GetPageInfo(start_page).transform,
                       -glm::vec3(page->CropBox().Leftbottom(), 0));
    const auto& world_to_page = glm::inverse(page_to_world);
    text_page->GetSelectionRects(
        geometry::Transform(start_world, world_to_page),
        geometry::Transform(end_world, world_to_page), page_to_world, out);
    return OkStatus();
  }

  // Start point and end point are on different pages.
  for (int i = start_page; i <= end_page; ++i) {
    INK_RETURN_UNLESS(FindOrOpenPage(i, &page));
    INK_RETURN_UNLESS(page->GetTextPage(&text_page));
    auto page_to_world =
        glm::translate(page_manager.GetPageInfo(i).transform,
                       -glm::vec3(page->CropBox().Leftbottom(), 0));
    const auto& world_to_page = glm::inverse(page_to_world);
    if (i == start_page) {
      text_page->GetSelectionRectsToEnd(
          geometry::Transform(start_world, world_to_page), page_to_world, out);
    } else if (i == end_page) {
      text_page->GetSelectionRectsFromBeginning(
          geometry::Transform(end_world, world_to_page), page_to_world, out);
    } else {
      text_page->GetSelectionRects(page_to_world, out);
    }
  }
  return OkStatus();
}

bool PdfEngineWrapper::IsInText(glm::vec2 world,
                                const PageManager& page_manager) const {
  GroupId const group_id = page_manager.GetPageGroupForRect(Rect(world, world));
  if (group_id == kInvalidElementId) {
    return false;
  }
  int start_page = page_manager.GetPageInfo(group_id).page_index;
  Page* page;
  if (!FindOrOpenPage(start_page, &page)) return false;

  TextPage* text_page;
  if (!page->GetTextPage(&text_page)) return false;

  auto page_to_world =
      glm::translate(page_manager.GetPageInfo(start_page).transform,
                     -glm::vec3(page->CropBox().Leftbottom(), 0));
  return text_page->IsInText(
      geometry::Transform(world, glm::inverse(page_to_world)));
}

std::vector<Rect> PdfEngineWrapper::GetCandidateRects(
    glm::vec2 world, const PageManager& page_manager) const {
  GroupId const group_id = page_manager.GetPageGroupForRect(Rect(world, world));
  if (group_id == kInvalidElementId) {
    return {};
  }
  int start_page = page_manager.GetPageInfo(group_id).page_index;
  Page* page;
  if (!FindOrOpenPage(start_page, &page)) return {};

  TextPage* text_page;
  if (!page->GetTextPage(&text_page)) return {};

  auto page_to_world =
      glm::translate(page_manager.GetPageInfo(start_page).transform,
                     -glm::vec3(page->CropBox().Leftbottom(), 0));
  auto candidates = text_page->CandidatesAt(
      geometry::Transform(world, glm::inverse(page_to_world)));
  if (candidates.empty()) return {};
  std::vector<Rect> world_rects(candidates.size());
  for (const auto& c : candidates) {
    world_rects.push_back(
        geometry::Transform(text_page->CandidateRect(c), page_to_world));
  }
  return world_rects;
}

}  // namespace pdf
}  // namespace ink
