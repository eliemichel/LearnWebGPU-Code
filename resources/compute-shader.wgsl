const PI = 3.14159265359;

struct Uniforms {
    kernel: mat3x3<f32>,
    test: f32,
    filter_type: u32,
}

@group(0) @binding(0) var inputTexture: texture_2d<f32>;
@group(0) @binding(1) var outputTexture: texture_storage_2d_array<rgba8unorm,write>;
@group(0) @binding(2) var<uniform> uniforms: Uniforms;

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
        0.0, -2.0, 0.0,
        0.0, 0.0, -2.0,
        1.0, 1.0, 1.0,
    ),
    mat3x3<f32>(
        0.0, -2.0, 0.0,
        0.0, 0.0, -2.0,
        1.0, 1.0, 1.0,
    ),
);

@compute @workgroup_size(4, 4, 6)
fn computeCubeMapFace(@builtin(global_invocation_id) id: vec3<u32>) {
    let outputDimensions = textureDimensions(outputTexture).xy;
    let inputDimensions = textureDimensions(inputTexture);
    let layer = id.z;

    let uv = vec2<f32>(id.xy) / vec2<f32>(outputDimensions);

    let direction = CUBE_FACE_TRANSFORM[layer] * vec3<f32>(uv, 1.0);

    //let direction = vec3<f32>(1.0, 1.0 - 2.0 * uv.x, 1.0 - 2.0 * uv.y);

    let theta = acos(direction.z / length(direction));
    let phi = atan2(direction.y, direction.x);
    let latlong_uv = vec2<f32>(phi / (2.0 * PI) + 0.5, theta / PI);
    // TODO: use sampler
    //let color = textureSampleLevel(inputTexture, textureSampler, latlong_uv, level).rgb;
    let color = textureLoad(inputTexture, vec2<u32>(latlong_uv * vec2<f32>(inputDimensions)), 0);
    textureStore(outputTexture, id.xy, layer, color);
}
