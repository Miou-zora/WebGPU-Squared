struct Camera {
  viewProjectionMatrix : mat4x4f,
  invViewProjectionMatrix : mat4x4f,
  position : vec3f,
}
@group(0) @binding(0) var<uniform> camera : Camera;

struct VertexOutput {
  @builtin(position) Position : vec4f,
  @location(0) fragNormal: vec3f,    // normal in world space
  @location(1) fragUV: vec2f,
}

@group(1) @binding(0) var texture: texture_2d<f32>;
@group(1) @binding(1) var textureSampler: sampler;

struct Uniform {
  modelMatrix : mat4x4f,
  normalModelMatrix : mat4x4f,
}

@group(2) @binding(0) var<storage, read> uniforms : array<Uniform>;

@vertex
fn vs_main(
  @location(0) position: vec3f,
  @location(1) normal: vec3f,
  @location(2) uv: vec2f,
  @location(3) uniformIndex: u32
) -> VertexOutput {
  var output : VertexOutput;
  let worldPosition = (uniforms[uniformIndex].modelMatrix * vec4(position, 1.0)).xyz;
  output.Position = camera.viewProjectionMatrix * vec4(worldPosition, 1.0);
  output.fragNormal = normalize((uniforms[uniformIndex].normalModelMatrix * vec4(normal, 1.0)).xyz);
  output.fragUV = uv;
  return output;
}

struct GBufferOutput {
    @location(0) normal : vec4f,
    @location(1) albedo : vec4f,
}

@fragment
fn fs_main(
  @location(0) fragNormal: vec3f,
  @location(1) fragUV : vec2f
) -> GBufferOutput {
  var output : GBufferOutput;
  output.normal = vec4(normalize(fragNormal), 1.0);
  output.albedo = vec4(textureSample(texture, textureSampler, fragUV).rgb, 1.0);

  return output;
}
