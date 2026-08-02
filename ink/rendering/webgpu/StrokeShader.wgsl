struct VertexOut {
  @builtin(position) position: vec4<f32>,
  @location(0) color: vec4<f32>,
}

struct Uniforms {
  modelTransform: mat4x4<f32>,
  viewTransform: mat4x4<f32>,
  projectionTransform: mat4x4<f32>,
  color: vec4<f32>,
  vertexStride: u32,
}

@group(0) @binding(0) var<uniform> uniforms: Uniforms;
@group(0) @binding(1) var<storage, read> vertices: array<f32>;

@vertex
fn vertexMain(@builtin(vertex_index) vertexID: u32) -> VertexOut {
  var out: VertexOut;
  let floatOffset = (vertexID * uniforms.vertexStride) / 4u;
  let pos = vec2<f32>(vertices[floatOffset], vertices[floatOffset + 1u]);

  out.position = uniforms.projectionTransform * uniforms.viewTransform * uniforms.modelTransform * vec4<f32>(pos, 0.0, 1.0);
  out.color = uniforms.color;
  return out;
}

@fragment
fn fragmentMain(in: VertexOut) -> @location(0) vec4<f32> {
  return in.color;
}
