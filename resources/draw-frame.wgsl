/* **************** BINDINGS **************** */

struct VertexInput {
	@location(0) position: vec3f,
	@location(1) color: vec3f,
};

struct VertexOutput {
	@builtin(position) position: vec4f,
	@location(0) color: vec3f,
};

/**
 * A structure holding the value of our uniforms
 */
struct GlobalUniforms {
	projectionMatrix: mat4x4f,
	viewMatrix: mat4x4f,
	cameraWorldPosition: vec3f,
	time: f32,
	gamma: f32,
};

/**
 * A structure holding the lighting settings
 */
struct LightingUniforms {
	directions: array<vec4f, 2>,
	colors: array<vec4f, 2>,
}

/**
 * Uniforms specific to a given GLTF node.
 */
struct NodeUniforms {
	modelMatrix: mat4x4f,
}

/**
 * A structure holding material properties as they are provided from the CPU code
 */
struct MaterialUniforms {
	baseColorFactor: vec4f,
	metallicFactor: f32,
	roughnessFactor: f32,
	baseColorTexCoords: u32,
	metallicTexCoords: u32,
	roughnessTexCoords: u32,
}

// General bind group
@group(0) @binding(0) var<uniform> uGlobal: GlobalUniforms;
@group(0) @binding(1) var<uniform> uLighting: LightingUniforms;

// Material bind group
@group(1) @binding(0) var<uniform> uMaterial: MaterialUniforms;
@group(1) @binding(1) var baseColorTexture: texture_2d<f32>;
@group(1) @binding(2) var baseColorSampler: sampler;
@group(1) @binding(3) var metallicRoughnessTexture: texture_2d<f32>;
@group(1) @binding(4) var metallicRoughnessSampler: sampler;
@group(1) @binding(3) var normalTexture: texture_2d<f32>;
@group(1) @binding(4) var normalSampler: sampler;

// Node bind group
@group(2) @binding(0) var<uniform> uNode: NodeUniforms;

/* **************** VERTEX MAIN **************** */

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
	var out: VertexOutput;
	let worldPosition = uNode.modelMatrix * vec4f(in.position, 1.0);
	out.position = uGlobal.projectionMatrix * uGlobal.viewMatrix * worldPosition;
	out.normal = (uNode.modelMatrix * vec4f(in.normal, 0.0)).xyz;
	out.color = in.color;
	out.uv = in.uv;
	out.viewDirection = uGlobal.cameraWorldPosition - worldPosition.xyz;
	return out;
}

/* **************** FRAGMENT MAIN **************** */

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
	// Sample texture
	let baseColor = textureSample(baseColorTexture, baseColorSampler, in.uv).rgb;
	let metallicRoughness = textureSample(metallicRoughnessTexture, metallicRoughnessSampler, in.uv).rgb;

	let material = MaterialProperties(
		baseColor,
		metallicRoughness.y,
		metallicRoughness.x,
		1.0, // reflectance
		1u, // high quality
	);

	let normalMapStrength = 1.0;
	//let N = sampleNormal(in, normalMapStrength);
	let N = normalize(in.normal);
	let V = normalize(in.viewDirection);

	// Compute shading
	var color = vec3f(0.0);
	for (var i: i32 = 0; i < 2; i++) {
		let L = normalize(uLighting.directions[i].xyz);
		let lightEnergy = uLighting.colors[i].rgb;
		color += brdf(material, N, L, V) * lightEnergy;
	}

	// Debug normals
	//color = N * 0.5 + 0.5;
	
	// Gamma-correction
	let corrected_color = pow(color, vec3f(uGlobal.gamma));
	return vec4f(corrected_color, 1.0);
}
