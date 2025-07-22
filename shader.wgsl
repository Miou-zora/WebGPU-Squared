struct VertexInput {
    @location(0) position: vec3f,
    @location(1) normal: vec3f,
};

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) normal: vec3f,
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
    position: vec3f,
    color: vec3f,
    intensity: f32,
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
    out.position = uMyUniform.projectionMatrix * uMyUniform.viewMatrix * uMyUniform.modelMatrix * vec4f(in.position, 1.0);
    out.normal = in.normal;
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    var finalColor: vec3f = vec3f(0.0, 0.0, 0.0);
    var ambiantColor: vec3f = vec3f(0.0, 0.8, 0.8);

    let MatKd: vec3f = uMyUniform.color.rgb;
    let MatKs: vec3f = vec3f(0.4, 0.4, 0.4);
    let Shiness: f32 = 32.0;

    for (var i = 0u; i < arrayLength(&uLights); i++) {
        let light = uLights[i];
        let L = normalize(light.position - in.position.xyz);
        let V = normalize(uMyUniform.cameraPosition - in.position.xyz);
        let HalfwayVector = normalize(V + L);
        let diffuse = MatKd * light.color * max(dot(L, in.normal), 0.0);
        let specular = MatKs * light.color * pow(max(dot(HalfwayVector, in.normal), 0.0), Shiness) * light.intensity;
        finalColor += diffuse + specular;
    }
    return vec4f(finalColor.rgb + ambiantColor * vec3f(0.2, 0.2, 0.2), 1.0);
}
