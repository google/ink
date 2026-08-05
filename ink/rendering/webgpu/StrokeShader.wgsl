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

// WGSL shaders that are used by Ink's non-Skia renderers.
//
// This file is based on the Skia shaders in the following files:
// - ../skia/common_internal/mesh_specification_data.cc
// - ../skia/common_internal/sksl_common_shader_helper_functions.h
// - ../skia/common_internal/sksl_vertex_shader_helper_functions.h
// - ../skia/common_internal/sksl_fragment_shader_helper_functions.h
//
// The WGSL spec differs from SkSL in some cases, so the logic in this file
// can't always be a direct match of the SkSL logic. But this code is structured
// to make it as easy as possible to keep the logic in sync. For more
// explanation on the logic in this file, see the comments in the Skia shader
// files listed above, as those comments are not duplicated here.

// The varyings passed from the vertex shader to the fragment shader.
//
// Grouped into 16 byte chunks, as some rendering backends copy varyings data in
// 16-byte increments.
struct VertexOut {
  // ===========================================================================
  // 16 bytes each
  // ===========================================================================
  @builtin(position) position: vec4<f32>,
  @location(0) color: vec4<f32>,

  // ===========================================================================
  // 16 bytes total
  // ===========================================================================
  // Separate from `position`, since that is in clip space, but we need the
  // position in stroke coordinates for texture mapping.
  @location(1) positionInStrokeCoords: vec2<f32>,
  // Note that this is not the same as the `textureCoords` varying in the
  // Skia shaders. The Skia shaders branch on the texture mapping mode in the
  // vertex shader because its fragment shader isn't responsible for texture
  // sampling, so its `textureCoords` varying may contain either the `surfaceUV`
  // vertex data (in normalized texture coordinates) or the vertex position (in
  // stroke coordinates). But in WGSL, the fragment shader is responsible for
  // texture sampling, so we always use normalized texture coordinates here, and
  // branch on the texture mapping mode in the fragment shader.
  @location(2) surfaceUV: vec2<f32>,

  // ===========================================================================
  // ********* Only 8 bytes used, padding added to make 16 bytes total *********
  // ===========================================================================
  // If anti-aliasing is enabled according to isAAEnabled() below, the following
  // values are valid.
  @location(3) aaPixelsPerDimension: vec2<f32>,
  // Padding to make the struct 16-byte aligned.
  @location(4) unusedPadding: vec2<f32>,

  // ===========================================================================
  // 16 bytes each
  // ===========================================================================
  @location(5) aaNormalizedToEdgeLRFB: vec4<f32>,
  @location(6) aaOutsetPixelsLRFB: vec4<f32>,
}

struct TextureLayer {
  // ===========================================================================
  // 16 bytes total
  // ===========================================================================
  mappingMode: u32,
  blendMode: u32,
  tilingOrigin: u32,
  tilingSizeUnit: u32,

  // ===========================================================================
  // 16 bytes total
  // ===========================================================================
  tilingSize: vec2<f32>,
  tilingOffset: vec2<f32>,

  // ===========================================================================
  // ********* Only 4 bytes used, padding added to make 16 bytes total *********
  // ===========================================================================
  tilingRotation: f32,
  padding0: f32,
  padding1: f32,
  padding2: f32,
};

// LINT.IfChange(max_texture_layers)
const MAX_TEXTURE_LAYERS = 4u;
// LINT.ThenChange(
//   :texture_layer_bindings,
//   ../../ios/rendering/metal/INKMetalRenderer.swift:max_texture_layers,
//   ../../brush/brush_paint.cc:max_texture_layers
// )

// The uniforms passed to the vertex and fragment shaders.
//
// Grouped into 16 byte chunks, as some rendering backends copy uniform data in
// 16-byte increments.
// LINT.IfChange(uniforms)
struct Uniforms {
  // ===========================================================================
  // 16 byte multiples each
  // ===========================================================================
  modelTransform: mat4x4<f32>,
  viewTransform: mat4x4<f32>,
  projectionTransform: mat4x4<f32>,

