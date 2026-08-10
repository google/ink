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

#import "ink/rendering/metal/metal_renderer_objc_helper.h"

#import <CoreFoundation/CFBase.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

#import <functional>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "ink/brush/brush_paint.h"
#include "ink/geometry/mesh.h"
#import "ink/rendering/metal/INKTextureBitmapStore.h"
#include "ink/rendering/metal/ink_metal_shaders_embedded.h"

__attribute__((objc_subclassing_restricted))
@interface INKMeshBuffers : NSObject

@property(nonatomic, readonly, nonnull) id<MTLBuffer> vertexBuffer;
@property(nonatomic, readonly, nonnull) id<MTLBuffer> indexBuffer;

- (instancetype)initWithVertexBuffer:(id<MTLBuffer>)vertexBuffer
                         indexBuffer:(id<MTLBuffer>)indexBuffer NS_DESIGNATED_INITIALIZER;

- (instancetype)init NS_UNAVAILABLE;

@end

@implementation INKMeshBuffers

- (instancetype)initWithVertexBuffer:(id<MTLBuffer>)vertexBuffer
                         indexBuffer:(id<MTLBuffer>)indexBuffer {
  self = [super init];
  if (self) {
    _vertexBuffer = vertexBuffer;
    _indexBuffer = indexBuffer;
  }
  return self;
}

@end

// Objective-C class holding Metal rendering pipeline state objects by value.
__attribute__((objc_subclassing_restricted))
@interface INKMetalRendererState : NSObject

@property(nonatomic, readonly, nonnull) id<MTLDevice> device;
@property(nonatomic, readonly, nonnull) id<MTLRenderPipelineState> pipelineState;
@property(nonatomic, readonly, nonnull) id<MTLDepthStencilState> discardSelfOverlapStencilState;
@property(nonatomic, readonly, nonnull) id<MTLDepthStencilState> clearStencilBufferStencilState;
@property(nonatomic, readonly, nonnull) id<MTLDepthStencilState> noOpStencilState;
@property(nonatomic, readonly, nullable) id<INKTextureBitmapStore> textureBitmapStore;
@property(nonatomic, readonly, nonnull) MTKTextureLoader* textureLoader;
@property(nonatomic, readonly, nonnull)
    NSMutableDictionary<NSString*, id<MTLTexture>>* textureCache;
@property(nonatomic, readonly, nonnull)
    NSMutableArray<NSMutableArray<id<MTLSamplerState>>*>* samplerCache;

- (instancetype)init NS_UNAVAILABLE;

- (nullable instancetype)
                    initWithDevice:(nonnull id<MTLDevice>)device
                     pipelineState:(nonnull id<MTLRenderPipelineState>)pipelineState
    discardSelfOverlapStencilState:(nullable id<MTLDepthStencilState>)discardSelfOverlapStencilState
    clearStencilBufferStencilState:(nullable id<MTLDepthStencilState>)clearStencilBufferStencilState
                  noOpStencilState:(nullable id<MTLDepthStencilState>)noOpStencilState
                textureBitmapStore:(nullable id<INKTextureBitmapStore>)textureBitmapStore
    NS_DESIGNATED_INITIALIZER;

@end

@implementation INKMetalRendererState

- (nullable instancetype)
                    initWithDevice:(nonnull id<MTLDevice>)device
                     pipelineState:(nonnull id<MTLRenderPipelineState>)pipelineState
    discardSelfOverlapStencilState:(nullable id<MTLDepthStencilState>)discardSelfOverlapStencilState
    clearStencilBufferStencilState:(nullable id<MTLDepthStencilState>)clearStencilBufferStencilState
                  noOpStencilState:(nullable id<MTLDepthStencilState>)noOpStencilState
                textureBitmapStore:(nullable id<INKTextureBitmapStore>)textureBitmapStore {
  self = [super init];
  if (self) {
    _device = device;
    _pipelineState = pipelineState;
    _discardSelfOverlapStencilState = discardSelfOverlapStencilState;
    _clearStencilBufferStencilState = clearStencilBufferStencilState;
    _noOpStencilState = noOpStencilState;
    _textureBitmapStore = textureBitmapStore;
    _textureLoader = [[MTKTextureLoader alloc] initWithDevice:device];
    _textureCache = [[NSMutableDictionary<NSString*, id<MTLTexture>> alloc] init];
    _samplerCache =
        [[NSMutableArray<NSMutableArray<id<MTLSamplerState>>*> alloc] initWithCapacity:3];
    for (int i = 0; i < 3; ++i) {
      NSMutableArray<id<MTLSamplerState>>* row =
          [[NSMutableArray<id<MTLSamplerState>> alloc] initWithCapacity:3];
      for (int j = 0; j < 3; ++j) {
        [row addObject:(id<MTLSamplerState>)[NSNull null]];
      }
      [_samplerCache addObject:row];
    }
  }
  return self;
}

