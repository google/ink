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

#include "ink/pdf/io.h"

#include <cmath>  // std::abs
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "third_party/absl/strings/substitute.h"
#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/algorithms/transform.h"
#include "ink/engine/geometry/primitives/matrix_utils.h"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/public/types/color.h"
#include "ink/engine/public/types/status.h"
#include "ink/engine/public/types/status_or.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"
#include "ink/engine/util/funcs/utils.h"
#include "ink/engine/util/proto/serialize.h"
#include "ink/public/contrib/export.h"

using ::glm::vec2;

namespace ink {
namespace pdf {

const char kInkAnnotationIdentifierKeyV1[] = "GOOG:INKIsInk";
const char kInkAnnotationIdentifierKeyV2[] = "GOOG:INKIsInker";
const char kInkAnnotationIdentifierValue[] = "true";

namespace {
Status Write(const std::vector<proto::VectorElement>& vector_elements,
             const Rect& page_world_bounds, Document* doc, Page* page) {
  SLOG(SLOG_PDF, "page scene bounds $0", page_world_bounds);

  Rect page_bounds = page->Bounds();
  SLOG(SLOG_PDF, "page bounds $0", page_bounds);

  /*
   * A PDF Page may have a non-0 display rotation, in which case we rotate the
   * Ink strokes around their world-space page center.
   */
  glm::mat4 world_to_page = matrix_utils::RotateAboutPoint(
      -page->RotationRadians(), page_world_bounds.Center());

  // Apply the world-to-page transform to the page's world Rect to correct for
  // landscape/portrait.
  Rect corrected_page_world_bounds =
      geometry::Transform(page_world_bounds, world_to_page);
  // The Ink world's page may be at a different scale from the PDF Page.
  glm::mat4 scale_correction =
      corrected_page_world_bounds.CalcTransformTo(page_bounds);
  world_to_page = scale_correction * world_to_page;

  SLOG(SLOG_PDF, "$0",
       absl::Substitute("writing $0 stroke(s) to PDF", vector_elements.size())
           .c_str());
  for (const auto& element : vector_elements) {
    if (!element.has_outline()) {
      SLOG(SLOG_WARNING, "skipping element with no outline");
      continue;
    }
    const auto& outline = element.outline();
    if (outline.x_size() < 2 || outline.x_size() != outline.y_size()) {
      SLOG(SLOG_WARNING, "skipping element with bad outline");
    }

    INK_ASSIGN_OR_RETURN(
        auto path, doc->CreatePath(geometry::Transform(
                       glm::vec2(outline.x(0), outline.y(0)), world_to_page)));
    for (int i = 1, n = outline.x_size(); i < n; i++) {
      INK_RETURN_UNLESS(path->LineTo(geometry::Transform(
          glm::vec2(outline.x(i), outline.y(i)), world_to_page)));
    }
    INK_RETURN_UNLESS(path->Close());
    INK_RETURN_UNLESS(path->SetStrokeMode(StrokeMode::kNoStroke));
    INK_RETURN_UNLESS(path->SetFillMode(FillMode::kWinding));
    INK_RETURN_UNLESS(
        path->SetFillColor(Color::FromNonPremultipliedRGBA(outline.rgba())));
    path->AddMark(kInkAnnotationIdentifierKeyV2);
    page->AppendObject(*path);
  }

  return OkStatus();
}
}  // namespace

Status Render(const ::ink::proto::ExportedDocument& exported_doc,
              pdf::Document* pdf_document) {
  if (pdf_document->PageCount() != exported_doc.page_size()) {
    return ErrorStatus(
        absl::Substitute("PDF doc has $0 pages, but exported ink doc has $1",
                         pdf_document->PageCount(), exported_doc.page_size()));
  }

  // The elements in the proto are stored flat, but we need
  // to look up elements by page.
  std::unordered_map<int, std::vector<proto::VectorElement>> elements;
  for (const auto& elem : exported_doc.element()) {
    elements[elem.page_index()].push_back(elem);
  }

  for (int i = 0; i < pdf_document->PageCount(); ++i) {
    INK_ASSIGN_OR_RETURN(auto page, pdf_document->GetPage(i));
    Rect world_bounds;
    if (!ink::util::ReadFromProto(exported_doc.page(i).bounds(),
                                  &world_bounds)) {
      return ErrorStatus(
          absl::Substitute("could not read exported page $0", i));
    }
    INK_RETURN_UNLESS(
        Write(elements[i], world_bounds, pdf_document, page.get()));
  }
  return OkStatus();
}

namespace {
// If annot_index is -1 on return, then no error occurred, but the annotation
// was not found.
StatusOr<std::unique_ptr<Annotation>> GetInkAnnotationV1(const Page& page,
                                                         int* annot_index) {
  assert(annot_index);
  *annot_index = -1;
  for (int i = 0; i < page.GetAnnotationCount(); i++) {
    INK_ASSIGN_OR_RETURN(auto annot, page.GetAnnotation(i));
    if (annot->HasKey(kInkAnnotationIdentifierKeyV1)) {
      *annot_index = i;
      return annot;
    }
  }
  return status::NotFound("no v1 ink annotation found");
}

Status ImportOnePath(const Path& path, const int page_num,
                     glm::mat4 page_to_world,
                     proto::ExportedDocument* ink_doc) {
  auto* element = ink_doc->add_element();
  element->set_page_index(page_num);
  auto* outline = element->mutable_outline();
  INK_ASSIGN_OR_RETURN(Color c, path.GetFillColor());
  outline->set_rgba(c.AsNonPremultipliedUintRGBA());
  INK_ASSIGN_OR_RETURN(std::vector<glm::vec2> coords, path.GetCoordinates());
  // pdfium explicitly adds the first coordinate at the end, but sketchology
  // implicitly closes an outline.
  coords.pop_back();
  for (const auto& coord : coords) {
    auto world_vertex = geometry::Transform(coord, page_to_world);
    outline->add_x(world_vertex.x);
    outline->add_y(world_vertex.y);
  }
  return OkStatus();
}

Status ReadAndStripV1(Page* page, const int page_num, glm::mat4 page_to_world,
                      proto::ExportedDocument* ink_doc) {
  int annot_index;
  INK_ASSIGN_OR_RETURN(auto annot, GetInkAnnotationV1(*page, &annot_index));
  auto stamp = reinterpret_cast<StampAnnotation*>(annot.get());
  for (int i = 0, n = stamp->GetPathCount(); i < n; i++) {
    std::unique_ptr<Path> path;
    auto status = stamp->GetPath(i, &path);
    if (!status.ok()) {
      continue;
    }
    INK_RETURN_UNLESS(ImportOnePath(*path, page_num, page_to_world, ink_doc));
  }
  return page->RemoveAnnotation(annot_index);
}

Status ReadAndStrip(Page* pdf_page, const int page_num,
                    proto::ExportedDocument* exported_doc) {
  auto* ink_page = exported_doc->add_page();
  /*
   * A PDF Page may have a non-0 display rotation, in which case the rectangle
   * itself and the annotation stroke coordinates may have to be rotated
   * around the page center by the inverse of the rotation.
   */
  auto page_to_world = matrix_utils::RotateAboutPoint(
      (*pdf_page).RotationRadians(), (*pdf_page).Bounds().Center());

  ink::util::WriteToProto(
      ink_page->mutable_bounds(),
      geometry::Transform(pdf_page->Bounds(), page_to_world));

  auto v1_status =
      ReadAndStripV1(pdf_page, page_num, page_to_world, exported_doc);
  // If it's ok, we're done, and we should return.
  if (v1_status.ok()) {
    return OkStatus();
  }
  // It's ok for the V1 annotation to be not-found; we continue looking for our
  // current serialization scheme. But if it was found, and there was some other
  // error, we return now.
  if (!status::IsNotFound(v1_status)) {
    return v1_status;
  }

  auto const object_count = pdf_page->PageObjectCount();
  if (object_count == 0) {
    // Nothing here.
    return OkStatus();
  }

  std::vector<std::unique_ptr<PageObject>> to_delete;
  for (int i = 0; i < object_count; i++) {
    INK_ASSIGN_OR_RETURN(auto obj, pdf_page->GetPageObject(i));
    for (int mark_index = 0, mark_count = obj->MarkCount();
         mark_index < mark_count; mark_index++) {
      INK_ASSIGN_OR_RETURN(auto mark, obj->GetMark(mark_index));
      INK_ASSIGN_OR_RETURN(auto name, mark->GetName());
      if (name != kInkAnnotationIdentifierKeyV2) continue;
      if (auto* path = dynamic_cast<Path*>(obj.get())) {
        ImportOnePath(*path, page_num, page_to_world, exported_doc);
      } else if (auto* text = dynamic_cast<Text*>(obj.get())) {
      } else {
        return ErrorStatus(StatusCode::INTERNAL,
                           "Don't know what to do with an object that's "
                           "neither a path nor a text.");
      }
      to_delete.emplace_back(obj.release());
    }
  }
  for (int i = 0; i < to_delete.size(); i++) {
    INK_RETURN_UNLESS(pdf_page->RemovePageObject(std::move(to_delete[i])));
  }

  return OkStatus();
}
}  // namespace

Status ReadAndStrip(pdf::Document* pdf_doc,
                    ::ink::proto::ExportedDocument* exported_doc) {
  EXPECT(pdf_doc);
  EXPECT(exported_doc);
  exported_doc->Clear();
  std::unique_ptr<Document> doc;
  for (int i = 0; i < pdf_doc->PageCount(); i++) {
    INK_ASSIGN_OR_RETURN(auto page, pdf_doc->GetPage(i));
    auto status = ReadAndStrip(page.get(), i, exported_doc);
    if (!status.ok()) {
      exported_doc->Clear();
      return status;
    }
  }
  return OkStatus();
}

}  // namespace pdf
}  // namespace ink