  // ===========================================================================
  // 16 bytes each
  // ===========================================================================
  color: vec4<f32>,
  positionUnpackingTransform: vec4<f32>,
  sideDerivativeUnpackingTransform: vec4<f32>,
  forwardDerivativeUnpackingTransform: vec4<f32>,

  // ===========================================================================
  // 16 bytes total
  // ===========================================================================
  // Stride in bytes. This is positive for packed vertex data and negative for
  // unpacked vertex data.
  vertexStride: i32,

  // Offsets of each renderer vertex attribute in bytes.
  // A negative value means the attribute is not present.
  positionAndOpacityShiftOffset: u32,
  hslShiftOffset: i32,
  sideDerivativeAndLabelOffset: i32,

  // ===========================================================================
  // 16 bytes total
  // ===========================================================================
  forwardDerivativeAndLabelOffset: i32,
  surfaceUvAndAnimationOffsetOffset: i32,
  brushSize: f32,
  textureLayerCount: u32,

  // ===========================================================================
  // 16 bytes total
  // ===========================================================================
  firstInputPos: vec2<f32>,
  lastInputPos: vec2<f32>,

  // ===========================================================================
  // Multiples of 16 bytes each
  // ===========================================================================
  layers: array<TextureLayer, MAX_TEXTURE_LAYERS>,
}
// LINT.ThenChange(../../ios/rendering/metal/INKMetalRenderer.swift:uniforms)

@group(0) @binding(0) var<uniform> uniforms: Uniforms;
@group(0) @binding(1) var<storage, read> vertices: array<f32>;
// LINT.IfChange(texture_layer_bindings)
@group(0) @binding(2) var texture0: texture_2d<f32>;
@group(0) @binding(3) var texture1: texture_2d<f32>;
@group(0) @binding(4) var texture2: texture_2d<f32>;
@group(0) @binding(5) var texture3: texture_2d<f32>;
@group(0) @binding(6) var sampler0: sampler;
@group(0) @binding(7) var sampler1: sampler;
@group(0) @binding(8) var sampler2: sampler;
@group(0) @binding(9) var sampler3: sampler;
// LINT.ThenChange(:max_texture_layers)

// LINT.IfChange(texture_origins)
const TEXTURE_ORIGIN_STROKE_SPACE_ORIGIN: u32 = 0u;
const TEXTURE_ORIGIN_FIRST_STROKE_INPUT: u32 = 1u;
const TEXTURE_ORIGIN_LAST_STROKE_INPUT: u32 = 2u;
// LINT.ThenChange(../../ios/brush/INKTextureLayer.h:texture_origins)

// LINT.IfChange(texture_size_units)
const TEXTURE_SIZE_UNIT_BRUSH_SIZE: u32 = 0u;
const TEXTURE_SIZE_UNIT_STROKE_COORDINATES: u32 = 1u;
// LINT.ThenChange(../../ios/brush/INKTextureLayer.h:texture_size_units)

// LINT.IfChange(texture_mapping_modes)
const TEXTURE_MAPPING_MODE_TILING: u32 = 0u;
const TEXTURE_MAPPING_MODE_STAMPING: u32 = 1u;
// LINT.ThenChange(
//   ../../ios/rendering/metal/INKMetalRenderer.swift:texture_mapping_modes
// )

// LINT.IfChange(blend_modes)
const BLEND_MODE_MODULATE: u32 = 0u;
const BLEND_MODE_DST_IN: u32 = 1u;
const BLEND_MODE_DST_OUT: u32 = 2u;
const BLEND_MODE_SRC_ATOP: u32 = 3u;
const BLEND_MODE_SRC_IN: u32 = 4u;
const BLEND_MODE_SRC_OVER: u32 = 5u;
const BLEND_MODE_DST_OVER: u32 = 6u;
const BLEND_MODE_SRC: u32 = 7u;
const BLEND_MODE_DST: u32 = 8u;
const BLEND_MODE_SRC_OUT: u32 = 9u;
const BLEND_MODE_DST_ATOP: u32 = 10u;
const BLEND_MODE_XOR: u32 = 11u;

