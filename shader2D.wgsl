struct VertexInput {
    @location(0) position: vec3f,
    @location(1) uv: vec2f,
};

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) uv: vec2f,
};

struct Uniforms {
    orthoMatrix: mat4x4f,
};

@group(0) @binding(0)
var<uniform> uniforms: Uniforms;

@group(1) @binding(0)
var gradientTexture: texture_2d<f32>;

@group(1) @binding(1)
var textureSampler: sampler;


@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    out.position = vec4f(in.position, 1.0) * uniforms.orthoMatrix;
    out.uv = in.uv;
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    let color = textureSample(gradientTexture, textureSampler, in.uv).rgb;

    return vec4f(color, 1.0);
}
