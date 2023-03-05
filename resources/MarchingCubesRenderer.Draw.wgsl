struct VertexInput {
	@location(0) position: vec3<f32>,
	@location(1) normal: vec3<f32>,
}

struct VertexOutput {
	@builtin(position) position: vec4<f32>,
	@location(0) normal: vec3<f32>,
}

struct MarchingCubesUniforms {
	resolution: u32,
}

struct CameraUniforms {
	projectionMatrix: mat4x4<f32>,
	viewMatrix: mat4x4<f32>,
	modelMatrix: mat4x4<f32>,
	color: vec4<f32>,
	time: f32,
}

@group(0) @binding(0) var<uniform> uMarchingCubes: MarchingCubesUniforms;
@group(0) @binding(1) var<uniform> uCamera: CameraUniforms;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
	var out: VertexOutput;
	out.position = uCamera.projectionMatrix * uCamera.viewMatrix * uCamera.modelMatrix * vec4<f32>(in.position.xyz, 1.0);
	out.normal = (uCamera.modelMatrix * vec4<f32>(in.normal.xyz, 0.0)).xyz;
	return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4<f32> {
	let normal = normalize(in.normal);
	let lightDirection1 = normalize(vec3<f32>(0.5, -0.9, 0.1));
	let lightDirection2 = normalize(vec3<f32>(0.2, 0.4, 0.3));
	let lightColor1 = vec3<f32>(1.0, 0.9, 0.6);
	let lightColor2 = vec3<f32>(0.6, 0.9, 1.0);
	let shading1 = max(0.0, dot(lightDirection1, normal));
	let shading2 = max(0.0, dot(lightDirection2, normal));
	let shading = shading1 * lightColor1 + shading2 * lightColor2;
    //let color = vec3<f32>(1.0, 0.5, 0.0) * shading;
    let color = vec3<f32>(1.0) * shading;

	// Gamma-correction
	let corrected_color = pow(color, vec3<f32>(2.2));
	return vec4<f32>(corrected_color, 1.0);
}