fn blend(src: vec4<f32>, dst: vec4<f32>, mode: u32) -> vec4<f32> {
  switch (mode) {
    case BLEND_MODE_MODULATE: {
      return src * dst;
    }
    case BLEND_MODE_DST_IN: {
      return dst * src.a;
    }
    case BLEND_MODE_DST_OUT: {
      return dst * (1.0 - src.a);
    }
    case BLEND_MODE_SRC_ATOP: {
      return src * dst.a + dst * (1.0 - src.a);
    }
    case BLEND_MODE_SRC_IN: {
      return src * dst.a;
    }
    case BLEND_MODE_SRC_OVER: {
      return src + dst * (1.0 - src.a);
    }
    case BLEND_MODE_DST_OVER: {
      return dst + src * (1.0 - dst.a);
    }
    case BLEND_MODE_SRC: {
      return src;
    }
    case BLEND_MODE_DST: {
      return dst;
    }
    case BLEND_MODE_SRC_OUT: {
      return src * (1.0 - dst.a);
    }
    case BLEND_MODE_DST_ATOP: {
      return dst * src.a + src * (1.0 - dst.a);
    }
    case BLEND_MODE_XOR: {
      return src * (1.0 - dst.a) + dst * (1.0 - src.a);
    }
    default: {
      return src;
    }
  }
}
// LINT.ThenChange(../../ios/brush/INKTextureLayer.h:blend_modes)

// LINT.IfChange(apply_opacity_shift)
fn applyOpacityShift(opacityShift: f32, baseOpacity: f32) -> f32 {
  return saturate((opacityShift + 1.0) * baseOpacity);
}
// LINT.ThenChange(../../rendering/skia/common_internal/sksl_vertex_shader_helper_functions.h:apply_opacity_shift)

// LINT.IfChange(unpack_float2_packed_into_ubyte3)
fn unpackFloat2PackedIntoUByte3(unpackingTransform: vec4<f32>,
                                packedValue0To255: vec3<f32>) -> vec2<f32> {
  let mixedXY = packedValue0To255.y / 16.0;
  let unpacked = vec2<f32>(
    16.0 * packedValue0To255.x + floor(mixedXY),
    4096.0 * fract(mixedXY) + packedValue0To255.z
  );
  return unpackingTransform.yw * unpacked + unpackingTransform.xz;
}
// LINT.ThenChange(../../rendering/skia/common_internal/sksl_vertex_shader_helper_functions.h:unpack_float2_packed_into_ubyte3)

// LINT.IfChange(surface_uv_unpacking)
fn unpackSurfaceUv(packedValue0To255: vec3<f32>) -> vec2<f32> {
  return vec2<f32>(
    (16.0 * packedValue0To255.x + floor(packedValue0To255.y / 16.0)),
    (4096.0 * fract(packedValue0To255.y / 16.0) + packedValue0To255.z)
  ) / 4095.0;
}
// LINT.ThenChange(../../rendering/skia/common_internal/sksl_vertex_shader_helper_functions.h:surface_uv_unpacking)

// LINT.IfChange(position_and_opacity_unpacking)
fn unpackPositionAndOpacityShift(unpackingTransform: vec4<f32>,
                                 packedValue0To255: vec4<f32>) -> vec3<f32> {
  return vec3<f32>(
    unpackFloat2PackedIntoUByte3(unpackingTransform, packedValue0To255.xyz),
    packedValue0To255.w / 127.0 - 1.0
  );
}
// LINT.ThenChange(../../rendering/skia/common_internal/sksl_vertex_shader_helper_functions.h:position_and_opacity_unpacking)

