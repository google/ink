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

#include <cstdint>
#include <memory>

#include "third_party/absl/memory/memory.h"
#include "third_party/absl/strings/string_view.h"
#include "third_party/absl/strings/substitute.h"
#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/input/prediction/input_predictor.h"
#include "ink/engine/input/prediction/kalman_predictor.h"
#include "ink/engine/public/host/exit.h"
#include "ink/engine/public/host/host.h"
#include "ink/engine/public/sengine.h"
#include "ink/engine/public/types/client_bitmap.h"
#include "ink/engine/public/types/input.h"
#include "ink/engine/public/types/iselection_provider.h"
#include "ink/engine/public/types/status.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/proto/brix_portable_proto.pb.h"
#include "ink/proto/document_portable_proto.pb.h"
#include "ink/proto/elements_portable_proto.pb.h"
#include "ink/proto/export_portable_proto.pb.h"
#include "ink/proto/mutations_portable_proto.pb.h"
#include "ink/proto/scene_change_portable_proto.pb.h"
#include "ink/proto/sengine_portable_proto.pb.h"
#include "ink/public/brix/brix.h"
#include "ink/public/contrib/export.h"
#include "ink/public/contrib/extensions/extension_points.h"
#include "ink/public/document/document.h"
#include "ink/public/document/passthrough_document.h"
#include "ink/public/document/single_user_document.h"
#include "ink/public/document/storage/in_memory_storage.h"
#include "ink/public/fingerprint/fingerprint.h"
#include "ink/public/mutations/mutation_applier.h"

#if defined(PDF_SUPPORT)
#include "ink/public/contrib/pdf_annotation.h"
#endif  // PDF_SUPPORT

#if defined(__asmjs__) || defined(__wasm__)
#include <emscripten/bind.h>
#include <emscripten/emscripten.h>
#if defined(__EMSCRIPTEN_PTHREADS__)
#include <emscripten/threading.h>
#endif  // defined(__EMSCRIPTEN_PTHREADS__)
#include <emscripten/val.h>

