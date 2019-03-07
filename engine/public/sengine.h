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

#ifndef INK_ENGINE_PUBLIC_SENGINE_H_
#define INK_ENGINE_PUBLIC_SENGINE_H_

#include <cstdint>
#include <memory>
#include <string>

#include "third_party/absl/strings/string_view.h"
#include "ink/engine/brushes/brushes.h"
#include "ink/engine/gl.h"
#include "ink/engine/input/input_coalescer.h"
#include "ink/engine/input/input_data.h"
#include "ink/engine/input/input_receiver.h"
#include "ink/engine/public/host/ihost.h"
#include "ink/engine/public/types/client_bitmap.h"
#include "ink/engine/public/types/exported_image.h"
#include "ink/engine/public/types/input.h"
#include "ink/engine/public/types/iselection_provider.h"
#include "ink/engine/public/types/itexture_request_handler.h"
#include "ink/engine/public/types/status.h"
#include "ink/engine/public/types/uuid.h"
#include "ink/engine/rendering/gl_managers/text_texture_provider.h"
#include "ink/engine/rendering/strategy/rendering_strategy.h"
#include "ink/engine/scene/graph/scene_change_notifier.h"
#include "ink/engine/scene/root_controller.h"
#include "ink/engine/service/definition_list.h"
#include "ink/engine/service/unchecked_registry.h"
#include "ink/engine/settings/flags.h"
#include "ink/proto/animations_portable_proto.pb.h"
#include "ink/proto/document_portable_proto.pb.h"
#include "ink/proto/elements_portable_proto.pb.h"
#include "ink/proto/geometry_portable_proto.pb.h"
#include "ink/proto/sengine_portable_proto.pb.h"
#include "ink/proto/text_portable_proto.pb.h"
#include "ink/public/document/document.h"
#include "ink/public/document/idocument_listener.h"

namespace ink {

class SEngine : public IDocumentListener {
 public:
  SEngine(std::shared_ptr<IHost> handler, const proto::Viewport& viewport,
          uint64_t randomSeed);
  SEngine(std::shared_ptr<IHost> handler, const proto::Viewport& viewport,
          uint64_t randomSeed, std::shared_ptr<Document> document);
  SEngine(std::shared_ptr<IHost> handler, const proto::Viewport& viewport,
          uint64_t randomSeed, std::shared_ptr<Document> document,
          std::unique_ptr<service::DefinitionList> service_definitions);


  void SetDocument(std::shared_ptr<Document> document);

  // Attempts to add the given element to the scene, placing it right below the
  // already-existing element with given UUID.
  ABSL_MUST_USE_RESULT Status
  Add(proto::ElementBundle element,
      absl::string_view below_element_uuid = kInvalidUUID);

  void SetRenderingStrategy(proto::RenderingStrategy rendering_strategy);
  void SetRenderingStrategy(RenderingStrategy rendering_strategy);

  // Sets the layout to the specified layout type at the given spacing. Also
  // updates the pages bounds to the final result of the page layout.
  enum PageLayout {
    VERTICAL = 1,
    HORIZONTAL = 2,
  };
  void SetPageLayout(PageLayout strategy, float spacing_world);
  void SetPageLayout(proto::LayoutSpec spec);

  // Returns the inter-page spacing, in world coordinates, if the engine
  // is currently editing a multi-page document, and if the current layout
  // is linear (and therefore has a well-definined inter-page spacing).
  // Returns 0 if there is no well-defined inter-page spacing.
  float GetPageSpacingWorld() const;

  // Returns the world coordinates of all pages in the currently-edited
  // multipage document, if any, or an empty proto::Rects otherwise.
  proto::Rects GetPageLocations() const;

  // Focuses the camera on the given page. If an invalid page is specified, will
  // return without doing anything.
  void FocusOnPage(int page);

  // WARNING: Mixing the zero- and one-argument draw() methods will result in
  // undefined behavior.
  void draw();

  // The time is expected to increase monotonically.
  void draw(double draw_time);

  void Undo();
  void Redo();

  void clear();

  // Remove all user-editable elements from the scene. Does not remove groups.
  // If you want to really "remove" everything, groups and all, then create a
  // new empty Document and SetDocument with it.
  void RemoveAllElements();

