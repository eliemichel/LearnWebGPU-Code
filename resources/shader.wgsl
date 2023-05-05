struct VertexInput {
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
	hardness: f32,
	kd: f32,
	ks: f32,
	normalMapStrength: f32,
}

// Instead of the simple uTime variable, our uniform variable is a struct
@group(0) @binding(0) var<uniform> uMyUniforms: MyUniforms;
@group(0) @binding(5) var<uniform> uLighting: LightingUniforms;

@group(0) @binding(1) var textureSampler: sampler;
@group(0) @binding(2) var baseColorTexture: texture_2d<f32>;
@group(0) @binding(3) var normalTexture: texture_2d<f32>;
@group(0) @binding(4) var cubemapTexture: texture_cube<f32>;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
	var out: VertexOutput;
	let worldPosition = uMyUniforms.modelMatrix * vec4f(in.position, 1.0);
	out.position = uMyUniforms.projectionMatrix * uMyUniforms.viewMatrix * worldPosition;
	out.color = in.color;
	out.tangent = (uMyUniforms.modelMatrix * vec4f(in.tangent, 0.0)).xyz;
	out.bitangent = (uMyUniforms.modelMatrix * vec4f(in.bitangent, 0.0)).xyz;
	out.normal = (uMyUniforms.modelMatrix * vec4f(in.normal, 0.0)).xyz;
	out.uv = in.uv;
	out.viewDirection = uMyUniforms.cameraWorldPosition - worldPosition.xyz;
	return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
	// Sample normal
	let encodedN = textureSample(normalTexture, textureSampler, in.uv).rgb;
	let localN = encodedN - 0.5;
	let rotation = mat3x3f(
		normalize(in.tangent),
		normalize(in.bitangent),
		normalize(in.normal),
	);
	let rotatedN = normalize(rotation * localN);
	let N = mix(in.normal, rotatedN, uLighting.normalMapStrength);

	// Compute shading
	let V = normalize(in.viewDirection);
	
	// Instead of looping over the light sources, we sample the environment map
	// in the reflected direction to get the specular shading.
	let ibl_direction = -reflect(V, N);
	let roughness = pow(1.0 - uLighting.hardness / 128.0, 1.0); // ad-hoc remapping to [0,1]
	let bias = mix(0.0, 10.0, roughness);
	let ibl_sample = textureSampleBias(cubemapTexture, textureSampler, ibl_direction, bias).rgb;

	var diffuse = vec3f(0.0);
	let specular = ibl_sample;
	
	// Sample texture
	let baseColor = textureSample(baseColorTexture, textureSampler, in.uv).rgb;

	// Combine texture and lighting
	let color = baseColor * uLighting.kd * diffuse + uLighting.ks * specular;

	// To debug normals
	//let color = N * 0.5 + 0.5;

	// Gamma-correction
	let corrected_color = pow(color, vec3f(uMyUniforms.gamma));
	return vec4f(corrected_color, uMyUniforms.color.a);
}
