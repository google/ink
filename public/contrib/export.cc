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

#include "ink/public/contrib/export.h"
#include "ink/engine/geometry/algorithms/transform.h"
#include "ink/engine/scene/data/common/mesh_serializer_provider.h"
#include "ink/engine/scene/data/common/stroke.h"
#include "ink/engine/scene/types/element_bundle.h"

namespace ink {

bool ink::contrib::ExtractTextBox(const ink::proto::ElementBundle& bundle,
                                  const glm::mat4& transform,
                                  ink::proto::TextBox* text_proto) {
  ink::Stroke stroke;
  if (!ink::Stroke::ReadFromProto(bundle, &stroke)) {
    return false;
  }
  auto mesh_reader = mesh::ReaderFor(stroke);
  Mesh mesh;
  if (stroke.MeshCount() == 0 || !stroke.GetMesh(*mesh_reader, 0, &mesh).ok()) {
    return false;
  }
  if (mesh.texture == nullptr) {
    return false;
  }
  if (!bundle.element().has_text() &&
      !absl::StartsWith(mesh.texture->uri, "text:")) {
    return false;
  }
  float min_x = std::numeric_limits<float>::max();
  float min_y = std::numeric_limits<float>::max();
  float max_x = std::numeric_limits<float>::lowest();
  float max_y = std::numeric_limits<float>::lowest();
  for (const auto& vert : mesh.verts) {
    auto pt = ink::geometry::Transform(vert.position, transform);
    min_x = std::min(min_x, pt.x);
    min_y = std::min(min_y, pt.y);
    max_x = std::max(max_x, pt.x);
    max_y = std::max(max_y, pt.y);
  }

  auto bounds = text_proto->mutable_bounds();
  bounds->set_xlow(min_x);
  bounds->set_ylow(min_x);
  bounds->set_xhigh(max_x);
  bounds->set_yhigh(max_y);

  if (bundle.element().has_text()) {
    *text_proto->mutable_text() = bundle.element().text();
  } else {
    text_proto->set_uri(mesh.texture->uri);
  }
  return true;
}

bool ink::contrib::ExtractStrokeOutline(
    const ink::proto::ElementBundle& bundle, const glm::mat4& transform,
    ink::proto::StrokeOutline* outline_proto) {
  if (bundle.has_element() && (bundle.element().attributes().is_group() ||
                               bundle.element().attributes().is_zoomable())) {
    return false;
  }
  // Transform and copy all the outline points
  auto stroke_proto = bundle.uncompressed_element().uncompressed_stroke();
  if (stroke_proto.outline_size() == 0) {
    SLOG(SLOG_WARNING, "Export encountered stroke $0 with no outline.", bundle);
    return false;
  }
  for (const auto& pt : stroke_proto.outline()) {
    auto tx_pt = ink::geometry::Transform(glm::vec2(pt.x(), pt.y()), transform);
    outline_proto->add_x(tx_pt.x);
    outline_proto->add_y(tx_pt.y);
  }

  outline_proto->set_rgba(stroke_proto.rgba());
  return true;
}

namespace {
enum class ExtractionResult { ADDED = 0, IGNORED = 1, FAILED = 2 };

// Helper common to ToVectorElements and ToExportedDocument.
ExtractionResult ExtractElement(const ink::proto::ElementBundle& bundle_proto,
                                ink::proto::VectorElement* element) {
  // Decode the transform
  glm::mat4 transform{1};
  if (!ink::ElementBundle::ReadObjectMatrix(bundle_proto, &transform)) {
    return ExtractionResult::FAILED;
  }

  ink::proto::TextBox text_proto;
  ink::proto::StrokeOutline outline_proto;
  if (contrib::ExtractTextBox(bundle_proto, transform, &text_proto)) {
    *element->mutable_text() = std::move(text_proto);
    return ExtractionResult::ADDED;
  } else if (contrib::ExtractStrokeOutline(bundle_proto, transform,
                                           &outline_proto)) {
    *element->mutable_outline() = std::move(outline_proto);
    return ExtractionResult::ADDED;
  }
  return ExtractionResult::IGNORED;
}
}  // namespace

bool ink::contrib::ToVectorElements(const ink::proto::Snapshot& scene,
                                    ink::proto::VectorElements* result) {
  if (scene.has_page_properties() && scene.page_properties().has_bounds()) {
    *(result->mutable_bounds()) = scene.page_properties().bounds();
  }
  int n_exported_elements = 0;
  for (auto& bundle_proto : scene.element()) {
    ink::proto::VectorElement element;
    auto res = ExtractElement(bundle_proto, &element);
    if (res == ExtractionResult::FAILED) {
      result->Clear();
      return false;  // already logged
    }
    if (res == ExtractionResult::ADDED) {
      *result->add_elements() = std::move(element);
      n_exported_elements++;
    }
  }
  if (n_exported_elements == 0) {
    SLOG(SLOG_INFO, "Zero exported elements found - empty page converted.");
  }
  return true;
}

bool ink::contrib::ToExportedDocument(const ink::proto::Snapshot& scene,
                                      ink::proto::ExportedDocument* result) {
  std::unordered_map<UUID, int> uuid_to_page;
  for (int i = 0; i < scene.per_page_properties().size(); ++i) {
    // Keep track of page UUID to index.
    auto& page = scene.per_page_properties(i);
    uuid_to_page[page.uuid()] = i;
    Rect page_bounds(0, 0, page.width(), page.height());
    // Add to the exported page list.
    auto* exported_page = result->add_page();
    ink::util::WriteToProto(exported_page->mutable_bounds(), page_bounds);
  }
  int n_exported_elements = 0;
  for (auto& bundle_proto : scene.element()) {
    if (uuid_to_page.count(bundle_proto.uuid()) > 0) {
      // This is a page element. Ignore it.
      continue;
    }
    ink::proto::VectorElement element;
    auto res = ExtractElement(bundle_proto, &element);
    if (res == ExtractionResult::FAILED) {
      result->Clear();
      return false;  // already logged
    }
    if (res == ExtractionResult::ADDED) {
      if (bundle_proto.has_group_uuid() &&
          uuid_to_page.count(bundle_proto.group_uuid()) > 0) {
        auto uuid = bundle_proto.group_uuid();
        element.set_page_index(uuid_to_page[uuid]);
      }
      *result->add_element() = std::move(element);
      n_exported_elements++;
    }
  }

  if (n_exported_elements == 0) {
    SLOG(SLOG_INFO, "Zero exported elements found - empty document converted.");
  }
  return true;
}

}  // namespace ink