// LINT.IfChange(derivative_and_label_unpacking)
fn unpackDerivativeAndLabel(unpackingTransform: vec4<f32>,
                            packedValue0To255: vec4<f32>) -> vec3<f32> {
  return vec3<f32>(
      unpackFloat2PackedIntoUByte3(unpackingTransform, packedValue0To255.xyz),
      packedValue0To255.w - 128.0
  );
}
// LINT.ThenChange(../../rendering/skia/common_internal/sksl_vertex_shader_helper_functions.h:label_packing)

// LINT.IfChange(hsl_shift_unpacking)
fn unpackHSLColorShift(packedValue0To255: vec4<f32>) -> vec3<f32> {
  return vec3<f32>(
    4.0 * packedValue0To255.x + floor(packedValue0To255.y / 64.0),
    1024.0 * fract(packedValue0To255.y / 64.0) + floor(packedValue0To255.z / 16.0),
    1024.0 * fract(packedValue0To255.z / 16.0) + packedValue0To255.w / 4.0
  ) / 511.0 - vec3<f32>(1.0);
}
// LINT.ThenChange(../../rendering/skia/common_internal/sksl_vertex_shader_helper_functions.h:hsl_shift_unpacking)

// LINT.IfChange(apply_hsl_and_opacity_shift)
fn applyHSLAndOpacityShift(hslShift: vec3<f32>,
                           opacityShift: f32,
                           colorUnpremul: vec4<f32>) -> vec4<f32> {
  var rgb = colorUnpremul.rgb;

  var y = dot(rgb, vec3<f32>(0.299,  0.587,  0.114));
  var i = dot(rgb, vec3<f32>(0.596, -0.275, -0.321));
  var q = dot(rgb, vec3<f32>(0.212, -0.523,  0.311));

  var hueRadians = 0.0;
  if (any(colorUnpremul.rgb != vec3<f32>(0.0, 0.0, 0.0))) {
    hueRadians = atan2(q, i);
  }

  let chroma = sqrt(i * i + q * q);

  hueRadians -= hslShift.x * 6.283185307179586;
  let newChroma = chroma * (hslShift.y + 1.0);
  let newY = y + hslShift.z;
  
  let newI = newChroma * cos(hueRadians);
  let newQ = newChroma * sin(hueRadians);

  let yiq = vec3<f32>(newY, newI, newQ);
  rgb = vec3<f32>(
    dot(yiq, vec3<f32>(1.0,  0.956,  0.621)),
    dot(yiq, vec3<f32>(1.0, -0.272, -0.647)),
    dot(yiq, vec3<f32>(1.0, -1.107,  1.704))
  );
  return vec4<f32>(rgb, applyOpacityShift(opacityShift, colorUnpremul.a));
}
// LINT.ThenChange(../../rendering/skia/common_internal/sksl_vertex_shader_helper_functions.h:apply_hsl_and_opacity_shift)

// LINT.IfChange(decode_margins)
fn decodeMargins(labels: vec2<f32>) -> vec2<f32> {
  return (4.0 / 126.0) * max(abs(labels) - vec2<f32>(1.0), vec2<f32>(0.0));
}
// LINT.ThenChange(../../rendering/skia/common_internal/sksl_vertex_shader_helper_functions.h:margin_encoding)

// LINT.IfChange(orthogonal)
fn orthogonal(v: vec2<f32>) -> vec2<f32> {
  return vec2<f32>(-v.y, v.x);
}
// LINT.ThenChange(../../rendering/skia/common_internal/sksl_vertex_shader_helper_functions.h:orthogonal)