namespace ink {

using emscripten::allow_raw_pointers;
using emscripten::class_;
using emscripten::constructor;
using emscripten::enum_;
using emscripten::function;
using emscripten::pure_virtual;
using emscripten::select_const;
using emscripten::select_overload;
using emscripten::typed_memory_view;
using emscripten::val;
using emscripten::value_array;
using emscripten::value_object;
using emscripten::wrapper;

using google::protobuf::MessageLite;

using ink::proto::AffineTransform;
using ink::proto::BackgroundColor;
using ink::proto::BackgroundImageInfo;
using ink::proto::Border;
using ink::proto::BrixElementBundle;
using ink::proto::BrixElementMutation;
using ink::proto::BrushType;
using ink::proto::CameraPosition;
using ink::proto::Command;
using ink::proto::ElementTransformMutations;
using ink::proto::GridInfo;
using ink::proto::ImageExport;
using ink::proto::ImageInfo;
using ink::proto::ImageRect;
using ink::proto::LineSize;
using ink::proto::OutOfBoundsColor;
using ink::proto::PageProperties;
using ink::proto::Path;
using ink::proto::Point;
using ink::proto::SetCallbackFlags;
using ink::proto::Snapshot;
using ink::proto::ToolParams;
using ink::proto::VectorElements;
using ink::proto::Viewport;

EMSCRIPTEN_BINDINGS(stl_wrapps) {
  emscripten::register_vector<uint8_t>("VectorUint");
  emscripten::register_vector<std::string>("VectorString");
}

// Expose C++ classes to JavaScript using Embind.
// See
// http://kripken.github.io/emscripten-site/docs/porting/connecting_cpp_and_javascript/embind.html
// for details.

// Bind SEngine and its methods.  The macro argument and string argument to
// class_<T> must match.  The template argument must be the name of the class
// being bound.  The binding name and class name may differ.
//
// Once bound the class must be instantiated from JavaScript like so:
// var sengine = Module.makeSEngine(host);
// For other bound classes (e.g. protobufs) use:
// var obj = new Module.className();

// A note on Style:
//   We try to match go/cstyle where possible.
//   However, symbols exposed to JS will match JS Style (as those will be
//   called by other projects via our API).

namespace {
// Copies the given JS val, which is assumed to be a Uint8Array, into a
// vector-like collection of bytes of type T, and returns that T instance.
template <typename T>
T CopyToHeap(val js_byte_array) {
  T dest;
  auto len = js_byte_array["length"].as<unsigned>();
  dest.resize(len);
  val memory_view = val(typed_memory_view(len, dest.data()));
  memory_view.call<void>("set", js_byte_array);
  return dest;
}

// Copies the given C++ vector-like container of bytes into a new Uint8Array,
// and returns that new object.
template <typename T>
val CopyFromHeap(const T& bytes) {
  val length = val(bytes.size());
  val js_array = val::global("Uint8Array").new_(length);
  js_array.call<void>("set",
                      val(typed_memory_view(bytes.size(), bytes.data())));
  return js_array;
}
}  // namespace

namespace embind_sengine {

std::unique_ptr<SEngine> MakeSEngine(std::unique_ptr<Host> host,
                                     const ink::proto::Viewport& viewport,
                                     double random_seed) {
  auto document = std::make_shared<ink::SingleUserDocument>(
      std::make_shared<ink::InMemoryStorage>());
  auto registry = extensions::GetServiceDefinitions();
  registry->DefineService<input::InputPredictor, input::KalmanPredictor>();
  auto sengine = absl::make_unique<SEngine>(
      std::move(host), viewport, random_seed, document, std::move(registry));
  extensions::PostConstruct(sengine.get());
  return sengine;
}

void Reset(SEngine* engine) {
  engine->SetDocument(std::make_shared<ink::SingleUserDocument>(
      std::make_shared<ink::InMemoryStorage>()));
}

void LoadFromSnapshot(SEngine* engine, const ink::proto::Snapshot& snapshot) {
  engine->clear();
  std::unique_ptr<Document> doc;
  auto status = SingleUserDocument::CreateFromSnapshot(
      std::make_shared<ink::InMemoryStorage>(), snapshot, &doc);
  if (!status.ok()) {
    SLOG(SLOG_ERROR, "fallback to empty document: $0", status.error_message());
    doc = absl::make_unique<SingleUserDocument>(
        std::make_shared<ink::InMemoryStorage>());
  }
  engine->SetDocument(std::move(doc));
}

namespace {
void LoadFromDocumentAndSerializedMutations(SEngine* engine,
                                            std::unique_ptr<Document> doc,
                                            val serialized_mutations) {
  ink::MutationApplier applier(std::move(doc));
  auto jsvec = emscripten::vecFromJSArray<val>(serialized_mutations);
  for (const auto& serialized_mutation : jsvec) {
    auto mutation_bytes = CopyToHeap<std::vector<uint8_t>>(serialized_mutation);
    ink::proto::mutations::Mutation p;
    if (!p.ParseFromArray(mutation_bytes.data(), mutation_bytes.size())) {
      SLOG(SLOG_ERROR, "skipping unparseable serialized Mutations proto");
      continue;
    }
    auto status = applier.Apply(p);
    if (!status.ok()) {
      SLOG(SLOG_ERROR, "could not apply a Mutation proto: $0",
           status.error_message());
    }
  }
  auto status = applier.LoadEngine(engine);
  if (!status.ok()) {
    SLOG(SLOG_ERROR, "fallback to empty document: $0", status.error_message());
    engine->SetDocument(absl::make_unique<SingleUserDocument>(
        std::make_shared<ink::InMemoryStorage>()));
  }
}
}  // namespace

// @param engine The engine to load from the given snapshot and mutations.
//
// @param serialized_snapshot A Uint8Array containing a serialized
// ink::proto::Snapshot.
//
// @param serialized_mutations An Array<Uint8Array> of serialized
// ink::proto::mutations::Mutation protos.
void LoadFromSerializedSnapshotAndMutations(SEngine* engine,
                                            val serialized_snapshot,
                                            val serialized_mutations) {
  std::unique_ptr<ink::Document> doc;

  auto snapshot_bytes = CopyToHeap<std::vector<uint8_t>>(serialized_snapshot);
  ink::proto::Snapshot snapshot;
  if (!snapshot.ParseFromArray(snapshot_bytes.data(), snapshot_bytes.size())) {
    SLOG(
        SLOG_ERROR,
        "could not parse given data as Snapshot; starting from empty document");
    doc = absl::make_unique<SingleUserDocument>(
        std::make_shared<ink::InMemoryStorage>());
  } else {
    auto status = SingleUserDocument::CreateFromSnapshot(
        std::make_shared<ink::InMemoryStorage>(), snapshot, &doc);
    if (!status.ok()) {
      SLOG(SLOG_ERROR, "fallback to empty document: $0",
           status.error_message());
      doc = absl::make_unique<SingleUserDocument>(
          std::make_shared<ink::InMemoryStorage>());
    }
  }
  LoadFromDocumentAndSerializedMutations(engine, std::move(doc),
                                         serialized_mutations);
}

// @param engine The engine to load from the given snapshot and mutations.
//
// @param serialized_mutations An Array<Uint8Array> of serialized
// ink::proto::mutations::Mutation protos.
void LoadFromSerializedMutations(SEngine* engine, val serialized_mutations) {
  LoadFromDocumentAndSerializedMutations(
      engine,
      absl::make_unique<SingleUserDocument>(
          std::make_shared<ink::InMemoryStorage>()),
      serialized_mutations);
}

#if defined(PDF_SUPPORT)
void LoadPdfForAnnotation(SEngine* engine, std::string pdf_data) {
  ink::contrib::pdf::LoadPdfForAnnotation(pdf_data, engine);
}

val GetAnnotatedPdf(SEngine* engine) {
  std::string output_pdf_bytes;
  EXPECT(ink::contrib::pdf::GetAnnotatedPdf(*engine, &output_pdf_bytes));

  return CopyFromHeap(output_pdf_bytes);
}

val GetAnnotatedPdfDestructive(SEngine* engine) {
  std::string output_pdf_bytes;
  EXPECT(ink::contrib::pdf::GetAnnotatedPdfDestructive(*engine,
                                                       &output_pdf_bytes));
  return CopyFromHeap(output_pdf_bytes);
}
#endif  // PDF_SUPPORT

void UseDirectRenderer(SEngine* engine) {
  engine->SetRenderingStrategy(RenderingStrategy::kDirectRenderer);
}

void UseBufferedRenderer(SEngine* engine) {
  engine->SetRenderingStrategy(RenderingStrategy::kBufferedRenderer);
}
}  // namespace embind_sengine

EMSCRIPTEN_BINDINGS(SEngine) {
  function("makeSEngine", &embind_sengine::MakeSEngine);
  function("loadFromSnapshot", &embind_sengine::LoadFromSnapshot,
           allow_raw_pointers());
  function("loadFromSerializedSnapshotAndMutations",
           &embind_sengine::LoadFromSerializedSnapshotAndMutations,
           allow_raw_pointers());
  function("loadFromSerializedMutations",
           &embind_sengine::LoadFromSerializedMutations, allow_raw_pointers());
  function("reset", &embind_sengine::Reset, allow_raw_pointers());
  function("useDirectRenderer", &embind_sengine::UseDirectRenderer,
           allow_raw_pointers());
  function("useBufferedRenderer", &embind_sengine::UseBufferedRenderer,
           allow_raw_pointers());
#if defined(PDF_SUPPORT)
  function("loadPdfForAnnotation", &embind_sengine::LoadPdfForAnnotation,
           allow_raw_pointers());
  function("getAnnotatedPdf", &embind_sengine::GetAnnotatedPdf,
           allow_raw_pointers());
  function("getAnnotatedPdfDestructive",
           &embind_sengine::GetAnnotatedPdfDestructive, allow_raw_pointers());
#endif

  class_<Status>("Status")
      .function("ok", &Status::ok)
      .function("error_message", &Status::error_message);

  class_<SEngine>("SEngine")
      .function("draw", select_overload<void(void)>(&SEngine::draw))
      .function("clear", &SEngine::clear)
      .function(
          "dispatchInput",
          select_overload<void(input::InputType, uint32_t /* id */,
                               uint32_t /* flags */, double /* time */,
                               float /* screenPosX */, float /* screenPosY */)>(
              &SEngine::dispatchInput))
      .function(
          "dispatchInputFull",
          select_overload<void(
              input::InputType, uint32_t /* id */, uint32_t /* flags */,
              double /* time */, float /* screenPosX */, float /* screenPosY */,
              float /* wheel_delta_x */, float /* wheel_delta_y */,
              float /* pressure */, float /* tilt */, float /* orientation */)>(
              &SEngine::dispatchInput))
      .function("handleCommand", &SEngine::handleCommand)
      .function("addImageData", &SEngine::addImageData)
      .function("addPath", &SEngine::addPath)
      .function("addImageRect", &SEngine::addImageRect)
      .function("setToolParams", &SEngine::setToolParams)
      .function("setViewport", &SEngine::setViewport)
      .function("setCameraPosition",
                select_overload<void(const CameraPosition&)>(
                    &SEngine::SetCameraPosition))
      .function("setCameraPositionRect",
                select_overload<void(const Rect&)>(&SEngine::SetCameraPosition))
      .function("getCameraPosition", &SEngine::GetCameraPosition)
      .function("pageUp", &SEngine::PageUp)
      .function("pageDown", &SEngine::PageDown)
      .function("scrollUp", &SEngine::ScrollUp)
      .function("scrollDown", &SEngine::ScrollDown)
      .function("getEngineState", &SEngine::getEngineState)
      .function("setOutOfBoundsColor", &SEngine::setOutOfBoundsColor)
      .function("setGrid", &SEngine::setGrid)
      .function("clearGrid", &SEngine::clearGrid)
      .function("addSequencePoint", &SEngine::addSequencePoint)
      .function("setCallbackFlags", &SEngine::setCallbackFlags)
      .function("setOutlineExportEnabled", &SEngine::setOutlineExportEnabled)
      .function("setHandwritingDataEnabled",
                &SEngine::setHandwritingDataEnabled)
      .function("assignFlag", &SEngine::assignFlag)
      .function("startImageExport", &SEngine::startImageExport)
      .function("setCameraBoundsConfig", &SEngine::setCameraBoundsConfig)
      .function("document", &SEngine::document)
      .function("deselectAll", &SEngine::deselectAll)
      .function("setCrop", &SEngine::SetCrop)
      .function("commitCrop", &SEngine::CommitCrop)
      .function("setLayerVisibility", &SEngine::SetLayerVisibility)
      .function("setLayerOpacity", &SEngine::SetLayerOpacity)
      .function("setActiveLayer", &SEngine::SetActiveLayer)
      .function("addLayer", &SEngine::AddLayer)
      .function("moveLayer", &SEngine::MoveLayer)
      .function("removeLayer", &SEngine::RemoveLayer)
      .function("getLayerState", &SEngine::GetLayerState)
      .function("getMinimumBoundingRect", &SEngine::GetMinimumBoundingRect)
      .function("getMinimumBoundingRectForLayer",
                &SEngine::GetMinimumBoundingRectForLayer)
      .function("updateText", &SEngine::updateText)
      .function("setSelectionProvider", &SEngine::SetSelectionProvider)
      .function("beginTextEditing", &SEngine::beginTextEditing)
      .function("setMouseWheelBehavior", &SEngine::SetMouseWheelBehavior)
      .function("removeAllElements", &SEngine::RemoveAllElements)
      .function("focusOnPage", &SEngine::FocusOnPage)
      .function("setPageLayout", select_overload<void(ink::proto::LayoutSpec)>(
                                     &SEngine::SetPageLayout))
      .function("getPageSpacingWorld", &SEngine::GetPageSpacingWorld)
      .function("getPageLocations", &SEngine::GetPageLocations);
};

EMSCRIPTEN_BINDINGS(Document) {
  class_<Document>("Document")
      .smart_ptr<std::shared_ptr<Document>>("Document")
      .function("Add", &Document::Add)
      .function("AddBelow", &Document::AddBelow)
      .function("GetPageProperties", &Document::GetPageProperties)
      .function("GetSnapshot", &Document::GetSnapshot)
      .function("Redo", &Document::Redo)
      .function("Remove", &Document::Remove)
      .function("RemoveAll", &Document::RemoveAll)
      .function("SetPageBounds", &Document::SetPageBounds)
      .function("SetBackgroundImage", &Document::SetBackgroundImage)
      .function("SetBackgroundColor", &Document::SetBackgroundColor)
      .function("SetPageBorder", &Document::SetPageBorder)
      .function("SetElementTransforms", &Document::SetElementTransforms)
      .function("Undo", &Document::Undo)
      .function("SetUndoEnabled", &Document::SetUndoEnabled);
}

// Exit by abort.  Handled by Module['onAbort'] in the JS wrapper.
void exit() { std::abort(); }

// Returns a Uint8Array of serialized proto bytes.
// NOTE(trybka): Since this method is going to be bound on objects of type T,
//               but it is not a member function, we pass a non-const ref.
//               See go/embind-ref for more information.
template <typename T>
val ToArrayBuffer(T& cpp_proto) {
  std::vector<uint8_t> bytes;
  auto len = cpp_proto.ByteSize();
  bytes.resize(len);
  cpp_proto.SerializeToArray(bytes.data(), len);
  return CopyFromHeap(bytes);
}
// A free function wrapper around HostWrapper::RequestFrame. Used to ferry a
// HostWrapper pointer through the emscripten message queue.
//
// Must be called on the main thread!
template <typename T>
void RequestFrameMainThreadTrampoline(T* host_wrapper) {
  // HostWrapper::SetTargetFPS will eventually end up calling draw_time::setFps,
  // which grabs an animation frame if needed.
  // (Target fps is reset at the end of every draw call.)
  host_wrapper->SetTargetFPS(60);
}

// Wrapper class that allows JavaScript to subclass the Host
// implementation.  The class implements Host by delegating to the
// JavaScript functions with the same name, scoped to a Host object
// implemented in JavaScript.
//
// JavaScript class declaration is like so:
// var MyViewController = Module.Host.extend('Host', {
//   getTargetFPS: function() { return 60; },
//   ...etc.
// });
class HostWrapper : public wrapper<Host> {
 public:
  EMSCRIPTEN_WRAPPER(HostWrapper);
  void RequestFrame() override {
#if defined(__EMSCRIPTEN_PTHREADS__)
    // Jump to the main thread before making a request.
    emscripten_async_run_in_main_runtime_thread(
        EM_FUNC_SIG_VI, RequestFrameMainThreadTrampoline<HostWrapper>, this);
#else
    // (Target fps is reset at the end of every draw call.)
    RequestFrameMainThreadTrampoline<HostWrapper>(this);
#endif  // __EMSCRIPTEN_PTHREADS__
  }
  void SetTargetFPS(uint32_t fps) override { call<void>("setTargetFPS", fps); }
  uint32_t GetTargetFPS() const override {
    return call<uint32_t>("getTargetFPS");
  }
  void BindScreen() override { return call<void>("bindScreen"); }
  void RequestImage(const std::string& uri) override {
    call<void>("requestImage", uri);
  }

