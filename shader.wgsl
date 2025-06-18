struct VertexInput {
    @location(0) position: vec3f,
    @location(1) color: vec3f,
};

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) color: vec3f,
};

struct MyUniforms {
    color: vec4f,
    time: f32,
};

// Anywhere in the global scope (e.g. just before defining vs_main)
const pi = 3.14159265359;

@group(0) @binding(0)
var<uniform> uMyUniform: MyUniforms;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    let ratio = 800.0 / 800.0;
    var offset = vec2f(0.0, 0.0);
	let angle = uMyUniform.time; // you can multiply it go rotate faster
	let alpha = cos(angle);
	let beta = sin(angle);
	// Rotate the model in the XY plane
	let angle1 = uMyUniform.time;
	let c1 = cos(angle1);
	let s1 = sin(angle1);
	let R1 = transpose(mat4x4f(
		c1,  s1, 0.0, 0.0,
		-s1,  c1, 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0,
		0.0, 0.0, 0.0, 1.0,
	));

	let S = transpose(mat4x4f(
    0.3,  0.0, 0.0, 0.0,
    0.0,  0.3, 0.0, 0.0,
    0.0,  0.0, 0.3, 0.0,
    0.0,  0.0, 0.0, 1.0,
	));

	// Translate the object
	let T = transpose(mat4x4f(
		1.0,  0.0, 0.0, 0.5,
		0.0,  1.0, 0.0, 0.0,
		0.0,  0.0, 1.0, 0.0,
		0.0,  0.0, 0.0, 1.0,
	));

	// Tilt the view point in the YZ plane
	// by three 8th of turn (1 turn = 2 pi)
	let angle2 = 3.0 * pi / 4.0;
	let c2 = cos(angle2);
	let s2 = sin(angle2);
	let R2 = transpose(mat4x4f(
		1.0, 0.0, 0.0, 0.0,
		0.0,  c2,  s2, 0.0,
		0.0, -s2,  c2, 0.0,
		0.0, 0.0, 0.0, 1.0,
	));

    var position = (R2 * R1 * T * S * vec4f(in.position, 1.0)).xyz;

    let focalPoint = vec3f(0.0, 0.0, -2.0);
    position = position - focalPoint;

    let focalLength = 2.0;

    let far = 100.0;
    let near = 0.01;

    let divides = 1.0 / (focalLength * (far - near));
    let P = transpose(mat4x4f(
        1.0,  0.0,        0.0,                  0.0,
        0.0, ratio,       0.0,                  0.0,
        0.0,  0.0,    far * divides,   -far * near * divides,
        0.0,  0.0,  1.0 / focalLength,          0.0,
    ));

	let homogeneous_position = vec4f(position, 1.0);
    out.position = P * homogeneous_position;

    out.color = in.color;
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    return vec4f(in.color, 1.0) * uMyUniform.color;
}
