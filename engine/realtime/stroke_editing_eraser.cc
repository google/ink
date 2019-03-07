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

#include "ink/engine/realtime/stroke_editing_eraser.h"
#include <memory>

#include "third_party/absl/memory/memory.h"
#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/processing/runner/task_runner.h"
#include "ink/engine/public/types/uuid.h"
#include "ink/engine/scene/data/common/processed_element.h"
#include "ink/engine/scene/data/common/serialized_element.h"
#include "ink/engine/scene/types/element_id.h"

namespace ink {
namespace tools {
namespace {

constexpr int kCuttingBatchSize = 50;

class EraserStrokeDrawable : public IDrawable {
 public:
  EraserStrokeDrawable(std::shared_ptr<GLResourceManager> gl_resources,
                       const Mesh& eraser_stroke)
      : renderer_(gl_resources), eraser_stroke_(eraser_stroke) {
    gl_resources->mesh_vbo_provider->GenVBOs(&eraser_stroke_, GL_STATIC_DRAW);
  }

  void Draw(const Camera& cam, FrameTimeS draw_time) const override {
    renderer_.Draw(cam, draw_time, eraser_stroke_);
  }

 private:
  MeshRenderer renderer_;
  Mesh eraser_stroke_;
};

}  // namespace

void StrokeEditingEraser::CuttingEraserTask::PreExecute() {
  auto scene_graph = weak_scene_graph_.lock();
  if (!scene_graph) return;

  auto shared_data_map = weak_data_map_.lock();
  if (!shared_data_map) return;

  Rect world_bounds = geometry::Transform(
      geometry::Envelope(cutting_mesh_.verts), cutting_mesh_.object_matrix);

  scene_graph->ElementsInRegion(RegionQuery::MakeRectangleQuery(world_bounds, 0)
                                    .SetGroupFilter(active_group_),
                                std::back_inserter(elements_to_cut_));

  if (elements_to_cut_.empty()) return;

  for (const auto old_id : elements_to_cut_) {
    auto it = shared_data_map->find(old_id);
    if (it != shared_data_map->end()) {
      local_data_map_.emplace(std::move(*it));
      shared_data_map->erase(it);
      continue;
    }

    auto data = absl::make_unique<ElementData>();
    OptimizedMesh* opt_mesh;
    if (scene_graph->GetMesh(old_id, &opt_mesh)) {
      data->splitter = absl::make_unique<MeshSplitter>(*opt_mesh);
      data->shader_type = opt_mesh->type;
      data->attributes = scene_graph->GetElementMetadata(old_id).attributes;
      local_data_map_.emplace(old_id, std::move(data));
    } else {
      SLOG(SLOG_WARNING,
           "Cannot perform partial erase on element $0: unable to get mesh",
           old_id);
      ASSERT(false);
    }
  }
}

void StrokeEditingEraser::CuttingEraserTask::Execute() {
  if (elements_to_cut_.empty()) return;

  cutting_mesh_.NormalizeTriangleOrientation();
  for (const auto& id : elements_to_cut_) {
    auto data_it = local_data_map_.find(id);
    if (data_it == local_data_map_.end()) continue;

    data_it->second->splitter->Split(cutting_mesh_);
  }
}

void StrokeEditingEraser::CuttingEraserTask::OnPostExecute() {
  auto shared_data_map = weak_data_map_.lock();
  if (!shared_data_map) return;

  for (auto& data_pair : local_data_map_)
    shared_data_map->emplace(std::move(data_pair));
}

StrokeEditingEraser::SerializeEraserTask::SerializeEraserTask(
    std::weak_ptr<SceneGraph> weak_scene_graph,
    std::shared_ptr<ElementDataMap> data_map, const settings::Flags& flags,
    GroupId active_group, const CallbackFlags& callback_flags,
    IDrawable* drawable_ptr)
    : weak_scene_graph_(weak_scene_graph),
      data_map_(data_map),
      active_group_(active_group),
      active_group_uuid_(kInvalidUUID),
      callback_flags_(callback_flags),
      low_memory_mode_(flags.GetFlag(settings::Flag::LowMemoryMode)),
      drawable_ptr_(drawable_ptr) {
  if (active_group_ != kInvalidElementId) {
    if (auto scene_graph = weak_scene_graph_.lock()) {
      active_group_uuid_ = scene_graph->UUIDFromElementId(active_group_);
    }
  }
}

void StrokeEditingEraser::SerializeEraserTask::PreExecute() {
  if (!data_map_ || data_map_->empty()) return;

  auto scene_graph = weak_scene_graph_.lock();
  if (!scene_graph) return;

  for (auto& id_data_pair : *data_map_) {
    auto& id = id_data_pair.first;
    auto& data = id_data_pair.second;
    if (!data->splitter->IsMeshChanged()) {
      // This element was unaffected by the cutting stroke, skip it.
      continue;
    }

    if (!data->splitter->IsResultEmpty()) {
      // This element still has geometry remaining, generate a new ID and UUID
      // so that we can serialize it.
      data->new_uuid = scene_graph->GenerateUUID();
      if (!scene_graph->GetNextPolyId(data->new_uuid, &data->new_id)) {
        SLOG(SLOG_WARNING,
             "Cannot commit result of splitting element $0: unable to get new "
             "ID",
             id);
      }
    }
  }
}

void StrokeEditingEraser::SerializeEraserTask::Execute() {
  if (!data_map_ || data_map_->empty()) return;

  for (auto& id_data_pair : *data_map_) {
    auto& data = id_data_pair.second;
    if (data->new_id == kInvalidElementId) {
      // We were unable to get an ID for this element; it can't be serialized.
      continue;
    } else if (!data->splitter->IsMeshChanged()) {
      // This element was unaffected by the cutting stroke, skip it.
      continue;
    } else if (data->splitter->IsResultEmpty()) {
      // There's nothing left after the split, so there's nothing to serialize.
      continue;
    }

    Mesh result_mesh;
    // GetResult() should always return true, because we already check
    // IsMeshChanged() above. Similarly, result_mesh should never be empty
    // because we checked IsResultEmpty() above.
    EXPECT(data->splitter->GetResult(&result_mesh));
    ASSERT(!result_mesh.verts.empty());
    data->processed_element = absl::make_unique<ProcessedElement>(
        data->new_id, result_mesh, data->shader_type, low_memory_mode_,
        data->attributes);
    data->processed_element->group = active_group_;
    data->serialized_element = absl::make_unique<SerializedElement>(
        data->new_uuid, active_group_uuid_, SourceDetails::FromEngine(),
        callback_flags_);
    data->serialized_element->Serialize(*data->processed_element);
  }
}

void StrokeEditingEraser::SerializeEraserTask::OnPostExecute() {
  auto scene_graph = weak_scene_graph_.lock();
  if (!scene_graph) return;

  scene_graph->RemoveDrawable(drawable_ptr_);

  if (!data_map_ || data_map_->empty()) return;

  std::vector<SceneGraph::ElementAdd> elements_to_add;
  std::vector<ElementId> elements_to_remove;
  elements_to_add.reserve(data_map_->size());
  elements_to_remove.reserve(data_map_->size());
  for (auto& id_data_pair : *data_map_) {
    auto old_id = id_data_pair.first;
    auto& data = id_data_pair.second;

    if (!data->splitter->IsMeshChanged()) {
      // This element was unaffected by the cutting stroke, skip it.
      continue;
    }
    elements_to_remove.emplace_back(old_id);

    if (data->new_id == kInvalidElementId) {
      // We were unable to serialize the new element, there's nothing to add.
      continue;
    }

    if (data->processed_element == nullptr ||
        data->serialized_element == nullptr) {
      // This should never happen.
      SLOG(SLOG_WARNING, "Element $0 was not serialized, and will be skipped",
           data->new_id);
      continue;
    }

    ElementId id_to_add_below;
    if (!scene_graph->GetElementAbove(old_id, &id_to_add_below).ok()) {
      SLOG(SLOG_WARNING,
           "Could not find element above $0; element $1 will be placed "
           "at the top of its group.",
           old_id, data->new_id);
      id_to_add_below = kInvalidElementId;
    }
    elements_to_add.emplace_back(std::move(data->processed_element),
                                 std::move(data->serialized_element),
                                 id_to_add_below);
  }

  scene_graph->ReplaceElements(std::move(elements_to_add), elements_to_remove,
                               SourceDetails::FromEngine());
}

StrokeEditingEraser::StrokeEditingEraser(
    const service::UncheckedRegistry& registry)
    : scene_graph_(registry.GetShared<SceneGraph>()),
      gl_resources_(registry.GetShared<GLResourceManager>()),
      input_modeler_(registry.GetShared<input::InputModeler>()),
      line_modifier_factory_(registry.GetShared<LineModifierFactory>()),
      layer_manager_(registry.GetShared<LayerManager>()),
      task_runner_(registry.GetShared<ITaskRunner>()),
      flags_(registry.GetShared<settings::Flags>()),
      renderer_(registry),
      line_builder_(registry.GetShared<settings::Flags>(), gl_resources_) {
  RegisterForInput(registry.GetShared<input::InputDispatch>());
}

void StrokeEditingEraser::Draw(const Camera& camera,
                               FrameTimeS draw_time) const {
  renderer_.Draw(camera, draw_time, line_builder_.StableMesh());
  renderer_.Draw(camera, draw_time, line_builder_.UnstableMesh());
  renderer_.Draw(camera, draw_time, line_builder_.PredictionMesh());
}

input::CaptureResult StrokeEditingEraser::OnInput(const input::InputData& data,
                                                  const Camera& camera) {
  if (data.Get(input::Flag::Cancel) || data.Get(input::Flag::Right)) {
    Clear();
    return input::CaptureResult::CapResRefuse;
  }

  bool is_line_end = false;
  if (data.Get(input::Flag::Primary) && data.Get(input::Flag::TDown)) {
    SetupNewLine(data, camera);
  } else if (data.Get(input::Flag::TUp)) {
    is_line_end = true;
  }

  if (!touch_id_.has_value() || touch_id_ != data.id)
    return input::CapResObserve;

  input_modeler_->AddInputToModel(data);
  std::vector<input::ModeledInput> model_results;
  while (input_modeler_->HasModelResult())
    model_results.emplace_back(input_modeler_->PopNextModelResult());
  if (!model_results.empty())
    line_builder_.ExtrudeModeledInput(camera, model_results, is_line_end);

  auto active_group_or = layer_manager_->GroupIdOfActiveLayer();
  auto active_group =
      active_group_or.ok() ? active_group_or.ValueOrDie() : kInvalidElementId;

  if (line_builder_.StableMesh().NumberOfTriangles() >
          next_stable_triangle_ + kCuttingBatchSize ||
      is_line_end) {
    StartCuttingTask(active_group);
  }

  if (is_line_end) {
    StartSerializationTask(active_group);
  } else {
    auto predicted_points = input_modeler_->PredictModelResults();
    line_builder_.ConstructPrediction(camera, predicted_points);
  }

  return input::CapResCapture;
}

void StrokeEditingEraser::SetupNewLine(const input::InputData& data,
                                       const Camera& camera) {
  touch_id_ = data.id;
  glm::vec4 draw_color{0};
  input_modeler_->Reset(camera, input::InputModelParams(data.type));
  BrushParams brush_params;
  brush_params.size = size_;
  input_modeler_->SetParams(brush_params.shape_params,
                            brush_params.size.WorldSize(camera));

  auto line_modifier = line_modifier_factory_->Make(brush_params, draw_color);
  auto prediction_modifier =
      line_modifier_factory_->Make(brush_params, draw_color);
  line_modifier->SetupNewLine(data, camera);
  prediction_modifier->SetupNewLine(data, camera);

  line_modifier->mutable_params().refine_mesh = true;
  prediction_modifier->mutable_params().refine_mesh = true;

  line_builder_.SetupNewLine(camera, brush_params.tip_type, data.time,
                             data.type, std::move(line_modifier),
                             std::move(prediction_modifier));
  line_builder_.SetShaderMetadata(ShaderMetadata::Eraser());
  next_stable_triangle_ = 0;
  element_data_map_ = std::make_shared<ElementDataMap>();
}

absl::optional<input::Cursor> StrokeEditingEraser::CurrentCursor(
    const Camera& camera) const {
  return input::Cursor(input::CursorType::BRUSH, ink::Color::kWhite,
                       size_.ScreenSize(camera));
}

void StrokeEditingEraser::Clear() {
  line_builder_.Clear();
  touch_id_ = absl::nullopt;
  next_stable_triangle_ = 0;
  element_data_map_ = nullptr;
}

void StrokeEditingEraser::StartCuttingTask(GroupId active_group) {
  const auto& stable_mesh = line_builder_.StableMesh();
  int n_stable_tris = stable_mesh.NumberOfTriangles();
  Mesh cutting_mesh;
  cutting_mesh.verts.reserve(3 * (n_stable_tris - next_stable_triangle_));
  cutting_mesh.idx.reserve(3 * (n_stable_tris - next_stable_triangle_));
  for (int i = next_stable_triangle_; i < n_stable_tris; ++i) {
    cutting_mesh.verts.emplace_back(stable_mesh.GetVertex(i, 0));
    cutting_mesh.verts.emplace_back(stable_mesh.GetVertex(i, 1));
    cutting_mesh.verts.emplace_back(stable_mesh.GetVertex(i, 2));
  }
  cutting_mesh.GenIndex();
  cutting_mesh.object_matrix = stable_mesh.object_matrix;
  next_stable_triangle_ = n_stable_tris;

  task_runner_->PushTask(absl::make_unique<CuttingEraserTask>(
      scene_graph_, element_data_map_, cutting_mesh, active_group));
}

void StrokeEditingEraser::StartSerializationTask(GroupId active_group) {
  auto drawable = std::make_shared<EraserStrokeDrawable>(
      gl_resources_, line_builder_.StableMesh());
  task_runner_->PushTask(absl::make_unique<SerializeEraserTask>(
      scene_graph_, std::move(element_data_map_), *flags_, active_group,
      scene_graph_->GetElementNotifier()->GetCallbackFlags(
          SourceDetails::FromEngine()),
      drawable.get()));
  scene_graph_->AddDrawable(std::move(drawable));
  Clear();
}

void StrokeEditingEraser::SetBrushSize(BrushParams::BrushSize size) {
  size_ = size;
}

}  // namespace tools
}  // namespace ink
