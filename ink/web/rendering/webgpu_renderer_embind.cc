// Copyright 2026 Google LLC
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

#include <emscripten/bind.h>
#include <emscripten/emscripten.h>
#include <emscripten/val.h>
#include <webgpu/webgpu.h>
#include <webgpu/webgpu_cpp.h>

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/log/absl_check.h"
#include "ink/color/color.h"
#include "ink/geometry/affine_transform.h"
#include "ink/rendering/webgpu/webgpu_renderer.h"
#include "ink/strokes/in_progress_stroke.h"
#include "ink/strokes/stroke.h"

extern "C" {
WGPUDevice emscripten_webgpu_get_device();
}

namespace ink::web {
namespace {

using ::ink::rendering::webgpu::WebGpuRenderer;
using ::ink::rendering::webgpu::WebGpuTextureStore;

wgpu::Device GetPreinitializedDevice() {
  return wgpu::Device::Acquire(emscripten_webgpu_get_device());
}

AffineTransform MakeCanvasToNdcTransform(float width, float height) {
  return AffineTransform(2.0f / width, 0.0f, -1.0f, 0.0f, -2.0f / height, 1.0f);
}

class WebGpuTextureStoreWrapper {
 public:
  WebGpuTextureStoreWrapper()
      : store_(std::make_shared<WebGpuTextureStore>()) {}

  void SetRgba8Texture(const std::string& id, uint32_t width, uint32_t height,
                       emscripten::val rgba_uint8_array) {
    std::vector<uint8_t> bytes =
        emscripten::convertJSArrayToNumberVector<uint8_t>(rgba_uint8_array);
    wgpu::Device device = GetPreinitializedDevice();
    auto status_or = store_->SetRgba8Texture(id, device, width, height, bytes);
    ABSL_CHECK_OK(status_or.status());
  }

  bool HasTexture(const std::string& id) const {
    return static_cast<bool>(store_->GetTexture(id));
  }

  std::shared_ptr<const WebGpuTextureStore> GetStore() const { return store_; }

 private:
  std::shared_ptr<WebGpuTextureStore> store_;
};

class WebGpuRendererWrapper {
 public:
  static std::unique_ptr<WebGpuRendererWrapper> Create(
      const WebGpuTextureStoreWrapper* texture_store_wrapper) {
    wgpu::Device device = GetPreinitializedDevice();
    std::shared_ptr<const WebGpuTextureStore> store =
        texture_store_wrapper != nullptr ? texture_store_wrapper->GetStore()
                                         : nullptr;
    auto renderer_or = WebGpuRenderer::Create(
        std::move(device), std::move(store), wgpu::TextureFormat::RGBA8Unorm,
        wgpu::TextureFormat::Stencil8);
    ABSL_CHECK_OK(renderer_or.status());
    return std::unique_ptr<WebGpuRendererWrapper>(
        new WebGpuRendererWrapper(*std::move(renderer_or)));
  }

  void BeginRenderPass(uint32_t width, uint32_t height,
                       const Color& clear_color) {
    wgpu::TextureDescriptor tex_desc{};
    tex_desc.dimension = wgpu::TextureDimension::e2D;
    tex_desc.size = {width, height, 1};
    tex_desc.format = wgpu::TextureFormat::RGBA8Unorm;
    tex_desc.usage =
        wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::CopySrc;
    target_texture_ = renderer_.GetDevice().CreateTexture(&tex_desc);
    ABSL_CHECK_OK(renderer_.BeginRenderPass(target_texture_, clear_color));
  }

  void DrawInProgressStroke(const InProgressStroke& stroke, float width,
                            float height) {
    AffineTransform object_to_canvas = AffineTransform::Identity();
    AffineTransform canvas_to_ndc = MakeCanvasToNdcTransform(width, height);
    ABSL_CHECK_OK(
        renderer_.DrawInActivePass(stroke, object_to_canvas, canvas_to_ndc));
  }

  void DrawStroke(const Stroke& stroke, float width, float height) {
    AffineTransform object_to_canvas = AffineTransform::Identity();
    AffineTransform canvas_to_ndc = MakeCanvasToNdcTransform(width, height);
    ABSL_CHECK_OK(
        renderer_.DrawInActivePass(stroke, object_to_canvas, canvas_to_ndc));
  }

  emscripten::val EndRenderPass() {
    ABSL_CHECK_OK(renderer_.EndRenderPass());
    emscripten::val js_webgpu = emscripten::val::module_property("WebGPU");
    return js_webgpu.call<emscripten::val>(
        "getJsObject", reinterpret_cast<uintptr_t>(target_texture_.Get()));
  }

 private:
  explicit WebGpuRendererWrapper(WebGpuRenderer renderer)
      : renderer_(std::move(renderer)) {}

  WebGpuRenderer renderer_;
  wgpu::Texture target_texture_;
};

EMSCRIPTEN_BINDINGS(ink_webgpu_renderer_bindings) {
  emscripten::class_<WebGpuTextureStoreWrapper>("WebGpuTextureStore")
      .constructor<>()
      .function("setRgba8Texture", &WebGpuTextureStoreWrapper::SetRgba8Texture)
      .function("hasTexture", &WebGpuTextureStoreWrapper::HasTexture);

  emscripten::class_<WebGpuRendererWrapper>("WebGpuRenderer")
      .class_function("create", &WebGpuRendererWrapper::Create,
                      emscripten::allow_raw_pointers())
      .function("beginRenderPass", &WebGpuRendererWrapper::BeginRenderPass)
      .function("drawInProgressStroke",
                &WebGpuRendererWrapper::DrawInProgressStroke)
      .function("drawStroke", &WebGpuRendererWrapper::DrawStroke)
      .function("endRenderPass", &WebGpuRendererWrapper::EndRenderPass);
}

}  // namespace
}  // namespace ink::web
