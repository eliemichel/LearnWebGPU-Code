struct VertexInput {
	@location(0) position: vec4<f32>,
}

struct VertexOutput {
	@builtin(position) position: vec4<f32>,
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
	return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4<f32> {
	let color = vec3<f32>(1.0, 0.5, 0.0);

	// Gamma-correction
	let corrected_color = pow(color, vec3<f32>(2.2));
	return vec4<f32>(corrected_color, 1.0);
}
