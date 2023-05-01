const PI = 3.14159265359;

struct Uniforms {
    kernel: mat3x3<f32>,
    test: f32,
    filter_type: u32,
}

@group(0) @binding(0) var inputEquirectangularTexture: texture_2d<f32>;
@group(0) @binding(1) var inputCubemapTexture: texture_cube<f32>;
@group(0) @binding(2) var outputEquirectangularTexture: texture_storage_2d<rgba8unorm,write>;
@group(0) @binding(3) var outputCubemapTexture: texture_storage_2d_array<rgba8unorm,write>;
@group(0) @binding(4) var<uniform> uniforms: Uniforms;
@group(0) @binding(5) var textureSampler: sampler;

const CUBE_FACE_TRANSFORM = array<mat3x3<f32>,6>(
    mat3x3<f32>(
        0.0, -2.0, 0.0,
        0.0, 0.0, -2.0,
        1.0, 1.0, 1.0,
    ),
    mat3x3<f32>(
        0.0, 2.0, 0.0,
        0.0, 0.0, -2.0,
        -1.0, -1.0, 1.0,
    ),
    mat3x3<f32>(
        2.0, 0.0, 0.0,
        0.0, 0.0, -2.0,
        -1.0, 1.0, 1.0,
    ),
    mat3x3<f32>(
        -2.0, 0.0, 0.0,
        0.0, 0.0, -2.0,
        1.0, -1.0, 1.0,
    ),
    mat3x3<f32>(
        2.0, 0.0, 0.0,
        0.0, 2.0, 0.0,
        -1.0, -1.0, 1.0,
    ),
    mat3x3<f32>(
        2.0, 0.0, 0.0,
        0.0, -2.0, 0.0,
        -1.0, 1.0, -1.0,
    ),
);

@compute @workgroup_size(4, 4, 6)
fn computeCubeMap(@builtin(global_invocation_id) id: vec3<u32>) {
    let outputDimensions = textureDimensions(outputCubemapTexture).xy;
    let inputDimensions = textureDimensions(inputEquirectangularTexture);
    let layer = id.z;

    let uv = vec2<f32>(id.xy) / vec2<f32>(outputDimensions);

    let direction = CUBE_FACE_TRANSFORM[layer] * vec3<f32>(uv, 1.0);

    //let direction = vec3<f32>(1.0, 1.0 - 2.0 * uv.x, 1.0 - 2.0 * uv.y);

    let theta = acos(direction.z / length(direction));
    let phi = atan2(direction.y, direction.x);
    let latlong_uv = vec2<f32>(phi / (2.0 * PI) + 0.5, theta / PI);
    let color = textureLoad(inputEquirectangularTexture, vec2<u32>(latlong_uv * vec2<f32>(inputDimensions)), 0);
    textureStore(outputCubemapTexture, id.xy, layer, color);
}

@compute @workgroup_size(8, 8)
fn computeEquirectangular(@builtin(global_invocation_id) id: vec3<u32>) {
    let outputDimensions = textureDimensions(outputEquirectangularTexture).xy;

    let uv = vec2<f32>(id.xy) / vec2<f32>(outputDimensions);
    let phi = (uv.x - 0.5) * 2.0 * PI;
    let theta = uv.y * PI;
    let direction = vec3<f32>(
        cos(phi) * sin(theta),
        sin(phi) * sin(theta),
        cos(theta),
    );

    let samples = array<vec4<f32>, 4>(
        textureGather(0, inputCubemapTexture, textureSampler, direction),
        textureGather(1, inputCubemapTexture, textureSampler, direction),
        textureGather(2, inputCubemapTexture, textureSampler, direction),
        textureGather(3, inputCubemapTexture, textureSampler, direction),
    );

    // TODO: compute weights properly
    // see https://www.reedbeta.com/blog/texture-gathers-and-coordinate-precision/
    let u = 0.5;
    let v = 0.5;
    // TODO: could be represented as a matrix/vector product
    let color = vec4<f32>(
        mix(mix(samples[0].x, samples[0].y, u), mix(samples[0].w, samples[0].z, u), v),
        mix(mix(samples[1].x, samples[1].y, u), mix(samples[1].w, samples[1].z, u), v),
        mix(mix(samples[2].x, samples[2].y, u), mix(samples[2].w, samples[2].z, u), v),
        mix(mix(samples[3].x, samples[3].y, u), mix(samples[3].w, samples[3].z, u), v),
    );
    textureStore(outputEquirectangularTexture, id.xy, color);
}