  // Remove all elements currently selected by the edit tool, if any.
  void RemoveSelectedElements();

  // If an element with the given UUID exists, removes it.
  void RemoveElement(const UUID& uuid);

  // The dispatchInput family of methods forward to
  // InputReceiver::DispatchInput. Please see those methods for documentation.
  void dispatchInput(input::InputType type, uint32_t id, uint32_t flags,
                     double time, float screen_pos_x, float screen_pos_y);
  void dispatchInput(input::InputType type, uint32_t id, uint32_t flags,
                     double time, float screen_pos_x, float screen_pos_y,
                     float wheel_delta_x, float wheel_delta_y, float pressure,
                     float tilt, float orientation);
  void dispatchInput(proto::SInputStream inputStream);
  void dispatchInput(const proto::PlaybackStream& unsafe_playback_stream,
                     bool force_camera = false);

  void handleCommand(const proto::Command& command);

  /**
   * Inserts a texture handler into the engine's chain of handlers, if any. The
   * given ID can be used to remove the given handler as needed. The SEngine
   * takes ownership of the given handler. If a handler is already registered
   * with the given ID, it is removed, and then the replacement one is added.
   *
   * The handlers added to the engine are tried, in the order in which they
   * were added. If any succeeds, then the rest are skipped. If none succeeds,
   * then the Host's requestImage() will be called.
   *
   * @param handler_id An arbitrary non-empty string used to later identify the
   * given texture handler for removal.
   * @param handler The texture request handler to add to this engine's chain
   * of handlers.
   */
  void AddTextureRequestHandler(
      const std::string& handler_id,
      std::shared_ptr<ITextureRequestHandler> handler);

  /**
   * Gets the texture handler identified by the given id. If no such id is
   * found, returns nullptr.
   * @param handler_id An arbitrary non-empty string associated with a texture
   * handler added via AddTextureRequestHandler.
   */
  ITextureRequestHandler* GetTextureRequestHandler(
      const std::string& handler_id) const;

  /**
   * Removes the texture handler identified by the given id. If no such id is
   * found, nothing happens.
   * @param handler_id An arbitrary non-empty string associated with a texture
   * handler added via AddTextureRequestHandler.
   */
  void RemoveTextureRequestHandler(const std::string& handler_id);

  void addImageData(const proto::ImageInfo& image_info,
                    const ClientBitmap& client_bitmap);
  // Counterpart to addImageData that indicates that the host cannot provide
  // image data for the given texture URI.
  void RejectTextureUri(const std::string& uri);
  // Evict the texture with the given URI from cache. May be re-requested if
  // needed.
  void evictImageData(const std::string& uri);
  void evictAllTextures();

  UUID addImageRect(const proto::ImageRect& addImageRect);
  UUID addPath(const proto::AddPath& path);
  UUID addText(const proto::text::AddText& unsafe_add_text);
  void beginTextEditing(const UUID& uuid);
  void updateText(const proto::text::UpdateText& unsafe_update_text);

  void setToolParams(const proto::ToolParams& unsafeProto);
  void setViewport(const proto::Viewport& viewport);

  void assignFlag(const proto::Flag& flag, bool value);

  // Render a bitmap of the current scene.  Camera will be pointed at the
  // entire document if page bounds have been previously specified (see
  // "setPageBounds()"/"setBackgroundImage()"), otherwise at the current
  // screen.
  // Generated bitmap will have width in pixels of "pngExport.width_px()" and
  // height in pixels to preserve the aspect ratio of the page bounds/screen.
  // Caller will receive a callback from Host::onImageExportComplete
  // with the bitmap.  Result may be scaled down if request would produce an
  // image larger than GL_MAX_TEXTURE_SIZE.
  void startImageExport(const proto::ImageExport& imageExport);

  // Render a bitmap of the current scene in RGBA_8888 format with the given
  // maximum dimension in pixels. If worldRect is non-empty, then export the
  // portion of the scene in that world rect.
  void exportImage(uint32_t maxPixelDimension, Rect worldRect,
                   ExportedImage* out);

  void SetCameraPosition(const Rect& position);
  void SetCameraPosition(const proto::CameraPosition& position);
  proto::CameraPosition GetCameraPosition() const;

