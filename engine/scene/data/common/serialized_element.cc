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

#include "ink/engine/scene/data/common/serialized_element.h"

#include <vector>

#include "ink/engine/colors/colors.h"
#include "ink/engine/geometry/primitives/matrix_utils.h"
#include "ink/engine/public/types/uuid.h"
#include "ink/engine/scene/data/common/mesh_serializer_provider.h"

namespace ink {

namespace {

void SerializeShaderTypeToStroke(ShaderType shader_type,
                                 ink::proto::Stroke* stroke) {
  switch (shader_type) {
    case NoShader:
      // FAIL
      RUNTIME_ERROR("Expected a shader to be set. Got NoShader.");
      break;
    case ColoredVertShader:
      stroke->set_shader_type(ink::proto::VERTEX_COLORED);
      break;
    case SingleColorShader:
      stroke->set_shader_type(ink::proto::SOLID_COLORED);
      break;
    case EraseShader:
      stroke->set_shader_type(ink::proto::ERASE);
      break;
    case TexturedVertShader:
      stroke->set_shader_type(ink::proto::VERTEX_TEXTURED);
      break;
    default:
      RUNTIME_ERROR("Unknown shader type: $0", static_cast<int>(shader_type));
  }
}

}  // namespace

SerializedElement::SerializedElement(const UUID& uuid, const UUID& parent_uuid,
                                     SourceDetails source_details,
                                     CallbackFlags callback_flags)
    : uuid(uuid),
      parent_uuid(parent_uuid),
      source_details(source_details),
      callback_flags(callback_flags) {}

void SerializedElement::Serialize(const ProcessedElement& processed_element) {
  // Do nothing if we don't need a callback for this line.
  if (!callback_flags.do_callback) {
    bundle.reset(nullptr);
    return;
  }
  // Form the arguments for ElementBundle::WriteToProto:
  //  (1) a blank bundle to fill in
  //  (2) the uuid (already present)
  //  (3) the element
  //  (4) the affine transform
  // and then fill them in based on callback flags.  Finally, (5) add the
  // uncompressed line data to the element bundle manually, as it is not part
  // of ElementBundle::WriteToProto
  bundle.reset(new ink::proto::ElementBundle());
  ink::proto::AffineTransform transform_proto;
  ink::proto::Element element_proto;

  // (4) We always write the transform.
  ink::util::WriteToProto(&transform_proto, processed_element.obj_to_group);

  // (3) Fill in the parts of Element
  const auto& processed_mesh = *processed_element.mesh;
  // Expect that we still have cpu-side data
  ASSERT(!processed_mesh.idx.empty());

  Stroke stroke(uuid, processed_mesh.type, processed_mesh.object_matrix);

  if (callback_flags.attach_compressed_input_points) {
    InputPoints::CompressToProto(stroke.MutableProto(),
                                 processed_element.input_points);
  }

  if (callback_flags.attach_compressed_mesh_data) {
    SerializeShaderTypeToStroke(processed_mesh.type, stroke.MutableProto());

    const bool vertex_colored = (processed_mesh.type == TexturedVertShader ||
                                 processed_mesh.type == ColoredVertShader);
    if (!vertex_colored) {
      stroke.MutableProto()->set_abgr(Vec4ToUintABGR(processed_mesh.color));
    }

    ink::proto::LOD* lod = stroke.MutableProto()->add_lod();
    lod->set_max_coverage(1);
    auto status =
        mesh::WriterFor(processed_mesh)->MeshToLod(processed_mesh, lod);
    if (!status.ok()) {
      SLOG(SLOG_ERROR, "could not read mesh: $0", status.error_message());
    }
  }

  ink::util::WriteToProto(&element_proto, stroke);
  ink::util::WriteToProto(element_proto.mutable_attributes(),
                          processed_element.attributes);

  if (processed_element.text) {
    // Text proto doesn't contain positioning information. Instead, the affine
    // transform populated above is assumed to be the transform of a kTextSize x
    // kTextSize rect at the origin to determine the positioning of the text.
    ink::util::WriteToProto(element_proto.mutable_text(),
                            *processed_element.text);
  }

  ink::ElementBundle::WriteToProto(bundle.get(), uuid, element_proto,
                                   transform_proto);
  if (parent_uuid != kInvalidUUID) bundle->set_group_uuid(parent_uuid);

  // (5) Write the uncompressed outline.
  if (callback_flags.attach_uncompressed_outline &&
      !processed_element.outline.empty()) {
    ink::proto::UncompressedStroke* stroke =
        bundle->mutable_uncompressed_element()->mutable_uncompressed_stroke();
    ink::util::WriteToProto(stroke->mutable_outline(),
                            processed_element.outline);
    stroke->set_rgba(
        Vec4ToUintRGBA(RGBPremultipliedtoRGB(processed_mesh.color)));
  }
}
}  // namespace ink