@end

namespace ink::rendering::metal_objc {

namespace {

MTLSamplerAddressMode TextureWrapToMTLSamplerAddressMode(BrushPaint::TextureWrap wrap) {
  switch (wrap) {
    case BrushPaint::TextureWrap::kRepeat:
      return MTLSamplerAddressModeRepeat;
    case BrushPaint::TextureWrap::kMirror:
      return MTLSamplerAddressModeMirrorRepeat;
    case BrushPaint::TextureWrap::kClamp:
      return MTLSamplerAddressModeClampToEdge;
    default:
      return MTLSamplerAddressModeRepeat;
  }
}

absl::StatusOr<std::unique_ptr<void, std::function<void(void*)>>> CreateINKMetalRendererState(
    id<MTLDevice> device, MTLPixelFormat color_pixel_format, MTLPixelFormat stencil_pixel_format,
    std::optional<int> sample_count, id<INKTextureBitmapStore> texture_bitmap_store) {
  if (!device) {
    return absl::InvalidArgumentError("Device cannot be nil");
  }

  dispatch_data_t shader_data = dispatch_data_create(kInkMetalShaders, std::size(kInkMetalShaders),
                                                     /*queue=*/nullptr,
                                                     /*destructor=*/nullptr);
  NSError* error = nil;
  id<MTLLibrary> shader_library = [device newLibraryWithData:shader_data error:&error];
  if (!shader_library) {
    return absl::InternalError(absl::StrCat("Failed to create Metal shader library",
                                            error ? ": " : "",
                                            error ? error.localizedDescription.UTF8String : ""));
  }
  id<MTLFunction> vertex_function = [shader_library newFunctionWithName:@"vertexMain"];
  id<MTLFunction> fragment_function = [shader_library newFunctionWithName:@"fragmentMain"];
  if (!vertex_function || !fragment_function) {
    return absl::InternalError("Failed to load vertexMain or fragmentMain shader functions");
  }

  MTLRenderPipelineDescriptor* pipeline_descriptor = [[MTLRenderPipelineDescriptor alloc] init];
  pipeline_descriptor.vertexFunction = vertex_function;
  pipeline_descriptor.fragmentFunction = fragment_function;
  pipeline_descriptor.colorAttachments[0].pixelFormat = color_pixel_format;

  // These blending settings are appropriate for colors with pre-multiplied alpha, which are
  // output by the shaders.
  pipeline_descriptor.colorAttachments[0].blendingEnabled = YES;
  pipeline_descriptor.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorOne;
  pipeline_descriptor.colorAttachments[0].destinationRGBBlendFactor =
      MTLBlendFactorOneMinusSourceAlpha;
  pipeline_descriptor.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorOne;
  pipeline_descriptor.colorAttachments[0].destinationAlphaBlendFactor =
      MTLBlendFactorOneMinusSourceAlpha;
  pipeline_descriptor.stencilAttachmentPixelFormat = stencil_pixel_format;
  if (sample_count.has_value()) {
    pipeline_descriptor.rasterSampleCount = sample_count.value();
  }

  id<MTLRenderPipelineState> pipeline_state =
      [device newRenderPipelineStateWithDescriptor:pipeline_descriptor error:&error];
  if (!pipeline_state) {
    return absl::InternalError(error ? error.localizedDescription.UTF8String
                                     : "Failed to create render pipeline state");
  }

  // Discard self-overlapping areas of a stroke by incrementing the stencil buffer for each
  // fragment, and discarding any fragment where the stencil buffer is greater than the
  // reference value of 0.
  MTLStencilDescriptor* stencil_pass = [[MTLStencilDescriptor alloc] init];
  stencil_pass.stencilCompareFunction = MTLCompareFunctionEqual;
  stencil_pass.stencilFailureOperation = MTLStencilOperationKeep;
  stencil_pass.depthFailureOperation = MTLStencilOperationKeep;
  stencil_pass.depthStencilPassOperation = MTLStencilOperationIncrementClamp;

  MTLDepthStencilDescriptor* discard_descriptor = [[MTLDepthStencilDescriptor alloc] init];
  discard_descriptor.frontFaceStencil = stencil_pass;
  discard_descriptor.backFaceStencil = stencil_pass;
  id<MTLDepthStencilState> discard_self_overlap_stencil_state =
      [device newDepthStencilStateWithDescriptor:discard_descriptor];

  // Clear the stencil buffer by replacing the stencil value with the reference value of 0 for
  // all fragments. This must be used with a mesh that covers the entire viewport.
  MTLStencilDescriptor* clear_pass = [[MTLStencilDescriptor alloc] init];
  clear_pass.stencilCompareFunction = MTLCompareFunctionAlways;
  clear_pass.stencilFailureOperation = MTLStencilOperationZero;
  clear_pass.depthFailureOperation = MTLStencilOperationZero;
  clear_pass.depthStencilPassOperation = MTLStencilOperationZero;

  MTLDepthStencilDescriptor* clear_descriptor = [[MTLDepthStencilDescriptor alloc] init];
  clear_descriptor.frontFaceStencil = clear_pass;
  clear_descriptor.backFaceStencil = clear_pass;
  id<MTLDepthStencilState> clear_stencil_buffer_stencil_state =
      [device newDepthStencilStateWithDescriptor:clear_descriptor];

  MTLDepthStencilDescriptor* no_op_descriptor = [[MTLDepthStencilDescriptor alloc] init];
  id<MTLDepthStencilState> no_op_stencil_state =
      [device newDepthStencilStateWithDescriptor:no_op_descriptor];

  INKMetalRendererState* renderer_state =
      [[INKMetalRendererState alloc] initWithDevice:device
                                      pipelineState:pipeline_state
                     discardSelfOverlapStencilState:discard_self_overlap_stencil_state
                     clearStencilBufferStencilState:clear_stencil_buffer_stencil_state
                                   noOpStencilState:no_op_stencil_state
                                 textureBitmapStore:texture_bitmap_store];

  return std::unique_ptr<void, std::function<void(void*)>>((__bridge_retained void*)renderer_state,
                                                           &CFRelease);
}

absl_nullable std::shared_ptr<void> CreateINKMeshBuffers(INKMetalRendererState* state,
                                                         const void* vertex_data,
                                                         size_t vertex_data_bytes,
                                                         const void* index_data,
                                                         size_t index_data_bytes) {
  id<MTLBuffer> vertex_buffer = [state.device newBufferWithBytes:vertex_data
                                                          length:vertex_data_bytes
                                                         options:MTLResourceStorageModeShared];
  id<MTLBuffer> index_buffer = [state.device newBufferWithBytes:index_data
                                                         length:index_data_bytes
                                                        options:MTLResourceStorageModeShared];
  if (!vertex_buffer || !index_buffer) {
    return nullptr;
  }
  INKMeshBuffers* buffers = [[INKMeshBuffers alloc] initWithVertexBuffer:vertex_buffer
                                                             indexBuffer:index_buffer];
  return std::shared_ptr<void>((__bridge_retained void*)buffers, &CFRelease);
}

void SetRenderPipelineState(id<MTLRenderCommandEncoder> render_encoder,
                            INKMetalRendererState* renderer_state) {
  [render_encoder setRenderPipelineState:renderer_state.pipelineState];
}

void SetDepthStencilState(id<MTLRenderCommandEncoder> render_encoder,
                          id<MTLDepthStencilState> depth_stencil_state) {
  [render_encoder setDepthStencilState:depth_stencil_state];
  [render_encoder setStencilReferenceValue:0];
}

void SetClearStencilBufferStencilState(id<MTLRenderCommandEncoder> render_encoder,
                                       INKMetalRendererState* renderer_state) {
  SetDepthStencilState(render_encoder, renderer_state.clearStencilBufferStencilState);
}

void SetDiscardSelfOverlapStencilState(id<MTLRenderCommandEncoder> render_encoder,
                                       INKMetalRendererState* renderer_state) {
  SetDepthStencilState(render_encoder, renderer_state.discardSelfOverlapStencilState);
}

void SetNoOpStencilState(id<MTLRenderCommandEncoder> render_encoder,
                         INKMetalRendererState* renderer_state) {
  SetDepthStencilState(render_encoder, renderer_state.noOpStencilState);
}

void SetFragmentTextureAndSampler(id<MTLRenderCommandEncoder> render_encoder,
                                  id<MTLTexture> texture, id<MTLSamplerState> sampler,
                                  uint32_t index) {
  [render_encoder setFragmentTexture:texture atIndex:index];
  [render_encoder setFragmentSamplerState:sampler atIndex:index];
}

void DrawIndexedTriangles(id<MTLRenderCommandEncoder> render_encoder, const void* uniforms_data,
                          size_t uniforms_data_bytes, uint32_t triangle_count, size_t index_stride,
                          INKMeshBuffers* absl_nullable mesh_buffers) {
  if (!render_encoder || !uniforms_data || triangle_count == 0 || !mesh_buffers) {
    return;
  }

  // Provide the same uniforms data to the vertex and fragment shaders.
  [render_encoder setVertexBytes:uniforms_data length:uniforms_data_bytes atIndex:0];
  [render_encoder setFragmentBytes:uniforms_data length:uniforms_data_bytes atIndex:0];

  MTLIndexType index_type = (index_stride == 2) ? MTLIndexTypeUInt16 : MTLIndexTypeUInt32;
  [render_encoder setVertexBuffer:mesh_buffers.vertexBuffer offset:0 atIndex:1];
  [render_encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                             indexCount:triangle_count * 3
                              indexType:index_type
                            indexBuffer:mesh_buffers.indexBuffer
                      indexBufferOffset:0];
}

id<MTLTexture> GetOrLoadMTLTexture(INKMetalRendererState* state, const char* client_texture_id) {
  if (!client_texture_id) return nullptr;
  id<INKTextureBitmapStore> texture_bitmap_store = state.textureBitmapStore;
  if (!texture_bitmap_store) return nullptr;

  NSMutableDictionary<NSString*, id<MTLTexture>>* texture_cache = state.textureCache;
  MTKTextureLoader* texture_loader = state.textureLoader;

  NSString* ns_texture_id = [NSString stringWithUTF8String:client_texture_id];
  id<MTLTexture> texture = texture_cache[ns_texture_id];
  if (!texture && texture_bitmap_store) {
    CGImageRef cg_image = [texture_bitmap_store textureForID:ns_texture_id];
    if (cg_image) {
      NSError* error = nil;
      texture = [texture_loader newTextureWithCGImage:cg_image options:nil error:&error];
      if (texture) {
        texture_cache[ns_texture_id] = texture;
      }
    }
  }
  return texture;
}

id<MTLSamplerState> GetOrCreateMTLSamplerState(INKMetalRendererState* state,
                                               BrushPaint::TextureWrap wrap_x,
                                               BrushPaint::TextureWrap wrap_y) {
  if (!state || !state.samplerCache) return nullptr;

  int x_idx = static_cast<int>(wrap_x);
  int y_idx = static_cast<int>(wrap_y);
  if (x_idx < 0 || x_idx >= 3 || y_idx < 0 || y_idx >= 3) {
    return nullptr;
  }

  NSMutableArray<id<MTLSamplerState>>* row = state.samplerCache[x_idx];
  id existing = row[y_idx];
  if (existing != [NSNull null] && existing != nil) {
    return existing;
  }

  MTLSamplerDescriptor* descriptor = [[MTLSamplerDescriptor alloc] init];
  descriptor.sAddressMode = TextureWrapToMTLSamplerAddressMode(wrap_x);
  descriptor.tAddressMode = TextureWrapToMTLSamplerAddressMode(wrap_y);
  descriptor.minFilter = MTLSamplerMinMagFilterLinear;
  descriptor.magFilter = MTLSamplerMinMagFilterLinear;
  descriptor.mipFilter = MTLSamplerMipFilterNotMipmapped;

  id<MTLSamplerState> sampler = [state.device newSamplerStateWithDescriptor:descriptor];
  if (sampler != nil) {
    row[y_idx] = sampler;
  }
  return sampler;
}

bool SupportsSelfOverlapDiscard(INKMetalRendererState* state) {
  return state.discardSelfOverlapStencilState != nil && state.clearStencilBufferStencilState != nil;
}

bool SupportsTextureLookup(INKMetalRendererState* state) { return state.textureBitmapStore != nil; }

}  // namespace

absl::StatusOr<std::unique_ptr<void, std::function<void(void*)>>> CreateINKMetalRendererState(
    void* device_ptr, uint64_t color_pixel_format_val, uint64_t stencil_pixel_format_val,
    std::optional<int> sample_count, void* absl_nullable texture_bitmap_store_ptr) {
  return CreateINKMetalRendererState(
      (__bridge id<MTLDevice>)device_ptr, static_cast<MTLPixelFormat>(color_pixel_format_val),
      static_cast<MTLPixelFormat>(stencil_pixel_format_val), sample_count,
      (__bridge id<INKTextureBitmapStore>)texture_bitmap_store_ptr);
}

absl_nullable std::shared_ptr<void> CreateINKMeshBuffers(void* renderer_state_ptr,
                                                         const void* vertex_data,
                                                         size_t vertex_data_bytes,
                                                         const void* index_data,
                                                         size_t index_data_bytes) {
  return CreateINKMeshBuffers((__bridge INKMetalRendererState*)renderer_state_ptr, vertex_data,
                              vertex_data_bytes, index_data, index_data_bytes);
}

void SetRenderPipelineState(void* render_encoder_ptr, void* renderer_state_ptr) {
  SetRenderPipelineState((__bridge id<MTLRenderCommandEncoder>)render_encoder_ptr,
                         (__bridge INKMetalRendererState*)renderer_state_ptr);
}

void SetClearStencilBufferStencilState(void* render_encoder_ptr, void* renderer_state_ptr) {
  SetClearStencilBufferStencilState((__bridge id<MTLRenderCommandEncoder>)render_encoder_ptr,
                                    (__bridge INKMetalRendererState*)renderer_state_ptr);
}

void SetDiscardSelfOverlapStencilState(void* render_encoder_ptr, void* renderer_state_ptr) {
  SetDiscardSelfOverlapStencilState((__bridge id<MTLRenderCommandEncoder>)render_encoder_ptr,
                                    (__bridge INKMetalRendererState*)renderer_state_ptr);
}

void SetNoOpStencilState(void* render_encoder_ptr, void* renderer_state_ptr) {
  SetNoOpStencilState((__bridge id<MTLRenderCommandEncoder>)render_encoder_ptr,
                      (__bridge INKMetalRendererState*)renderer_state_ptr);
}

void SetFragmentTextureAndSampler(void* render_encoder_ptr, void* texture_ptr, void* sampler_ptr,
                                  uint32_t index) {
  SetFragmentTextureAndSampler((__bridge id<MTLRenderCommandEncoder>)render_encoder_ptr,
                               (__bridge id<MTLTexture>)texture_ptr,
                               (__bridge id<MTLSamplerState>)sampler_ptr, index);
}

void DrawIndexedTriangles(void* render_encoder_ptr, const void* uniforms_data,
                          size_t uniforms_data_bytes, uint32_t triangle_count, size_t index_stride,
                          void* mesh_buffers_ptr) {
  DrawIndexedTriangles((__bridge id<MTLRenderCommandEncoder>)render_encoder_ptr, uniforms_data,
                       uniforms_data_bytes, triangle_count, index_stride,
                       (__bridge INKMeshBuffers*)mesh_buffers_ptr);
}

void* GetOrLoadMTLTexture(void* renderer_state_ptr, const char* client_texture_id) {
  return (__bridge void*)GetOrLoadMTLTexture((__bridge INKMetalRendererState*)renderer_state_ptr,
                                             client_texture_id);
}

void* GetOrCreateMTLSamplerState(void* renderer_state_ptr, BrushPaint::TextureWrap wrap_x,
                                 BrushPaint::TextureWrap wrap_y) {
  return (__bridge void*)GetOrCreateMTLSamplerState(
      (__bridge INKMetalRendererState*)renderer_state_ptr, wrap_x, wrap_y);
}

bool SupportsSelfOverlapDiscard(void* renderer_state_ptr) {
  return SupportsSelfOverlapDiscard((__bridge INKMetalRendererState*)renderer_state_ptr);
}

bool SupportsTextureLookup(void* renderer_state_ptr) {
  return SupportsTextureLookup((__bridge INKMetalRendererState*)renderer_state_ptr);
}

}  // namespace ink::rendering::metal_objc
