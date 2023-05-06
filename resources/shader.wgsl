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

/**
 * Sample a local normal from the normal map and rotate it using the normal
 * frame to get a global normal.
 */
fn sampleNormal(in: VertexOutput, normalMapStrength: f32) -> vec3f {
	let encodedN = textureSample(normalTexture, textureSampler, in.uv).rgb;
	let localN = encodedN - 0.5;
	let rotation = mat3x3f(
		normalize(in.tangent),
		normalize(in.bitangent),
		normalize(in.normal),
	);
	let rotatedN = normalize(rotation * localN);
	return normalize(mix(in.normal, rotatedN, normalMapStrength));
}

/* **************** BINDINGS **************** */

struct VertexInput {
	@builtin(instance_index) instance_index: u32,
	@location(0) position: vec3f,
	@location(4) tangent: vec3f,
	@location(5) bitangent: vec3f,
	@location(1) normal: vec3f,
	@location(2) color: vec3f,
	@location(3) uv: vec2f,
}

struct VertexOutput {
	@builtin(position) position: vec4f,
	@location(0) color: vec3f,
	@location(4) tangent: vec3f,
	@location(5) bitangent: vec3f,
	@location(1) normal: vec3f,
	@location(2) uv: vec2f,
	@location(3) viewDirection: vec3f,
	@location(6) instance: f32,
}

/**
 * A structure holding the value of our uniforms
 */
struct MyUniforms {
	projectionMatrix: mat4x4f,
	viewMatrix: mat4x4f,
	modelMatrix: mat4x4f,
	color: vec4f,
	cameraWorldPosition: vec3f,
	time: f32,
	gamma: f32,
}

/**
 * A structure holding the lighting settings
 */
struct LightingUniforms {
	directions: array<vec4f, 2>,
	colors: array<vec4f, 2>,
	roughness: f32,
	metallic: f32,
	reflectance: f32,
	normalMapStrength: f32,
	highQuality: u32,
	roughness2: f32,
	metallic2: f32,
	reflectance2: f32,
	instanceSpacing: f32,
}

// Instead of the simple uTime variable, our uniform variable is a struct
@group(0) @binding(0) var<uniform> uMyUniforms: MyUniforms;
@group(0) @binding(5) var<uniform> uLighting: LightingUniforms;

@group(0) @binding(1) var textureSampler: sampler;
@group(0) @binding(2) var baseColorTexture: texture_2d<f32>;
@group(0) @binding(3) var normalTexture: texture_2d<f32>;
@group(0) @binding(4) var environmentTexture: texture_2d<f32>;

/* **************** VERTEX MAIN **************** */

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
	var out: VertexOutput;
	let worldPosition = uMyUniforms.modelMatrix * vec4f(in.position, 1.0);
	out.instance = (f32(in.instance_index) + 0.5) / 8.0;
	
	// A trick to offset instances without changing their perspective
	var proj = uMyUniforms.projectionMatrix;
	proj[2][0] = (out.instance - 0.5) * uLighting.instanceSpacing;

	out.position = proj * uMyUniforms.viewMatrix * worldPosition;
	out.color = in.color;
	out.tangent = (uMyUniforms.modelMatrix * vec4f(in.tangent, 0.0)).xyz;
	out.bitangent = (uMyUniforms.modelMatrix * vec4f(in.bitangent, 0.0)).xyz;
	out.normal = (uMyUniforms.modelMatrix * vec4f(in.normal, 0.0)).xyz;
	out.uv = in.uv;
	out.viewDirection = uMyUniforms.cameraWorldPosition - worldPosition.xyz;
	return out;
}

/* **************** FRAGMENT MAIN **************** */

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
	let N = sampleNormal(in, uLighting.normalMapStrength);

	// Compute shading
	let V = normalize(in.viewDirection);

	/*
	// Instead of looping over the light sources, we sample the environment map
	// in the reflected direction to get the specular shading.
	let ibl_direction = -reflect(V, N);
	let Pi = 3.14159266;
	let theta = acos(ibl_direction.z / length(ibl_direction));
	let phi = atan2(length(ibl_direction.xy), ibl_direction.x);
	let ibl_uv = vec2f(phi / (2.0 * Pi), theta / Pi + 0.5);
	let ibl_sample = textureSample(environmentTexture, textureSampler, ibl_uv).rgb;

	var diffuse = vec3f(0.0);
	let specular = ibl_sample;
	*/
	
	// Sample texture
	let baseColor = textureSample(baseColorTexture, textureSampler, in.uv).rgb;

	// Combine texture and lighting
	//let color = baseColor * uLighting.kd * diffuse + uLighting.ks * specular;

	// To debug normals
	//let color = N * 0.5 + 0.5;

	let material = MaterialProperties(
		baseColor,
		mix(uLighting.roughness, uLighting.roughness2, in.instance),
		mix(uLighting.metallic, uLighting.metallic2, in.instance),
		mix(uLighting.reflectance, uLighting.reflectance2, in.instance),
		uLighting.highQuality,
	);
	var color = vec3f(0.0);
	for (var i: i32 = 0; i < 2; i++) {
		let L = normalize(uLighting.directions[i].xyz);
		let lightEnergy = uLighting.colors[i].rgb * 2.0;
		color += brdf(material, N, L, V) * lightEnergy;
	}

	// Gamma-correction
	let corrected_color = pow(color, vec3f(uMyUniforms.gamma));
	return vec4f(corrected_color, uMyUniforms.color.a);
}
