const PI = 3.14159265359;

/* **************** SHADING **************** */

/**
 * Standard properties of a material
 * (Describe all the properties of a surface needed to shade it)
 */
struct MaterialProperties {
    baseColor: vec3f,
    roughness: f32,
    metallic: f32,
    reflectance: f32,
    highQuality: u32, // bool, turn on costly extra visual effects
}

/**
 * Given some material properties, the BRDF (Bidirectional Reflection
 * Distribution Function) tells the probability that a photon coming from an
 * incoming direction (towards the light) is reflected in an outgoing direction
 * (towards the camera).
 * The returned value gives a different probability for each color channel.
 */
fn brdf(
    material: MaterialProperties,
    normal: vec3f, // assumed to be normalized
    incomingDirection: vec3f, // assumed to be normalized
    outgoingDirection: vec3f, // assumed to be normalized
) -> vec3f {
    // Switch to compact notations used in math formulas
    // (notations of https://google.github.io/filament/Filament.html)
    let L = incomingDirection;
    let V = outgoingDirection;
    let N = normal;
    let H = normalize(L + V);
    let alpha = material.roughness * material.roughness;

    let NoV = abs(dot(N, V)) + 1e-5;
    let NoL = clamp(dot(N, L), 0.0, 1.0);
    let NoH = clamp(dot(N, H), 0.0, 1.0);
    let LoH = clamp(dot(L, H), 0.0, 1.0);

    // == Specular (reflected) lobe ==
    // Contribution of the Normal Distribution Function (NDF)
    let D = D_GGX(NoH, alpha);
    // Self-shadowing
    let Vis = V_SmithGGXCorrelatedFast(NoV, NoL, alpha);
    // Fresnel
    let f0_dielectric = vec3f(0.16 * material.reflectance * material.reflectance);
    let f0_conductor = material.baseColor;
    let f0 = mix(f0_dielectric, f0_conductor, material.metallic);
    let F = F_Schlick_vec3f(LoH, f0, 1.0);
    let f_r = D * Vis * F;

    // == Diffuse lobe ==
    let diffuseColor = (1.0 - material.metallic) * material.baseColor;
    var f_d = vec3f(0.0);
    if (material.highQuality != 0u) {
        f_d = diffuseColor * Fd_Burley(NoV, NoL, LoH, alpha);
    } else {
        f_d = diffuseColor * Fd_Lambert();
    }

    return f_r + f_d;
}

fn D_GGX(NoH: f32, roughness: f32) -> f32 {
    let a = NoH * roughness;
    let k = roughness / (1.0 - NoH * NoH + a * a);
    return k * k * (1.0 / PI);
}

fn V_SmithGGXCorrelatedFast(NoV: f32, NoL: f32, roughness: f32) -> f32 {
    let a = roughness;
    let GGXV = NoL * (NoV * (1.0 - a) + a);
    let GGXL = NoV * (NoL * (1.0 - a) + a);
    return clamp(0.5 / (GGXV + GGXL), 0.0, 1.0);
}

// f90 is 1.0 for specular
fn F_Schlick_vec3f(u: f32, f0: vec3f, f90: f32) -> vec3f {
    let v_pow_1 = 1.0 - u;
    let v_pow_2 = v_pow_1 * v_pow_1;
    let v_pow_5 = v_pow_2 * v_pow_2 * v_pow_1;
    return f0 * (1.0 - v_pow_5) + vec3f(f90) * v_pow_5;
}
fn F_Schlick_f32(u: f32, f0: f32, f90: f32) -> f32 {
    let v_pow_1 = 1.0 - u;
    let v_pow_2 = v_pow_1 * v_pow_1;
    let v_pow_5 = v_pow_2 * v_pow_2 * v_pow_1;
    return f0 * (1.0 - v_pow_5) + f90 * v_pow_5;
}

fn Fd_Lambert() -> f32 {
    return 1.0 / PI;
}

// More costly but more realistic at grazing angles
fn Fd_Burley(NoV: f32, NoL: f32, LoH: f32, roughness: f32) -> f32 {
    let f90 = 0.5 + 2.0 * roughness * LoH * LoH;
    let lightScatter = F_Schlick_f32(NoL, 1.0, f90);
    let viewScatter = F_Schlick_f32(NoV, 1.0, f90);
    return lightScatter * viewScatter / PI;
}

/* **************** BINDINGS **************** */

struct Uniforms {
    currentMipLevel: u32,
    mipLevelCount: u32,
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
    uv: vec2f,
    layer: u32,
}