// LINT.IfChange(calculate_antialiasing_and_position_outset)
fn calculateAntialiasingAndPositionOutset(
    sideDerivativeAndLabel: vec3<f32>,
    forwardDerivativeAndLabel: vec3<f32>,
    objectToCanvasLinearComponent: mat2x2<f32>,
    pixelsPerDimension: ptr<function, vec2<f32>>,
    normalizedToEdgeLRFB: ptr<function, vec4<f32>>,
    outsetPixelsLRFB: ptr<function, vec4<f32>>) -> vec2<f32> {
  let sideDerivative = sideDerivativeAndLabel.xy;
  let forwardDerivative = forwardDerivativeAndLabel.xy;

  *pixelsPerDimension =
    abs(determinant(objectToCanvasLinearComponent)) *
    vec2<f32>(dot(sideDerivative, sideDerivative),
              dot(forwardDerivative, forwardDerivative)) /
      max(vec2<f32>(0.000001), vec2<f32>(length(objectToCanvasLinearComponent *
                                                orthogonal(sideDerivative)),
                                         length(objectToCanvasLinearComponent *
                                                orthogonal(forwardDerivative))));

  let labels = vec2<f32>(sideDerivativeAndLabel.z,
                         forwardDerivativeAndLabel.z);

  *normalizedToEdgeLRFB = vec4<f32>(f32(labels.x > -0.005),
                                    f32(labels.x < 0.005),
                                    f32(labels.y > -0.005),
                                    f32(labels.y < 0.005));

  let pixelOutsetTarget =
    targetAntialiasingPixelOutset((*pixelsPerDimension).x);
  let outsetTargets = vec2<f32>(pixelOutsetTarget) / (*pixelsPerDimension);
  var outsets = min(outsetTargets, decodeMargins(labels));
  outsets.x = mix(outsetTargets.x, outsets.x,
                  saturate(4.0 * (*pixelsPerDimension).x - 1.0));
  let fromEdge = vec4<f32>(1.0) - *normalizedToEdgeLRFB;
  *outsetPixelsLRFB =
    pixelOutsetTarget * fromEdge * (outsets / outsetTargets).xxyy;

  let sideOutset = sign(labels.x) * outsets.x * sideDerivative;
  let forwardOutset = sign(labels.y) * outsets.y * forwardDerivative;
  let commonForwardMagnitude =
    saturate(dot(sideOutset, forwardOutset) /
             max(0.000001, dot(forwardOutset, forwardOutset)));

  return sideOutset + (1.0 - commonForwardMagnitude) * forwardOutset;
}
// LINT.ThenChange(../../rendering/skia/common_internal/sksl_vertex_shader_helper_functions.h:calculate_antialiasing_and_position_outset)

// LINT.IfChange(target_antialiasing_pixel_outset)
fn targetAntialiasingPixelOutset(widthInPixels: f32) -> f32 {
  return mix(0.5, 0.707107, saturate(2.0 * (widthInPixels - 0.5)));
}

// Duplicate of the above function, but with a different name, so that we can
// use it in the fragment shader without causing a redefinition error.
fn targetAntialiasingPixelOutsetFrag(widthInPixels: f32) -> f32 {
  return mix(0.5, 0.707107, saturate(2.0 * (widthInPixels - 0.5)));
}
// LINT.ThenChange(../../rendering/skia/common_internal/sksl_common_shader_helper_functions.h:target_antialiasing_pixel_outset_frag)

// LINT.IfChange(simulated_pixel_coverage)
fn simulatedPixelCoverage(pixelsPerDimension: vec2<f32>,
                          normalizedToEdgeLRFB: vec4<f32>,
                          outsetPixelsLRFB: vec4<f32>) -> f32 {
  let targetOutset = targetAntialiasingPixelOutsetFrag(pixelsPerDimension.x);
  let fromEdge = vec4<f32>(1.0) - normalizedToEdgeLRFB;
  let outset = min(outsetPixelsLRFB / max(fromEdge, vec4<f32>(0.000001)), vec4<f32>(targetOutset));
  let adjustedPixels = pixelsPerDimension + outset.xz + outset.yw;
  let pixelsToEdge = saturate((adjustedPixels.xxyy * normalizedToEdgeLRFB) / (2.0 * targetOutset));
  let isInterior = step(vec2<f32>(1.9999), normalizedToEdgeLRFB.xz + normalizedToEdgeLRFB.yw);
  let coverage = mix(
    max(
      pixelsToEdge.xz + pixelsToEdge.yw - vec2<f32>(1.0),
      vec2<f32>(0.0)
    ),
    vec2<f32>(1.0),
    isInterior
  );
  return coverage.x * coverage.y;
}
// LINT.ThenChange(../../rendering/skia/common_internal/sksl_fragment_shader_helper_functions.h:simulated_pixel_coverage)

