struct VertexInput {
    @location(0) position: vec3f,
    @location(1) normal: vec3f,
};

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) normal: vec3f,
    @location(1) viewDirection: vec3f,
};

struct MyUniforms {
    projectionMatrix: mat4x4f,
    viewMatrix: mat4x4f,
    modelMatrix: mat4x4f,
    color: vec4f,
    cameraPosition: vec3f,
    time: f32,
};

struct Light {
    color: vec4f,
    direction: vec3f,
    intensity: f32,
    enabled: u32,
};

// Anywhere in the global scope (e.g. just before defining vs_main)
const pi = 3.14159265359;

@group(0) @binding(0)
var<uniform> uMyUniform: MyUniforms;

@group(0) @binding(1)
var<storage, read> uLights: array<Light>;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    let worldPosition = uMyUniform.modelMatrix * vec4f(in.position, 1.0);
    out.position = uMyUniform.projectionMatrix * uMyUniform.viewMatrix * worldPosition;
    out.normal = (uMyUniform.modelMatrix * vec4f(in.normal, 0.0)).xyz;
    out.viewDirection = normalize(uMyUniform.cameraPosition - worldPosition.xyz);
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    var finalColor: vec3f = vec3f(0.0, 0.0, 0.0);
    var ambiantColor: vec3f = vec3f(0.0, 0.8, 0.8);
    let V = normalize(in.viewDirection);
    let N = normalize(in.normal);

    let MatKd: vec3f = uMyUniform.color.rgb;
    let MatKs: vec3f = vec3f(0.4, 0.4, 0.4);
    let Shiness: f32 = 400.0;

	var color = vec3f(0.0);
    for (var i = 0u; i < arrayLength(&uLights); i++) {
        let light = uLights[i];

        if (light.enabled == 0u) {
            continue; // Skip disabled lights
        }

		let L = normalize(light.direction);
		let R = reflect(-L, N); // equivalent to 2.0 * dot(N, L) * N - L

		let diffuse = max(0.0, dot(L, N)) * light.color.rgb * light.intensity;
        // let diffuse = light.color.rgb;

		// We clamp the dot product to 0 when it is negative
		let RoV = max(0.0, dot(R, V));
		let specular = pow(RoV, Shiness);

		color += MatKd * diffuse + MatKs * specular;
    }
    let corrected_color = pow(color, vec3f(2.2));
	return vec4f(corrected_color, uMyUniform.color.a);
}