/* **************** UTILS **************** */

/**
 * Utility function for textureGatherWeights
 */
fn cubeMapUVLFromDirection(direction: vec3f) -> CubeMapUVL {
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
        vec2f(s, t),
        2 * major_axis_idx + sign_offset,
    );
}

/**
 * Inverse of cubeMapUVLFromDirection
 * This returns an unnormalized direction, to save some calculation
 */
fn directionFromCubeMapUVL(uvl: CubeMapUVL) -> vec3f {
    let s = uvl.uv.x;
    let t = uvl.uv.y;
    let abs_ma = 1.0;
    let sc = 2.0 * s - 1.0;
    let tc = 2.0 * t - 1.0;
    var direction = vec3f(0.0);
    switch (uvl.layer) {
        case 0u {
            return vec3f(1.0, -tc, -sc);
        }
        case 1u {
            return vec3f(-1.0, -tc, sc);
        }
        case 2u {
            return vec3f(sc, 1.0, tc);
        }
        case 3u {
            return vec3f(sc, -1.0, -tc);
        }
        case 4u {
            return vec3f(sc, -tc, 1.0);
        }
        case 5u {
            return vec3f(-sc, -tc, -1.0);
        }
        default {
            return vec3f(0.0); // should not happen
        }
    }
}

/**
 * Compute the u/v weights corresponding to the bilinear mix of samples
 * returned by textureGather.
 * To be included in some standard helper library.
 */
fn textureGatherWeights_cubef(t: texture_cube<f32>, direction: vec3f) -> vec2f {
    // major axis direction
    let cubemap_uvl = cubeMapUVLFromDirection(direction);
    let dim = textureDimensions(t).xy;
    var uv = cubemap_uvl.uv;

    // Empirical fix...
    if (cubemap_uvl.layer == 4u || cubemap_uvl.layer == 5u) {
        uv.x = 1.0 - uv.x;
    }

    let scaled_uv = uv * vec2f(dim);
    // This is not accurate, see see https://www.reedbeta.com/blog/texture-gathers-and-coordinate-precision/
    // but bottom line is:
    //   "Unfortunately, if we need this to work, there seems to be no option but to check
    //    which hardware you are running on and apply the offset or not accordingly."
    return fract(scaled_uv - 0.5);
}

/**
 * Compute the u/v weights corresponding to the bilinear mix of samples
 * returned by textureGather.
 * To be included in some standard helper library.
 */
fn textureGatherWeights_2df(t: texture_2d<f32>, uv: vec2f) -> vec2f {
    let dim = textureDimensions(t).xy;
    let scaled_uv = uv * vec2f(dim);
    // This is not accurate, see see https://www.reedbeta.com/blog/texture-gathers-and-coordinate-precision/
    // but bottom line is:
    //   "Unfortunately, if we need this to work, there seems to be no option but to check
    //    which hardware you are running on and apply the offset or not accordingly."
    return fract(scaled_uv - 0.5);
}

/* **************** EQUIRECTANGULAR -> CUBEMAP **************** */

const CUBE_FACE_TRANSFORM = array<mat3x3f,6>(
    mat3x3f(
        0.0, -2.0, 0.0,
        0.0, 0.0, -2.0,
        1.0, 1.0, 1.0,
    ),
    mat3x3f(
        0.0, 2.0, 0.0,
        0.0, 0.0, -2.0,
        -1.0, -1.0, 1.0,
    ),
    mat3x3f(
        2.0, 0.0, 0.0,
        0.0, 0.0, -2.0,
        -1.0, 1.0, 1.0,
    ),
    mat3x3f(
        -2.0, 0.0, 0.0,
        0.0, 0.0, -2.0,
        1.0, -1.0, 1.0,
    ),
    mat3x3f(
        2.0, 0.0, 0.0,
        0.0, 2.0, 0.0,
        -1.0, -1.0, 1.0,
    ),
    mat3x3f(
        2.0, 0.0, 0.0,
        0.0, -2.0, 0.0,
        -1.0, 1.0, -1.0,
    ),
);