// Returns a vec4<f32> that represents each of the 4 bytes in the input f32, as
// values in [0, 255].
//
// Although Skia automatically converts a ubyte4 vertex attribute to a half4
// with values in [0, 1], and a goal of these WGSL shaders is to match the
// behavior of the Skia shaders, the Skia shaders need to convert the [0, 1]
// value back to [0, 255] before doing further math. We skip that step here as
// we are able to get the values in [0, 255] more directly, which saves some
// computation and the potential for floating point inaccuracies.
//
// While there is an f16 type in WGSL, it's not supported on all GPUs, which is
// why we don't return that even though it would have sufficient precision to
// represent the values.
fn ubyte4AsF32To4xF32In0To255(ubyte4AsF32: f32) -> vec4<f32> {
  let ubyte4AsU32 = bitcast<u32>(ubyte4AsF32);
  let ubyte4As4xU8 = unpack4xU8(ubyte4AsU32);
  return vec4<f32>(ubyte4As4xU8);
}

@vertex
fn vertexMain(@builtin(vertex_index) vertexID: u32) -> VertexOut {
  var out: VertexOut;
  let isPackedVertexData = uniforms.vertexStride > 0;
  let aaEnabled =
      uniforms.sideDerivativeAndLabelOffset >= 0 &&
      uniforms.forwardDerivativeAndLabelOffset >= 0;
  let surfaceUvEnabled = uniforms.surfaceUvAndAnimationOffsetOffset >= 0;
  let hslShiftEnabled = uniforms.hslShiftOffset >= 0;
  let stride = u32(abs(uniforms.vertexStride));
  let floatOffset = (vertexID * stride) / 4u;
  let positionAndOpacityShiftFloatOffset = uniforms.positionAndOpacityShiftOffset / 4u;

  var sideDerivativeAndLabelFloatOffset = 0xffffffffu;
  var forwardDerivativeAndLabelFloatOffset = 0xffffffffu;
  if (aaEnabled) {
    sideDerivativeAndLabelFloatOffset = u32(uniforms.sideDerivativeAndLabelOffset) / 4u;
    forwardDerivativeAndLabelFloatOffset = u32(uniforms.forwardDerivativeAndLabelOffset) / 4u;
  }

  var surfaceUvFloatOffset = 0xffffffffu;
  if (surfaceUvEnabled) {
    surfaceUvFloatOffset = u32(uniforms.surfaceUvAndAnimationOffsetOffset) / 4u;
  }

  var hslShiftFloatOffset = 0xffffffffu;
  if (hslShiftEnabled) {
    hslShiftFloatOffset = u32(uniforms.hslShiftOffset) / 4u;
  }

  var pos = vec2<f32>(0.0, 0.0);
  var opacityShift = 0.0;
  var sideDerivativeAndLabel = vec3<f32>(0.0, 0.0, 0.0);
  var forwardDerivativeAndLabel = vec3<f32>(0.0, 0.0, 0.0);
  var hslShift = vec3<f32>(0.0, 0.0, 0.0);
  var surfaceUV = vec2<f32>(0.0, 0.0);

  if (isPackedVertexData) {
    let positionAndOpacityShift = unpackPositionAndOpacityShift(
      uniforms.positionUnpackingTransform,
      ubyte4AsF32To4xF32In0To255(vertices[floatOffset + positionAndOpacityShiftFloatOffset]));
    pos = positionAndOpacityShift.xy;
    opacityShift = positionAndOpacityShift.z;

    if (aaEnabled) {
      sideDerivativeAndLabel = unpackDerivativeAndLabel(
        uniforms.sideDerivativeUnpackingTransform,
        ubyte4AsF32To4xF32In0To255(vertices[floatOffset + sideDerivativeAndLabelFloatOffset]));
      forwardDerivativeAndLabel = unpackDerivativeAndLabel(
        uniforms.forwardDerivativeUnpackingTransform,
        ubyte4AsF32To4xF32In0To255(vertices[floatOffset + forwardDerivativeAndLabelFloatOffset]));
    }

    if (hslShiftEnabled) {
      hslShift = unpackHSLColorShift(
        ubyte4AsF32To4xF32In0To255(vertices[floatOffset + hslShiftFloatOffset]));
    }
    if (surfaceUvEnabled) {
      surfaceUV = unpackSurfaceUv(
        ubyte4AsF32To4xF32In0To255(vertices[floatOffset + surfaceUvFloatOffset]).xyz
      );
    }
  } else {
    pos = vec2<f32>(
      vertices[floatOffset + positionAndOpacityShiftFloatOffset],
      vertices[floatOffset + positionAndOpacityShiftFloatOffset + 1u]
    );
    opacityShift = vertices[floatOffset + positionAndOpacityShiftFloatOffset + 2u];

    if (aaEnabled) {
      sideDerivativeAndLabel = vec3<f32>(
        vertices[floatOffset + sideDerivativeAndLabelFloatOffset],
        vertices[floatOffset + sideDerivativeAndLabelFloatOffset + 1u],
        vertices[floatOffset + sideDerivativeAndLabelFloatOffset + 2u]);
      forwardDerivativeAndLabel = vec3<f32>(
        vertices[floatOffset + forwardDerivativeAndLabelFloatOffset],
        vertices[floatOffset + forwardDerivativeAndLabelFloatOffset + 1u],
        vertices[floatOffset + forwardDerivativeAndLabelFloatOffset + 2u]);
    }
    if (surfaceUvEnabled) {
      surfaceUV = vec2<f32>(
        vertices[floatOffset + surfaceUvFloatOffset],
        vertices[floatOffset + surfaceUvFloatOffset + 1u]
      );
    }

    if (hslShiftEnabled) {
      hslShift = vec3<f32>(
        vertices[floatOffset + hslShiftFloatOffset],
        vertices[floatOffset + hslShiftFloatOffset + 1u],
        vertices[floatOffset + hslShiftFloatOffset + 2u]);
    }
  }

  var pixelsPerDimension = vec2<f32>(0.0, 0.0);
  var normalizedToEdgeLRFB = vec4<f32>(0.0, 0.0, 0.0, 0.0);
  var outsetPixelsLRFB = vec4<f32>(0.0, 0.0, 0.0, 0.0);

  let objectToCanvasTransform = uniforms.viewTransform * uniforms.modelTransform;

  if (aaEnabled) {
    let objectToCanvasLinearComponent = mat2x2<f32>(
      objectToCanvasTransform[0].xy,
      objectToCanvasTransform[1].xy
    );

    pos += calculateAntialiasingAndPositionOutset(
      sideDerivativeAndLabel,
      forwardDerivativeAndLabel,
      objectToCanvasLinearComponent,
      &pixelsPerDimension,
      &normalizedToEdgeLRFB,
      &outsetPixelsLRFB
    );
  }

  out.aaPixelsPerDimension = pixelsPerDimension;
  out.unusedPadding = vec2<f32>(0.0, 0.0);
  out.aaNormalizedToEdgeLRFB = normalizedToEdgeLRFB;
  out.aaOutsetPixelsLRFB = outsetPixelsLRFB;

  out.positionInStrokeCoords = pos;
  out.surfaceUV = surfaceUV;
  out.position = uniforms.projectionTransform * objectToCanvasTransform *
    vec4<f32>(pos, 0.0, 1.0);

  let shiftedColorAndAlpha = applyHSLAndOpacityShift(hslShift, opacityShift, uniforms.color);
  out.color = vec4<f32>(shiftedColorAndAlpha.rgb * shiftedColorAndAlpha.a, shiftedColorAndAlpha.a);

  return out;
}