  void PageUp();
  void PageDown();
  void ScrollUp();
  void ScrollDown();

  proto::EngineState getEngineState() const;

  void setOutOfBoundsColor(const proto::OutOfBoundsColor& outOfBoundsColor);
  void setGrid(const proto::GridInfo& grid_info);
  void clearGrid();

  // Engine operations that modify the scene graph (e.g. addElement, addPath)
  // are asynchronous.  "addSequencePoint" provides a callback through the Host
  // that is guaranteed to occur after any prior scene graph modifications have
  // completed.
  void addSequencePoint(const proto::SequencePoint& sequencePoint);

  void setCallbackFlags(const proto::SetCallbackFlags& callbackFlags);
  void setOutlineExportEnabled(bool enabled);
  void setHandwritingDataEnabled(bool enabled);

  void setCameraBoundsConfig(
      const proto::CameraBoundsConfig& cameraBoundsConfig);

  // If an element with the given UUID exists, switches to the
  // ElementManipulationTool and selects that element.
  void SelectElement(const UUID& uuid);

  void deselectAll();

  // If the crop tool or mode is active, commit its currently indicated crop
  // region. If not, this is a no-op (although a warning is produced).
  void CommitCrop();

  // Set the crop area to the given rectangle in world coordinates.
  //
  // This method is only meaningful if the crop tool or crop mode is currently
  // enabled. Invalid, empty or out-of-page-bounds rectangles will result in an
  // error logged and no change in crop state.
  void SetCrop(const proto::Rect& crop_rect);

  void handleElementAnimation(const proto::ElementAnimation& animation);

  // If possible, avoid using this -- we want to move towards
  // a single API surface for Ink.
  std::shared_ptr<Document> document() const { return document_; }

  service::UncheckedRegistry* registry() const {
    return root_controller_->registry();
  }

  RootController* root() const { return root_controller_.get(); }

  // IDocumentListener
  void UndoRedoStateChanged(bool canUndo, bool canRedo) override;
  void EmptyStateChanged(bool empty) override{};

  /////////////////////////////////////////////////////////////////////////////
  // Layers API
  /////////////////////////////////////////////////////////////////////////////

  // Sets the visibility of the layer at index.
  bool SetLayerVisibility(int index, bool visible);

  // Sets the opacity of the layer at index.
  bool SetLayerOpacity(int index, int opacity);

  // Makes the layer at index the active layer. All input will be added to this
  // layer.
  void SetActiveLayer(int index);

  // Adds a new layer at the top of the stack. Returns true iff the layer is
  // added.
  bool AddLayer();

  // Move the layer at from_index to to_index.
  bool MoveLayer(int from_index, int to_index);

  // Remove the layer at index.
  bool RemoveLayer(int index);

  // Get the current order and properties of layers from the scene graph.
  proto::LayerState GetLayerState();

  // Get the minimum bounding rectangle (in world coordinates) of all the
  // elements currently in the scene, regardless of visibility. Empty rect if
  // there are no elements.
  Rect GetMinimumBoundingRect() const;
  Rect GetMinimumBoundingRectForLayer(int index) const;

  // Set the selection provider.
  void SetSelectionProvider(
      std::shared_ptr<ISelectionProvider> selection_provider);

  // Set the effect of the mouse wheel's scroll events.
  void SetMouseWheelBehavior(const proto::MouseWheelBehavior& behavior);

  // Changes SEngine's interface should also be reflected in embind.cc and Ink's
  // web API.
  // Test your build changes with
  // ./ink/public/js/update_prebuilts.sh
  // Thank you!  -- The Ink Web Team
 private:
  void handleRemoveElementsCommand(const proto::RemoveElementsCommand& cmd);
  proto::ElementBundle convertPathToBundle(const proto::Path& unsafe_path);
  bool CheckBlockedState() const;

  std::unique_ptr<RootController> root_controller_;
  std::shared_ptr<IHost> host_;
  std::shared_ptr<Document> document_;
  std::shared_ptr<input::InputReceiver> input_receiver_;
  std::shared_ptr<SceneChangeNotifier> scene_change_notifier_;

  friend class SEngineTestHelper;
};

}  // namespace ink

#endif  // INK_ENGINE_PUBLIC_SENGINE_H_
