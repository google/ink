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

#ifndef INK_PUBLIC_CONTRIB_EXPORT_H_
#define INK_PUBLIC_CONTRIB_EXPORT_H_

#include <limits>
#include <vector>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/scene/data/common/stroke.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/funcs/utils.h"
#include "ink/engine/util/proto/serialize.h"
#include "ink/proto/document_portable_proto.pb.h"
#include "ink/proto/elements_portable_proto.pb.h"
#include "ink/proto/export_portable_proto.pb.h"

namespace ink {
namespace contrib {

// Populates the TextBox proto with text data from the bundle.  Uses the
// transform to write world coordinates for the text element bounds.
// Returns true if the bundle contained valid text data to put in the proto.
bool ExtractTextBox(const ink::proto::ElementBundle& bundle,
                    const glm::mat4& transform,
                    ink::proto::TextBox* text_proto);

// Populates the StrokeOutline proto with the outline from the bundle.
// Uses the transform to write world coordinates to the StrokeOutline
// proto. Returns true if the bundle contained a valid outline to put in the
// proto.
bool ExtractStrokeOutline(const ink::proto::ElementBundle& bundle,
                          const glm::mat4& transform,
                          ink::proto::StrokeOutline* outline_proto);

// Flattens the given scene into a representation suitable for use in a
// 2D drawing context or vector graphics format.
//
// Requires the attach_outline flag to be specified in SEngine::setCallbackFlags
// before anything is drawn. Any strokes drawn without the attach_outline flag
// set will be silently skipped when flattening. Note that this call will not
// fill in VectorElement.page_index.
bool ToVectorElements(const ink::proto::Snapshot& scene,
                      ink::proto::VectorElements* result);

// Similar to ToVectorElements above, but will also store page definitions along
// with the scene, if they are present. This function will also properly fill
// in VectorElement.page_index.
// This is an experimental API, to support multi-page documents.
bool ToExportedDocument(const ink::proto::Snapshot& scene,
                        ink::proto::ExportedDocument* result);

}  // namespace contrib
}  // namespace ink

#endif  // INK_PUBLIC_CONTRIB_EXPORT_H_