@fragment
fn fragmentMain(in: VertexOut) -> @location(0) vec4<f32> {
  let aaEnabled =
      uniforms.sideDerivativeAndLabelOffset >= 0 &&
      uniforms.forwardDerivativeAndLabelOffset >= 0;

  var color = in.color;
  if (aaEnabled) {
    color *= simulatedPixelCoverage(in.aaPixelsPerDimension,
                                    in.aaNormalizedToEdgeLRFB,
                                    in.aaOutsetPixelsLRFB);
  }

  if (uniforms.textureLayerCount == 0u) {
    return color;
  }

  var blendedTexColor: vec4<f32>;
  for (var layerIndex = 0u; layerIndex < uniforms.textureLayerCount; layerIndex++) {
    let layer = uniforms.layers[layerIndex];
    var normalizedTextureCoords: vec2<f32>;
    switch(layer.mappingMode) {
      case TEXTURE_MAPPING_MODE_TILING: {
        var texturePositionStrokeCoordsOffset: vec2<f32>;
        switch (layer.tilingOrigin) {
          case TEXTURE_ORIGIN_STROKE_SPACE_ORIGIN: {
            texturePositionStrokeCoordsOffset = vec2<f32>(0.0, 0.0);
          }
          case TEXTURE_ORIGIN_FIRST_STROKE_INPUT: {
            texturePositionStrokeCoordsOffset = -uniforms.firstInputPos;
          }
          case TEXTURE_ORIGIN_LAST_STROKE_INPUT: {
            texturePositionStrokeCoordsOffset = -uniforms.lastInputPos;
          }
          default: {
            texturePositionStrokeCoordsOffset = vec2<f32>(0.0, 0.0);
          }
        }
        var sizeUnitScale: f32;
        switch (layer.tilingSizeUnit) {
          case TEXTURE_SIZE_UNIT_BRUSH_SIZE: {
            sizeUnitScale = uniforms.brushSize;
          }
          case TEXTURE_SIZE_UNIT_STROKE_COORDINATES: {
            sizeUnitScale = 1.0;
          }
          default: {
            sizeUnitScale = 1.0;
          }
        }
        let scale = layer.tilingSize * sizeUnitScale;
        let s = sin(layer.tilingRotation);
        let c = cos(layer.tilingRotation);
        let rotationMat = mat2x2(c, -s, s, c);
        normalizedTextureCoords = layer.tilingOffset +
          rotationMat *
            (in.positionInStrokeCoords + texturePositionStrokeCoordsOffset) /
            scale;
      }
      case TEXTURE_MAPPING_MODE_STAMPING: {
        normalizedTextureCoords = in.surfaceUV;
      }
      default: {
        normalizedTextureCoords = vec2<f32>(0.0, 0.0);
      }
    }

    var texColor: vec4<f32>;
    switch (layerIndex) {
      case 0u: {
        texColor = textureSample(texture0, sampler0, normalizedTextureCoords);
      }
      case 1u: {
        texColor = textureSample(texture1, sampler1, normalizedTextureCoords);
      }
      case 2u: {
        texColor = textureSample(texture2, sampler2, normalizedTextureCoords);
      }
      case 3u: {
        texColor = textureSample(texture3, sampler3, normalizedTextureCoords);
      }
      default: {
        texColor = vec4<f32>(0.0, 0.0, 0.0, 0.0);
      }
    }

    if (layerIndex == 0u) {
      blendedTexColor = texColor;
    } else {
      blendedTexColor = blend(
        blendedTexColor,
        texColor,
        uniforms.layers[layerIndex - 1u].blendMode
      );
    }
  }

  return blend(
    blendedTexColor,
    color,
    uniforms.layers[uniforms.textureLayerCount - 1u].blendMode
  );
}
