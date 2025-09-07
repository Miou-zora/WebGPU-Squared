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

@group(0) @binding(0) var gBufferNormal: texture_2d<f32>;
@group(0) @binding(1) var gBufferAlbedo: texture_2d<f32>;
@group(0) @binding(2) var gBufferDepth: texture_2d<f32>;

struct Light {
  lightViewProjMatrix: mat4x4f,
  color: vec4f,
  direction: vec3f,
  intensity: f32,
  enabled: u32,
  light_type: u32,
  lightIndex: u32,
};

struct Lights {
    numberOfLights: u32,
    lights: array<Light>,
}

@group(1) @binding(0)
var<storage, read> uLights: Lights;

struct Camera {
  viewProjectionMatrix : mat4x4f,
  invViewProjectionMatrix : mat4x4f,
  position : vec3f,
}

@group(2) @binding(0) var<uniform> camera: Camera;

@group(3) @binding(0) var lightsDirectionalTextures: texture_depth_2d_array;
@group(3) @binding(1) var lightsDirectionalTextureSampler: sampler_comparison;

@group(4) @binding(0) var skybox: texture_2d<f32>;

const shadowDepthTextureSize: f32 = 2048.0;

fn world_from_screen_coord(coord : vec2f, depth_sample: f32) -> vec3f {
  // reconstruct world-space position from the screen coordinate.
  let posClip = vec4(coord.x * 2.0 - 1.0, (1.0 - coord.y) * 2.0 - 1.0, depth_sample, 1.0);
  let posWorldW = camera.invViewProjectionMatrix * posClip;
  let posWorld = posWorldW.xyz / posWorldW.www;
  return posWorld;
}

fn calculateDirectionalLight(light: Light, N: vec3f, V: vec3f, MatKd: vec3f, MatKs: vec3f, Shiness: f32, position: vec3f) -> vec3f
{
  let FragPosLightSpace = light.lightViewProjMatrix * vec4f(position, 1.0);
  let shadowCoord = FragPosLightSpace.xyz / FragPosLightSpace.w;
  let projCoord = shadowCoord * vec3f(0.5, -0.5, 1.0) + vec3f(0.5, 0.5, 0.0);

  var visibility = 0.0;
  let oneOverShadowDepthTextureSize = 1.0 / 2048.0;
  for (var y = -3; y <= 3; y++) {
    for (var x = -3; x <= 3; x++) {
      let offset = vec2f(vec2(x, y)) * oneOverShadowDepthTextureSize;

      visibility += textureSampleCompare(
        lightsDirectionalTextures, lightsDirectionalTextureSampler,
        projCoord.xy + offset, i32(light.lightIndex), projCoord.z - 0.007
      );
    }
  }
  visibility /= 27.0;
  if (visibility < 0.01) {
    return vec3f(0.0);
  }

  let L = normalize(-light.direction);
  let R = reflect(-L, N); // equivalent to 2.0 * dot(N, L) * N - L

  let diffuse = max(0.0, dot(L, N)) * light.color.rgb * light.intensity;
  // let diffuse = light.color.rgb;

  // We clamp the dot product to 0 when it is negative
  let RoV = max(0.0, dot(R, V));
  let specular = pow(RoV, Shiness) * light.color.rgb * light.intensity;

  return (MatKd * diffuse + MatKs * specular) * visibility;
}

@fragment
fn fs_main(
  @builtin(position) coord : vec4f
) -> @location(0) vec4f {
  var result : vec3f;

  let depth = textureLoad(
    gBufferDepth,
    vec2i(floor(coord.xy)),
    0
  ).x;

  let skyboxColor = textureLoad(
    skybox,
    vec2i(floor(coord.xy)),
    0
  ).rgb;

  if (depth >= 1.0) {
    return vec4(skyboxColor, 1.0);
  }

  let bufferSize = textureDimensions(gBufferDepth);
  let coordUV = coord.xy / vec2f(bufferSize);
  let position = world_from_screen_coord(coordUV, depth);

  let normal = textureLoad(
    gBufferNormal,
    vec2i(floor(coord.xy)),
    0
  ).xyz;

  let albedo = textureLoad(
    gBufferAlbedo,
    vec2i(floor(coord.xy)),
    0
  ).rgb;
  let MatKd = albedo;
  let MatKs = vec3f(0.4, 0.4, 0.4);
  let N = normalize(normal);
  let V = normalize(camera.position - position);
  let Shiness: f32 = 100.0;

	var color = vec3f(0.0);
  var visibility = 0.0;
  let oneOverShadowDepthTextureSize = 1.0 / shadowDepthTextureSize;
  for (var i = 0u; i < uLights.numberOfLights; i++) {
    let light = uLights.lights[i];

        if (light.enabled == 0u) {
            continue; // Skip disabled lights
        }

        if (light.light_type == 0u) { // Directional light
            color += calculateDirectionalLight(light, N, V, MatKd, MatKs, Shiness, position);
        } else if (light.light_type == 1u) { // Point light
            let lightDir = light.direction - position;
            let distance = length(lightDir);
            let L = normalize(lightDir);
            let HalfwayVector = normalize(V + L);

            let attenuation = 1.0 / (distance);

            let diffuse = MatKd * light.color.rgb * light.intensity * max(dot(L, N), 0.0) * attenuation;
            let specular = MatKs * light.color.rgb * light.intensity * pow(max(dot(HalfwayVector, N), 0.0), Shiness) * attenuation;
            color += diffuse + specular;
        }
  }

  return vec4(color, 1.0);
}
