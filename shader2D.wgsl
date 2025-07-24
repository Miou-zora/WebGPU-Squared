struct VertexInput {
    @location(0) position: vec3f,
};

struct VertexOutput {
    @builtin(position) position: vec4f,
};

struct Uniforms {
    orthoMatrix: mat4x4f,
};

@group(0) @binding(0)
var<uniform> uniforms: Uniforms;

@vertex
fn vs_main(in: VertexInput, @builtin(vertex_index) in_vertex_index: u32) -> VertexOutput {
    var out: VertexOutput;
    out.position = vec4f(in.position, 1.0) * uniforms.orthoMatrix;
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    return vec4f(0.0, 0.4, 0.7, 1.0);
}