@compute @workgroup_size(4, 4, 6)
fn computeCubeMap(@builtin(global_invocation_id) id: vec3u) {
    let outputDimensions = textureDimensions(outputCubemapTexture).xy;
    let inputDimensions = textureDimensions(inputEquirectangularTexture);
    let layer = id.z;

    let uv = vec2f(id.xy) / vec2f(outputDimensions - 1u);

    let direction = directionFromCubeMapUVL(CubeMapUVL(uv, layer));

    let theta = acos(direction.z / length(direction));
    let phi = atan2(direction.y, direction.x);
    let latlong_uv = vec2f(phi / (2.0 * PI) + 0.5, theta / PI);

    let samples = array<vec4f, 4>(
        textureGather(0, inputEquirectangularTexture, textureSampler, latlong_uv),
        textureGather(1, inputEquirectangularTexture, textureSampler, latlong_uv),
        textureGather(2, inputEquirectangularTexture, textureSampler, latlong_uv),
        textureGather(3, inputEquirectangularTexture, textureSampler, latlong_uv),
    );

    let w = textureGatherWeights_2df(inputEquirectangularTexture, latlong_uv);
    // TODO: could be represented as a matrix/vector product
    let color = vec4f(
        mix(mix(samples[0].w, samples[0].z, w.x), mix(samples[0].x, samples[0].y, w.x), w.y),
        mix(mix(samples[1].w, samples[1].z, w.x), mix(samples[1].x, samples[1].y, w.x), w.y),
        mix(mix(samples[2].w, samples[2].z, w.x), mix(samples[2].x, samples[2].y, w.x), w.y),
        mix(mix(samples[3].w, samples[3].z, w.x), mix(samples[3].x, samples[3].y, w.x), w.y),
    );

    textureStore(outputCubemapTexture, id.xy, layer, color);
}

/* **************** CUBEMAP -> EQUIRECTANGULAR **************** */

@compute @workgroup_size(8, 8)
fn computeEquirectangular(@builtin(global_invocation_id) id: vec3u) {
    let outputDimensions = textureDimensions(outputEquirectangularTexture).xy;

    let uv = (vec2f(id.xy) + 0.5) / vec2f(outputDimensions);
    let phi = (uv.x - 0.5) * 2.0 * PI;
    let theta = uv.y * PI;
    let direction = vec3f(
        cos(phi) * sin(theta),
        sin(phi) * sin(theta),
        cos(theta),
    );

    let samples = array<vec4f, 4>(
        textureGather(0, inputCubemapTexture, textureSampler, direction),
        textureGather(1, inputCubemapTexture, textureSampler, direction),
        textureGather(2, inputCubemapTexture, textureSampler, direction),
        textureGather(3, inputCubemapTexture, textureSampler, direction),
    );

    let w = textureGatherWeights_cubef(inputCubemapTexture, direction);
    // TODO: could be represented as a matrix/vector product
    let color = vec4f(
        mix(mix(samples[0].w, samples[0].z, w.x), mix(samples[0].x, samples[0].y, w.x), w.y),
        mix(mix(samples[1].w, samples[1].z, w.x), mix(samples[1].x, samples[1].y, w.x), w.y),
        mix(mix(samples[2].w, samples[2].z, w.x), mix(samples[2].x, samples[2].y, w.x), w.y),
        mix(mix(samples[3].w, samples[3].z, w.x), mix(samples[3].x, samples[3].y, w.x), w.y),
    );
    textureStore(outputEquirectangularTexture, id.xy, color);
}

/* **************** CUBEMAP PREFILTERING **************** */

fn rand(co: vec2f) -> f32 {
    return fract(sin(dot(co, vec2f(12.9898, 78.233))) * 43758.5453);
}

const TOF = 0.5 / f32(0x80000000u);
fn hammersley(i: u32, numSamples: u32) -> vec2f {
    var bits = i;
    bits = (bits << 16) | (bits >> 16);
    bits = ((bits & 0x55555555) << 1) | ((bits & 0xAAAAAAAA) >> 1);
    bits = ((bits & 0x33333333) << 2) | ((bits & 0xCCCCCCCC) >> 2);
    bits = ((bits & 0x0F0F0F0F) << 4) | ((bits & 0xF0F0F0F0) >> 4);
    bits = ((bits & 0x00FF00FF) << 8) | ((bits & 0xFF00FF00) >> 8);
    return vec2(f32(i) / f32(numSamples), f32(bits) * TOF);
}

fn randomDirection(seed: u32) -> vec3f {
    let t = f32(seed) / 32.0;
    let x = rand(vec2f(t, 0.01));
    let y = rand(vec2f(t, x));
    let z = rand(vec2f(t, y));
    return normalize(vec3f(x, y, z));
}

