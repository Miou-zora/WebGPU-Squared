struct Uniforms {
    modelViewProjectionMatrix: mat4x4f,
};

@group(0) @binding(0) var<uniform> uniforms: Uniforms;
@group(0) @binding(1) var cubemapTexture: texture_cube<f32>;
@group(0) @binding(2) var cubemapSampler: sampler;

struct VertexOutput {
  @builtin(position) position: vec4f,
  @location(0) uv: vec2f,
  @location(1) fragPosition: vec3f,
}

@vertex
fn vs_main(
    @location(0) position: vec3f,
    @location(1) uv: vec2f,
) -> VertexOutput {
    var out: VertexOutput;
    out.position = uniforms.modelViewProjectionMatrix * vec4f(position, 1.0);
    out.uv = uv;
    out.fragPosition = 0.5 * (position + vec3(1.0, 1.0, 1.0));
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    var cubemapVec = in.fragPosition - vec3(0.5);
    cubemapVec.z *= -1;
    return textureSample(cubemapTexture, cubemapSampler, cubemapVec);

}
