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

/**
 * Identify a cubemap sample location by a layer/face and a normalized UV
 * coordinate within this face.
 */
struct CubeMapUVL {
    uv: vec2<f32>,
    layer: u32,
}

/**
 * Utility function for textureGatherWeights
 */
fn cubeMapUVLFromDirection(direction: vec3<f32>) -> CubeMapUVL {
    let abs_direction = abs(direction);
    var major_axis_idx = 0u;
    //  Using the notations of https://www.khronos.org/registry/OpenGL/specs/gl/glspec46.core.pdf page 253
    // as suggested here: https://stackoverflow.com/questions/55558241/opengl-cubemap-face-order-sampling-issue
    var ma = 0.0;
    var sc = 0.0;
    var tc = 0.0;
    if (abs_direction.x > abs_direction.y && abs_direction.x > abs_direction.z) {
        major_axis_idx = 0u;
        ma = direction.x;
        if (ma >= 0) {
            sc = -direction.z;
        } else {
            sc = direction.z;
        }
        tc = -direction.y;
    } else if (abs_direction.y > abs_direction.x && abs_direction.y > abs_direction.z) {
        major_axis_idx = 1u;
        ma = direction.y;
        sc = direction.x;
        if (ma >= 0) {
            tc = direction.z;
        } else {
            tc = -direction.z;
        }
    } else {
        major_axis_idx = 2u;
        ma = direction.z;
        if (ma >= 0) {
            sc = -direction.x;
        } else {
            sc = direction.x;
        }
        tc = -direction.y;
    }
    var sign_offset = 0u;
    if (ma < 0) {
        sign_offset = 1u;
    }
    let s = 0.5 * (sc / abs(ma) + 1.0);
    let t = 0.5 * (tc / abs(ma) + 1.0);
    return CubeMapUVL(
        vec2<f32>(s, t),
        2 * major_axis_idx + sign_offset,
    );
}

/**
 * Inverse of cubeMapUVLFromDirection
 * This returns an unnormalized direction, to save some calculation
 */
fn directionFromCubeMapUVL(uvl: CubeMapUVL) -> vec3<f32> {
    let s = uvl.uv.x;
    let t = uvl.uv.y;
    let abs_ma = 1.0;
    let sc = 2.0 * s - 1.0;
    let tc = 2.0 * t - 1.0;
    var direction = vec3<f32>(0.0);
    switch (uvl.layer) {
        case 0u {
            return vec3<f32>(1.0, -tc, -sc);
        }
        case 1u {
            return vec3<f32>(-1.0, -tc, sc);
        }
        case 2u {
            return vec3<f32>(sc, 1.0, tc);
        }
        case 3u {
            return vec3<f32>(sc, -1.0, -tc);
        }
        case 4u {
            return vec3<f32>(sc, -tc, 1.0);
        }
        case 5u {
            return vec3<f32>(-sc, -tc, -1.0);
        }
        default {
            return vec3<f32>(0.0); // should not happen
        }
    }
}

/**
 * Compute the u/v weights corresponding to the bilinear mix of samples
 * returned by textureGather.
 * To be included in some standard helper library.
 */
fn textureGatherWeights(t: texture_cube<f32>, direction: vec3<f32>) -> vec2<f32> {
    // major axis direction
    let cubemap_uvl = cubeMapUVLFromDirection(direction);
    let dim = textureDimensions(t).xy;
    let scaled_uv = cubemap_uvl.uv * vec2<f32>(dim);
    // This is not accurate, see see https://www.reedbeta.com/blog/texture-gathers-and-coordinate-precision/
    // but bottom line is:
    //   "Unfortunately, if we need this to work, there seems to be no option but to check
    //    which hardware you are running on and apply the offset or not accordingly."
    return fract(scaled_uv - 0.5);
}

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

    //let direction = CUBE_FACE_TRANSFORM[layer] * vec3<f32>(uv, 1.0);
    let direction = directionFromCubeMapUVL(CubeMapUVL(uv, layer));

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

    let w = textureGatherWeights(inputCubemapTexture, direction);
    // TODO: could be represented as a matrix/vector product
    let color = vec4<f32>(
        mix(mix(samples[0].x, samples[0].y, w.x), mix(samples[0].w, samples[0].z, w.x), w.y),
        mix(mix(samples[1].x, samples[1].y, w.x), mix(samples[1].w, samples[1].z, w.x), w.y),
        mix(mix(samples[2].x, samples[2].y, w.x), mix(samples[2].w, samples[2].z, w.x), w.y),
        mix(mix(samples[3].x, samples[3].y, w.x), mix(samples[3].w, samples[3].z, w.x), w.y),
    );
    textureStore(outputEquirectangularTexture, id.xy, color);
}
