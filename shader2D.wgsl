struct VertexInput {
    @location(0) position: vec3f,
};

struct VertexOutput {
    @builtin(position) position: vec4f,
};

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    out.position = vec4f(in.position, 1.0);
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    return vec4f(1.0, 0.0, 0.0, 1.0); // Red color
}
