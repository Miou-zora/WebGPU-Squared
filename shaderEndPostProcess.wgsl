@vertex
fn vs_main(
  @builtin(vertex_index) VertexIndex : u32
) -> @builtin(position) vec4f {
  const pos = array(
    vec2(-1.0, -1.0), vec2(1.0, -1.0), vec2(-1.0, 1.0),
    vec2(-1.0, 1.0), vec2(1.0, -1.0), vec2(1.0, 1.0),
  );

  return vec4f(pos[VertexIndex], 0.9, 1.0);
}

@group(0) @binding(0) var inputTexture: texture_2d<f32>;

@fragment
fn fs_main(
  @builtin(position) coord : vec4f
) -> @location(0) vec4f {
  let color = textureLoad(
    inputTexture,
    vec2i(floor(coord.xy)),
    0
  ).rgb;

  return vec4(color, 1.0);
}
