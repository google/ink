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

#include "ink/public/contrib/import.h"

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/algorithms/transform.h"
#include "ink/engine/util/proto/serialize.h"

namespace ink {
namespace contrib {

using glm::vec2;

namespace {
// Instantiate pages and register with the page manager and the document.
Status AddPagesFromExportedDoc(
    const proto::ExportedDocument& unsafe_exported_doc,
    std::vector<glm::mat4>* original_page_transforms, SEngine* engine) {
  auto* page_manager = engine->registry()->Get<PageManager>();
  for (int i = 0; i < unsafe_exported_doc.page().size(); ++i) {
    auto& unsafe_page = unsafe_exported_doc.page(i);
    // Add the page to the page manager.
    Rect unsafe_page_bounds;
    if (!util::ReadFromProto(unsafe_page.bounds(), &unsafe_page_bounds)) {
      return ErrorStatus("Page $0 had bad bounds.", i);
    }
    if (unsafe_page_bounds.Area() == 0) {
      return ErrorStatus("Page $0 has invalid bounds $1", i,
                         unsafe_page_bounds.ToString());
    }

    // At this point unsafe_page_bounds is safe.
    const Rect& safe_page_bounds = unsafe_page_bounds;
    if (!page_manager->AddNewPageWithDimensions(safe_page_bounds.Dim())) {
      return ErrorStatus("Could not add page $0", i);
    }

    // We are storing data in the page manager assuming the coordinate
    // system is 0,0 on the lower left point of the page. Keep track of the
    // transform if that isn't the case.
    Rect actual(vec2(0, 0), safe_page_bounds.Dim());
    original_page_transforms->push_back(
        safe_page_bounds.CalcTransformTo(actual));

    // Add the page to the document, to be persisted. Get the page spec from
    // from the page manager as the UUID has been filled in.
    PageSpec page_spec = page_manager->GetPageSpec(i);
    proto::PerPageProperties page_proto;
    page_proto.set_uuid(page_spec.uuid);
    page_proto.set_width(safe_page_bounds.Width());
    page_proto.set_height(safe_page_bounds.Height());
    INK_RETURN_UNLESS(engine->document()->AddPage(page_proto));
  }
  // Layout the pages to ensure the scene knows about it.
  page_manager->GenerateLayout();

  ink::proto::Rect bounds;
  ink::util::WriteToProto(&bounds, page_manager->GetFullBounds());
  if (!engine->document()->SetPageBounds(bounds)) {
    return ErrorStatus("invalid bounds provided by page manager");
  }
  return OkStatus();
}

// Create image rects for each page in the doc according to the
// per_page_uri_format. See ImportFromExportedDocument for details.
void AddImagesForExportedDoc(int num_pages,
                             ImportedPageBackgroundType page_background_type,
                             absl::string_view per_page_uri_format,
                             SEngine* engine) {
  if (per_page_uri_format.empty() ||
      per_page_uri_format.find("$0") == std::string::npos) {
    // Nothing to do.
    return;
  }
  auto* page_manager = engine->registry()->Get<PageManager>();
  // Generate image elements per page.
  for (int i = 0; i < num_pages; ++i) {
    auto img_uri = absl::Substitute(per_page_uri_format, i);
    if (img_uri.empty()) {
      continue;
    }
    auto& page_info = page_manager->GetPageInfo(i);
    proto::ImageRect image_proto;
    image_proto.set_bitmap_uri(img_uri);
    Rect actual(vec2(0, 0), page_info.bounds.Dim());
    util::WriteToProto(image_proto.mutable_rect(), actual);
    image_proto.mutable_attributes()->set_is_zoomable(
        page_background_type == ImportedPageBackgroundType::ZOOMABLE_TILES);
    image_proto.mutable_attributes()->set_selectable(false);
    image_proto.mutable_attributes()->set_magic_erasable(false);
    image_proto.set_group_uuid(page_info.page_spec.uuid);
    engine->addImageRect(image_proto);
  }
}
}  // namespace

Status ImportFromExportedDocument(
    const proto::ExportedDocument& unsafe_exported_doc, SEngine* engine) {
  return ImportFromExportedDocument(
      unsafe_exported_doc, ImportedPageBackgroundType::BITMAP, "", engine);
}

Status ImportFromExportedDocument(
    const proto::ExportedDocument& unsafe_exported_doc,
    ImportedPageBackgroundType page_background_type,
    absl::string_view per_page_uri_format, SEngine* engine) {
  auto* page_manager = engine->registry()->Get<PageManager>();
  page_manager->Clear();

  // Don't put any of this initialization into the undo stack.
  engine->document()->SetUndoEnabled(false);
  engine->document()->SetMutationEventsEnabled(false);

  engine->setOutlineExportEnabled(true);

  std::vector<glm::mat4> original_page_transforms;
  INK_RETURN_UNLESS(AddPagesFromExportedDoc(unsafe_exported_doc,
                                            &original_page_transforms, engine));

  AddImagesForExportedDoc(unsafe_exported_doc.page_size(), page_background_type,
                          per_page_uri_format, engine);

  auto xform_outline_to_page = [](const proto::VectorElement& unsafe_element,
                                  const glm::mat4& page_transform) {
    // Normalize all points to the current layout.
    proto::StrokeOutline out;
    auto& current = unsafe_element.outline();
    for (int i = 0; i < current.x().size(); ++i) {
      auto coord =
          geometry::Transform(vec2(current.x(i), current.y(i)), page_transform);
      out.add_x(coord.x);
      out.add_y(coord.y);
    }
    out.set_rgba(current.rgba());
    return out;
  };

  for (size_t i = 0; i < unsafe_exported_doc.element_size(); i++) {
    const auto& unsafe_element = unsafe_exported_doc.element(i);
    if (!unsafe_element.has_outline()) {
      SLOG(SLOG_WARNING,
           "don't know how to import non-outline, skipping imported element $0",
           i);
      continue;
    }
    if (unsafe_element.outline().x_size() !=
        unsafe_element.outline().y_size()) {
      return ErrorStatus("x size != y size for element $0", i);
    }
    // Note: AddStrokeOutline assumes that the input is unsafe.
    if (unsafe_element.has_page_index()) {
      uint64_t page_index = unsafe_element.page_index();
      if (page_index >= original_page_transforms.size()) {
        return ErrorStatus("Invalid page index found: $0", page_index);
      }
      const auto& page_transform = original_page_transforms[page_index];
      auto group = page_manager->GetPageGroupId(unsafe_element.page_index());
      engine->root()->AddStrokeOutline(
          xform_outline_to_page(unsafe_element, page_transform), group,
          SourceDetails::FromEngine());
    } else {
      // This is an outline without a page.
      engine->root()->AddStrokeOutline(unsafe_element.outline(),
                                       /* group= */ kInvalidElementId,
                                       SourceDetails::FromEngine());
    }
  }

  // Element creation tasks have been launched. As soon as they're finished,
  // turn the undo manager back on.
  engine->registry()->Get<ITaskRunner>()->PushTask(
      absl::make_unique<LambdaTask>(nullptr, [engine]() {
        engine->document()->SetUndoEnabled(true);
        engine->document()->SetMutationEventsEnabled(true);
      }));
  return OkStatus();
}

}  // namespace contrib
}  // namespace ink
