struct VertexInput {
	@location(0) position: vec3<f32>,
	@location(1) color: vec3<f32>,
}

struct VertexOutput {
	@builtin(position) position: vec4<f32>,
	@location(0) color: vec3<f32>,
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
	out.color = in.color;
	return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4<f32> {
	let color = in.color;

	// Gamma-correction
	let corrected_color = pow(color, vec3<f32>(2.2));
	return vec4<f32>(corrected_color, 1.0);
}