fn importanceSampleDirection(uniformSample: vec2f, alpha: f32) -> vec3f {
    let phi = 2 * PI * uniformSample.x;
    let A = 1 - uniformSample.y;
    let B = 1.0 + (alpha + 1.0) * ((alpha - 1.0) * uniformSample.y);
    let cos_theta2 = A / B;
    let cos_theta = sqrt(cos_theta2);
    let sin_theta = sqrt(1 - cos_theta2);
    return vec3f(cos(phi) * sin_theta, sin(phi) * sin_theta, cos_theta);
}

fn sampleCubeMap(cubemapTexture: texture_cube<f32>, direction: vec3f) -> vec4f {
    let samples = array<vec4f, 4>(
        textureGather(0, cubemapTexture, textureSampler, direction),
        textureGather(1, cubemapTexture, textureSampler, direction),
        textureGather(2, cubemapTexture, textureSampler, direction),
        textureGather(3, cubemapTexture, textureSampler, direction),
    );

    let w = textureGatherWeights_cubef(cubemapTexture, direction);
    
    return vec4f(
        mix(mix(samples[0].w, samples[0].z, w.x), mix(samples[0].x, samples[0].y, w.x), w.y),
        mix(mix(samples[1].w, samples[1].z, w.x), mix(samples[1].x, samples[1].y, w.x), w.y),
        mix(mix(samples[2].w, samples[2].z, w.x), mix(samples[2].x, samples[2].y, w.x), w.y),
        mix(mix(samples[3].w, samples[3].z, w.x), mix(samples[3].x, samples[3].y, w.x), w.y),
    );
}

/**
 * BRDF simplified for IBL prefiltering
 * (sec 5.3.4.2.1 of Filament ref, LD term)
 */
fn brdf_ibl(L: vec3f, N: vec3f, alpha: f32) -> f32 {
    let V = N; // approximation to make prefiltering tractable
    let NoV = 1.0;
    let NoL = clamp(dot(N, L), 0.0, 1.0);
    return V_SmithGGXCorrelatedFast(NoV, NoL, alpha);
}

/**
 * Create a transform matrix that converts coordinates local to the frame whose
 * Z axis is the provided normal to world space coordinates.
 */
fn makeLocalFrame(N: vec3f) -> mat3x3f {
    let Z = N;
    var up = vec3f(0.0, 0.0, 1.0);
    if (abs(N.z) > abs(N.x) && abs(N.z) > abs(N.y)) {
        up = vec3f(0.0, 1.0, 0.0);
    }
    let X = normalize(cross(up, Z));
    let Y = normalize(cross(Z, X));
    return mat3x3f(X, Y, Z);
}

const MIN_ROUGHNESS = 0.002025;
const SAMPLE_COUNT = 512u;
@compute @workgroup_size(4, 4, 6)
fn prefilterCubeMap(@builtin(global_invocation_id) id: vec3u) {
    let outputDimensions = textureDimensions(outputCubemapTexture).xy;

    let layer = id.z;
    var color = vec3f(0.0);
    var total_weight = 0.0;

    let roughness = 0.5;
    let alpha = roughness * roughness;

    let uv = vec2f(id.xy) / vec2f(outputDimensions - 1u);
    let N = normalize(directionFromCubeMapUVL(CubeMapUVL(uv, layer)));

    let local_to_world = makeLocalFrame(N);

    // Sample many light sources in the environment map
    for (var i = 0u ; i < SAMPLE_COUNT ; i++) {
        // Assuming N = (0,0,1)
        let random = hammersley(i, SAMPLE_COUNT);
        let H = importanceSampleDirection(random, alpha);
        let NoH = H.z;
        let NoH2 = NoH * NoH;
        let NoL = clamp(2.0 * NoH2 - 1.0, 0.0, 1.0);
        let L = vec3f(2.0 * NoH * H.x, 2.0 * NoH * H.y, NoL);

        // PDF of the importance sampled direction
        let pdf = D_GGX(NoH, max(MIN_ROUGHNESS, roughness)) * 0.25;

        // TODO
        //let invOmegaS = f32(SAMPLE_COUNT) * pdf;
        //let l = -0.5 * log2(invOmegaS);
        // then sample lod level l in prefiltered cubemap

        if (NoL > 0.0) {
            let L_world = local_to_world * L;
            let radiance_ortho = sampleCubeMap(inputCubemapTexture, L_world).rgb;

            color += brdf_ibl(-L, vec3f(0.0, 0.0, 1.0), alpha) * radiance_ortho * NoL;
            total_weight += L.z;
        }

    }

    color *= (1.0 / total_weight);

    textureStore(outputCubemapTexture, id.xy, layer, vec4f(color, 1.0));
}