  void OnMutation(const proto::mutations::Mutation& mutation) override {
    auto buf = ToArrayBuffer(mutation);
    call<void>("handleMutation", buf);
  }

  void SceneChanged(
      const ink::proto::scene_change::SceneChangeEvent& scene_change) override {
    std::vector<uint8_t> bytes;
    auto len = scene_change.ByteSize();
    bytes.resize(len);
    scene_change.SerializeToArray(bytes.data(), len);
    // Using a typed_memory_view to avoid making an extra buffer copy.  We
    // expect onSceneChanged to parse this protobuf but not attempt to use
    // the memory view after that.
    val view = val(typed_memory_view(len, bytes.data()));
    call<void>("onSceneChanged", view);
  }

  // Use c-strings for base64 encoded protos and UUIDs for simpler wire
  // binding.
  void ElementsAdded(const ink::proto::ElementBundleAdds& bundle_adds,
                     const ink::proto::SourceDetails& source_details) override {
    for (const auto& add : bundle_adds.element_bundle_add()) {
      const auto& bundle = add.element_bundle();
      BrixElementBundle brix_element_bundle;
      if (!ink::brix::ElementBundleToBrixElementBundle(bundle,
                                                       &brix_element_bundle)) {
        return;
      }
      call<void>("handleElementCreated",
                 val(brix_element_bundle.uuid().c_str()),
                 val(brix_element_bundle.encoded_element().c_str()),
                 val(brix_element_bundle.encoded_transform().c_str()));
    }
  }
  void ElementsTransformMutated(
      const ElementTransformMutations& mutations,
      const ink::proto::SourceDetails& source_details) override {}
  void ElementsRemoved(
      const ink::proto::ElementIdList& c,
      const ink::proto::SourceDetails& source_details) override {
    auto uuids = val::array();
    for (size_t i = 0; i < c.uuid_size(); i++) {
      uuids.set(i, val(c.uuid(i).c_str()));
    }
    call<void>("handleElementsRemoved", uuids);
  }
  void ImageExportComplete(uint32_t width_px, uint32_t height_px,
                           const std::vector<uint8_t>& img_bytes,
                           uint64_t fingerprint) override {
    call<void>("onImageExportComplete", width_px, height_px,
               val(typed_memory_view(img_bytes.size(), img_bytes.data())));
  }
  std::string GetPlatformId() const override {
    return call<std::string>("getPlatformId");
  }
  void SequencePointReached(int32_t id) override {
    call<void>("onSequencePointReached", id);
  }
  void FlagChanged(const ink::proto::Flag& which, bool enabled) override {
    call<void>("onFlagChanged", static_cast<int32_t>(which), enabled);
  }
  void UndoRedoStateChanged(bool canUndo, bool canRedo) override {
    call<void>("onUndoRedoStateChanged", canUndo, canRedo);
  }
};

namespace internal {
class ClientBitmapWrapper : public wrapper<ClientBitmap> {
 public:
  EMSCRIPTEN_WRAPPER(ClientBitmapWrapper);
  const void* imageByteData() const override {
    return reinterpret_cast<const void*>(call<uintptr_t>("imageByteData"));
  }
  void* imageByteData() override {
    return reinterpret_cast<void*>(call<uintptr_t>("imageByteData"));
  }
};
}  // namespace internal

// Bindings for the abstract class are also required for implementing in
// JavaScript so that there is a vtable on the Emscripten heap for the class.
EMSCRIPTEN_BINDINGS(classes_implemented_in_js) {
  class_<ClientBitmap>("ClientBitmap")
      .function("imageByteData", select_const(&ClientBitmap::imageByteData),
                pure_virtual(), allow_raw_pointers())
      .function("toString", &ClientBitmap::toString)
      .allow_subclass<internal::ClientBitmapWrapper>(
          "ClientBitmapWrapper", constructor<ink::ImageSize, ImageFormat>());

  class_<Host>("Host")
      .function("setTargetFPS", &Host::SetTargetFPS, pure_virtual())
      .function("getTargetFPS", &Host::GetTargetFPS, pure_virtual())
      .function("bindScreen", &Host::BindScreen, pure_virtual())
      .function("requestImage", &Host::RequestImage, pure_virtual())
      .function("handleMutation", &Host::OnMutation, pure_virtual())
      .function("handleElementCreated", &Host::ElementsAdded, pure_virtual())
      .function("handleElementsRemoved", &Host::ElementsRemoved, pure_virtual())
      .function("getPlatformId", &Host::GetPlatformId, pure_virtual())
      .function("onSequencePointReached", &Host::SequencePointReached,
                pure_virtual())
      .function("onSceneChanged", &Host::SceneChanged, pure_virtual())
      .allow_subclass<HostWrapper>("HostWrapper");
}

// Bind all the types (protos, enums) required for SEngine method arguments.
using ::ink::ImageSize;
EMSCRIPTEN_BINDINGS(ImageSize) {
  value_array<ImageSize>("ImageSize")
      .element(&ImageSize::width)
      .element(&ImageSize::height);
}

EMSCRIPTEN_BINDINGS(geometry) {
  class_<ink::Rect>("SketchologyRect")
      .constructor<float, float, float, float>()
      .function("Left", &ink::Rect::Left)
      .function("Top", &ink::Rect::Top)
      .function("Right", &ink::Rect::Right)
      .function("Bottom", &ink::Rect::Bottom);
}

void CopyToJs(MessageLite& cpp_proto, val js_proto, val serializer) {
  std::vector<uint8_t> bytes;
  auto len = cpp_proto.ByteSize();
  bytes.resize(len);
  cpp_proto.SerializeToArray(bytes.data(), len);
  // Avoids a copy by calling the deserializer here.
  serializer.call<void>("deserializeTo", js_proto,
                        val(typed_memory_view(len, bytes.data())));
}

// NOTE(trybka): Since this method is going to be bound on objects of type T,
//               but it is not a member function, we pass a non-const ref.
//               See go/embind-ref for more information.
template <typename T>
bool InitFromJs(T& cpp_proto, val js_proto, val serializer) {
  auto bytes = CopyToHeap<std::vector<uint8_t>>(
      serializer.call<val>("serialize", js_proto));
  if (!cpp_proto.ParseFromArray(bytes.data(), bytes.size())) {
    SLOG(SLOG_ERROR, "could not deserialize protobuf");
    return false;
  }
  return true;
}

EMSCRIPTEN_BINDINGS(protos) {
  class_<MessageLite>("MessageLite")
      .function("copyToJs", &CopyToJs)
      .function("toArrayBuffer", &ToArrayBuffer<MessageLite>);

  class_<ink::proto::Element, emscripten::base<MessageLite>>("ElementProto")
      .constructor();

  class_<Point, emscripten::base<MessageLite>>("PointProto")
      .constructor()
      .property("x", &Point::x, &Point::set_x)
      .property("y", &Point::y, &Point::set_y);

  class_<ink::proto::ElementBundle, emscripten::base<MessageLite>>(
      "ElementBundleProto")
      .constructor();

  class_<ink::proto::ElementQueryData, emscripten::base<MessageLite>>(
      "ElementQueryDataProto")
      .constructor();

  class_<ink::proto::ElementIdList, emscripten::base<MessageLite>>(
      "ElementIdListProto")
      .constructor();

  using ink::proto::ElementAttributes;
  class_<ElementAttributes, emscripten::base<MessageLite>>(
      "ElementAttributesProto")
      .constructor()
      .property("selectable", &ElementAttributes::selectable,
                &ElementAttributes::set_selectable)
      .property("magic_erasable", &ElementAttributes::magic_erasable,
                &ElementAttributes::set_magic_erasable)
      .property("is_sticker", &ElementAttributes::is_sticker,
                &ElementAttributes::set_is_sticker)
      .property("is_text", &ElementAttributes::is_text,
                &ElementAttributes::set_is_text)
      .property("is_group", &ElementAttributes::is_group,
                &ElementAttributes::set_is_group)
      .property("is_zoomable", &ElementAttributes::is_zoomable,
                &ElementAttributes::set_is_zoomable);

  class_<ink::proto::VectorElements, emscripten::base<MessageLite>>(
      "VectorElementsProto")
      .constructor();

  class_<BackgroundColor, emscripten::base<MessageLite>>("BackgroundColorProto")
      .constructor()
      .property("rgba", &BackgroundColor::rgba, &BackgroundColor::set_rgba);

  class_<Path, emscripten::base<MessageLite>>("PathProto")
      .constructor()
      .property("rgba", &Path::rgba, &Path::set_rgba)
      .property("radius", &Path::radius, &Path::set_radius)
      .function("add_segment_args", &Path::add_segment_args);

  class_<AffineTransform, emscripten::base<MessageLite>>("AffineTransformProto")
      .constructor()
      .property("tx", &AffineTransform::tx, &AffineTransform::set_tx)
      .property("ty", &AffineTransform::ty, &AffineTransform::set_ty)
      .property("scale_x", &AffineTransform::scale_x,
                &AffineTransform::set_scale_x)
      .property("scale_y", &AffineTransform::scale_y,
                &AffineTransform::set_scale_y)
      .property("rotation_radians", &AffineTransform::rotation_radians,
                &AffineTransform::set_rotation_radians);

  class_<ink::proto::SequencePoint, emscripten::base<MessageLite>>(
      "SequencePointProto")
      .constructor()
      .property("id", &ink::proto::SequencePoint::id,
                &ink::proto::SequencePoint::set_id);

  class_<CameraPosition, emscripten::base<MessageLite>>("CameraPositionProto")
      .constructor()
      .function("mutable_world_center", &CameraPosition::mutable_world_center,
                allow_raw_pointers())
      .property("world_width", &CameraPosition::world_width,
                &CameraPosition::set_world_width)
      .property("world_height", &CameraPosition::world_height,
                &CameraPosition::set_world_height);

  class_<ink::proto::EngineState, emscripten::base<MessageLite>>(
      "EngineStateProto")
      .constructor();

  class_<ink::proto::SourceDetails, emscripten::base<MessageLite>>(
      "source_detailsProto")
      .constructor();

  class_<ink::proto::CallbackFlags, emscripten::base<MessageLite>>(
      "CallbackFlagsProto")
      .constructor();

  class_<ink::proto::MutationPacket, emscripten::base<MessageLite>>(
      "MutationPacketProto")
      .constructor();

  class_<Viewport, emscripten::base<MessageLite>>("ViewportProto")
      .constructor()
      .property("width", &Viewport::width, &Viewport::set_width)
      .property("height", &Viewport::height, &Viewport::set_height)
      .property("ppi", &Viewport::ppi, &Viewport::set_ppi)
      .property("screen_rotation", &Viewport::screen_rotation,
                &Viewport::set_screen_rotation);

  class_<proto::Rect, emscripten::base<MessageLite>>("RectProto")
      .constructor()
      .property("xlow", &proto::Rect::xlow, &proto::Rect::set_xlow)
      .property("ylow", &proto::Rect::ylow, &proto::Rect::set_ylow)
      .property("xhigh", &proto::Rect::xhigh, &proto::Rect::set_xhigh)
      .property("yhigh", &proto::Rect::yhigh, &proto::Rect::set_yhigh);

  class_<proto::Rects, emscripten::base<MessageLite>>("RectsProto")
      .constructor()
      .property("length", &proto::Rects::rect_size)
      .function("rectAt", select_overload<const proto::Rect&(int)const>(
                              &proto::Rects::rect));

  enum_<ImageFormat>("ImageFormat")
      .value("NONE", ImageFormat::BITMAP_FORMAT_NONE)
      .value("RGBA_8888", ImageFormat::BITMAP_FORMAT_RGBA_8888);

  class_<ImageInfo, emscripten::base<MessageLite>>("ImageInfoProto")
      .constructor()
      .property("uri", &ImageInfo::uri,
                select_overload<void(const std::string&)>(&ImageInfo::set_uri))
      .property("asset_type", &ImageInfo::asset_type,
                &ImageInfo::set_asset_type);

  class_<ImageRect, emscripten::base<MessageLite>>("ImageRectProto")
      .constructor()
      .property(
          "bitmap_uri", &ImageRect::bitmap_uri,
          select_overload<void(const std::string&)>(&ImageRect::set_bitmap_uri))
      .property("rotation_radians", &ImageRect::rotation_radians,
                &ImageRect::set_rotation_radians)
      .function("mutable_attributes", &ImageRect::mutable_attributes,
                allow_raw_pointers())
      .function("mutable_rect", &ImageRect::mutable_rect, allow_raw_pointers());

  class_<ToolParams, emscripten::base<MessageLite>>("ToolParamsProto")
      .constructor()
      .property("tool", &ToolParams::tool, &ToolParams::set_tool)
      .function("mutable_line_size", &ToolParams::mutable_line_size,
                allow_raw_pointers())
      .property("brush_type", &ToolParams::brush_type,
                &ToolParams::set_brush_type)
      .property("rgba", &ToolParams::rgba, &ToolParams::set_rgba);

  class_<LineSize, emscripten::base<MessageLite>>("LineSizeProto")
      .property("stroke_width", &LineSize::stroke_width,
                &LineSize::set_stroke_width)
      .property("use_web_sizes", &LineSize::use_web_sizes,
                &LineSize::set_use_web_sizes)
      .property("units", &LineSize::units, &LineSize::set_units);

  class_<OutOfBoundsColor, emscripten::base<MessageLite>>(
      "OutOfBoundsColorProto")
      .constructor()
      .property("rgba", &OutOfBoundsColor::rgba, &OutOfBoundsColor::set_rgba);

  class_<PageProperties, emscripten::base<MessageLite>>("PagePropertiesProto")
      .constructor()
      .property("bounds", &PageProperties::bounds);

  class_<ink::proto::LayerState, emscripten::base<MessageLite>>(
      "LayerStateProto")
      .constructor();

  class_<ink::proto::LayoutSpec, emscripten::base<MessageLite>>(
      "LayoutSpecProto")
      .property("strategy", &ink::proto::LayoutSpec::strategy,
                &ink::proto::LayoutSpec::set_strategy)
      .property("spacing_world", &ink::proto::LayoutSpec::spacing_world,
                &ink::proto::LayoutSpec::set_spacing_world)
      .constructor();
}

EMSCRIPTEN_BINDINGS(protos_with_js_init) {
  class_<Command, emscripten::base<MessageLite>>("CommandProto")
      .constructor()
      .function("initFromJs", &InitFromJs<Command>, allow_raw_pointers());

  class_<BackgroundImageInfo, emscripten::base<MessageLite>>(
      "BackgroundImageInfoProto")
      .constructor()
      .property("uri", &BackgroundImageInfo::uri,
                select_overload<void(const std::string&)>(
                    &BackgroundImageInfo::set_uri))
      .function("mutable_bounds", &BackgroundImageInfo::mutable_bounds,
                allow_raw_pointers())
      .function("initFromJs", &InitFromJs<BackgroundImageInfo>,
                allow_raw_pointers());

  class_<GridInfo, emscripten::base<MessageLite>>("GridInfoProto")
      .constructor()
      .property("uri", &GridInfo::uri,
                select_overload<void(const std::string&)>(&GridInfo::set_uri))
      .property("rgba_multipler", &GridInfo::rgba_multiplier,
                &GridInfo::set_rgba_multiplier)
      .property("size_world", &GridInfo::size_world, &GridInfo::set_size_world)
      .function("mutable_origin", &GridInfo::mutable_origin,
                allow_raw_pointers())
      .function("initFromJs", &InitFromJs<GridInfo>, allow_raw_pointers());

  class_<Border, emscripten::base<MessageLite>>("BorderProto")
      .constructor()
      .function("initFromJs", &InitFromJs<Border>, allow_raw_pointers());

  class_<SetCallbackFlags, emscripten::base<MessageLite>>(
      "SetCallbackFlagsProto")
      .constructor()
      .function("initFromJs", &InitFromJs<SetCallbackFlags>,
                allow_raw_pointers());

  class_<Snapshot, emscripten::base<MessageLite>>("SnapshotProto")
      .constructor()
      .property("fingerprint", &Snapshot::fingerprint,
                &Snapshot::set_fingerprint)
      .function("initFromJs", &InitFromJs<Snapshot>, allow_raw_pointers());

  class_<ImageExport, emscripten::base<MessageLite>>("ImageExportProto")
      .constructor()
      .function("initFromJs", &InitFromJs<ImageExport>, allow_raw_pointers());
}

EMSCRIPTEN_BINDINGS(enums) {
  using input::Flag;
  using input::InputType;
  using input::MouseIds;

  enum_<ImageInfo::AssetType>("AssetType")
      .value("DEFAULT", ImageInfo::DEFAULT)
      .value("BORDER", ImageInfo::BORDER)
      .value("GRID", ImageInfo::GRID)
      .value("STICKER", ImageInfo::STICKER)
      .value("TILED_TEXTURE", ImageInfo::TILED_TEXTURE);

  enum_<MouseIds>("MouseIds")
      .value("MOUSEHOVER", MouseIds::MouseHover)
      .value("MOUSELEFT", MouseIds::MouseLeft)
      .value("MOUSERIGHT", MouseIds::MouseRight)
      .value("MOUSEWHEEL", MouseIds::MouseWheel);

  enum_<InputType>("InputType")
      .value("INVALID", InputType::Invalid)
      .value("MOUSE", InputType::Mouse)
      .value("TOUCH", InputType::Touch)
      .value("PEN", InputType::Pen);

  enum_<Flag>("InputFlag")
      .value("INCONTACT", Flag::InContact)
      .value("LEFT", Flag::Left)
      .value("RIGHT", Flag::Right)
      .value("TDOWN", Flag::TDown)
      .value("TUP", Flag::TUp)
      .value("WHEEL", Flag::Wheel)
      .value("CANCEL", Flag::Cancel)
      .value("FAKE", Flag::Fake)
      .value("PRIMARY", Flag::Primary)
      .value("ERASER", Flag::Eraser)
      .value("SHIFT", Flag::Shift)
      .value("CONTROL", Flag::Control)
      .value("ALT", Flag::Alt)
      .value("META", Flag::Meta);

  enum_<LineSize::SizeType>("BrushSizeType")
      .value("WORLD_UNITS", LineSize::WORLD_UNITS)
      .value("POINTS", LineSize::POINTS)
      .value("PERCENT_WORLD", LineSize::PERCENT_WORLD);

  enum_<ToolParams::ToolType>("ToolType")
      .value("LINE", ToolParams::LINE)
      .value("EDIT", ToolParams::EDIT)
      .value("MAGIC_ERASE", ToolParams::MAGIC_ERASE)
      .value("NOTOOL", ToolParams::NOTOOL)
      .value("PUSHER", ToolParams::PUSHER)
      .value("SMART_HIGHLIGHTER_TOOL", ToolParams::SMART_HIGHLIGHTER_TOOL)
      .value("TEXT_HIGHLIGHTER_TOOL", ToolParams::TEXT_HIGHLIGHTER_TOOL)
      .value("STROKE_EDITING_ERASER", ToolParams::STROKE_EDITING_ERASER);

  enum_<BrushType>("BrushType")
      .value("CALLIGRAPHY", BrushType::CALLIGRAPHY)
      .value("INKPEN", BrushType::INKPEN)
      .value("MARKER", BrushType::MARKER)
      .value("BALLPOINT", BrushType::BALLPOINT)
      .value("ERASER", BrushType::ERASER)
      .value("HIGHLIGHTER", BrushType::HIGHLIGHTER)
      .value("BALLPOINT_IN_PEN_MODE_ELSE_MARKER",
             BrushType::BALLPOINT_IN_PEN_MODE_ELSE_MARKER)
      .value("PENCIL", BrushType::PENCIL)
      .value("CHARCOAL", BrushType::CHARCOAL);

  enum_<ink::proto::Flag>("Flag")
      .value("READ_ONLY_MODE", ink::proto::Flag::READ_ONLY_MODE)
      .value("ENABLE_PAN_ZOOM", ink::proto::Flag::ENABLE_PAN_ZOOM)
      .value("ENABLE_ROTATION", ink::proto::Flag::ENABLE_ROTATION)
      .value("ENABLE_AUTO_PEN_MODE", ink::proto::Flag::ENABLE_AUTO_PEN_MODE)
      .value("ENABLE_PEN_MODE", ink::proto::Flag::ENABLE_PEN_MODE)
      .value("ENABLE_FLING", ink::proto::Flag::ENABLE_FLING)
      .value("ENABLE_HOST_CAMERA_CONTROL",
             ink::proto::Flag::ENABLE_HOST_CAMERA_CONTROL)
      .value("OPAQUE_PREDICTED_SEGMENT",
             ink::proto::Flag::OPAQUE_PREDICTED_SEGMENT)
      .value("STRICT_NO_MARGINS", ink::proto::Flag::STRICT_NO_MARGINS)
      .value("KEEP_MESHES_IN_CPU_MEMORY",
             ink::proto::Flag::KEEP_MESHES_IN_CPU_MEMORY);

  enum_<ink::Document::SnapshotQuery>("SnapshotQuery")
      .value("INCLUDE_UNDO_STACK",
             ink::Document::SnapshotQuery::INCLUDE_UNDO_STACK)
      .value("DO_NOT_INCLUDE_UNDO_STACK",
             ink::Document::SnapshotQuery::DO_NOT_INCLUDE_UNDO_STACK);

  enum_<ink::proto::MouseWheelBehavior>("MouseWheelBehavior")
      .value("ZOOMS", ink::proto::MouseWheelBehavior::ZOOMS)
      .value("SCROLLS", ink::proto::MouseWheelBehavior::SCROLLS);

  enum_<ink::proto::LayoutStrategy>("LayoutStrategy")
      .value("VERTICAL", ink::proto::LayoutStrategy::VERTICAL)
      .value("HORIZONTAL", ink::proto::LayoutStrategy::HORIZONTAL);
}

namespace embind_fingerprint {
void SetFingerprint(ink::proto::Snapshot* snapshot) {
  uint64_t fingerprint = ink::GetFingerprint(*snapshot);
  snapshot->set_fingerprint(fingerprint);
}
}  // namespace embind_fingerprint

EMSCRIPTEN_BINDINGS(Fingerprinter) {
  function("SetFingerprint", &embind_fingerprint::SetFingerprint,
           allow_raw_pointers());
}

}  // namespace ink
#endif  // defined(__asmjs__) || defined(__wasm__)
