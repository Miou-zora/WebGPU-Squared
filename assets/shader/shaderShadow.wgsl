struct Uniform {
  modelMatrix : mat4x4f,
  normalModelMatrix : mat4x4f,
}

struct ShadowData {
  lightViewProj : mat4x4f,
};

@group(0) @binding(0) var<storage, read> uniforms : array<Uniform>;
@group(1) @binding(0) var<uniform> shadowData : ShadowData;

@vertex
fn vs_main(
  @location(0) position: vec3f,
  @location(1) normal: vec3f,
  @location(2) uv: vec2f,
  @location(3) uniformIndex: u32
) -> @builtin(position) vec4f {
    return shadowData.lightViewProj * uniforms[uniformIndex].modelMatrix * vec4f(position, 1.0);
}

@fragment
fn fs_main() {

}
