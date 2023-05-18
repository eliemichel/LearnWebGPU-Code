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

struct VertexInput {
	@builtin(instance_index) instance_index: u32,
	@location(0) position: vec3f,
	@location(1) normal: vec3f,
	@location(2) color: vec3f,
	@location(3) uv: vec2f,
}

struct VertexOutput {
	@builtin(position) position: vec4f,
	@location(0) color: vec3f,
	@location(1) normal: vec3f,
	@location(2) uv: vec2f,
	@location(3) viewDirection: vec3f,
}

/**
 * A structure holding the value of our uniforms
 */
struct Uniforms {
	projectionMatrix: mat4x4f,
	viewMatrix: mat4x4f,
	modelMatrix: mat4x4f,
	color: vec4f,
	cameraWorldPosition: vec3f,
	time: f32,
	gamma: f32,
}

// Instead of the simple uTime variable, our uniform variable is a struct
@group(0) @binding(0) var<uniform> uniforms: Uniforms;
@group(0) @binding(1) var textureSampler: sampler;

/* **************** VERTEX MAIN **************** */

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
	var out: VertexOutput;
	let worldPosition = uniforms.modelMatrix * vec4f(in.position, 1.0);
	var proj = uniforms.projectionMatrix;
	out.position = proj * uniforms.viewMatrix * worldPosition;
	out.color = in.color;
	out.normal = (uniforms.modelMatrix * vec4f(in.normal, 0.0)).xyz;
	out.uv = in.uv;
	out.viewDirection = uniforms.cameraWorldPosition - worldPosition.xyz;
	return out;
}

/* **************** FRAGMENT MAIN **************** */

const cLightingDirections = array<vec3f,2>(
	vec3f(1.0, 1.0, 1.0),
	vec3f(-1.0, -0.5, 0.5),
);

const cLightingColors = array<vec3f,2>(
	vec3f(1.0, 0.98, 0.95),
	vec3f(0.95, 0.98, 1.0),
);

const cLightingPower = 1.2;

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
	let N = normalize(in.normal);

	// Compute shading
	let V = normalize(in.viewDirection);

	let material = MaterialProperties(
		vec3f(1.0, 0.0, 0.0), // basecolor
		0.6, // roughness
		0.0, // metallic
		0.2, // reflectance
		1u, // high quality
	);
	var color = vec3f(0.0);

	for (var i: i32 = 0; i < 2; i++) {
		let L = normalize(cLightingDirections[i].xyz);
		let lightEnergy = cLightingColors[i].rgb * cLightingPower;
		color += brdf(material, N, L, V) * lightEnergy;
	}

	// Gamma-correction
	let corrected_color = pow(color, vec3f(uniforms.gamma));
	return vec4f(corrected_color, uniforms.color.a);
}
