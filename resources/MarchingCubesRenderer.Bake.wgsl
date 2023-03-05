struct VertexInput {
	@location(0) position: vec2<f32>,
}

struct VertexOutput {
	@builtin(position) position: vec4<f32>,
}

struct Uniforms {
	resolution: u32,
}

@group(0) @binding(0) var<uniform> uniforms: Uniforms;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
	var out: VertexOutput;
	out.position = vec4<f32>(in.position, 0.0, 1.0);
	return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4<f32> {
	let color = vec3<f32>(1.0, 0.5, 0.0);

	// Gamma-correction
	let corrected_color = pow(color, vec3<f32>(2.2));
	return vec4<f32>(corrected_color, 1.0);
}
