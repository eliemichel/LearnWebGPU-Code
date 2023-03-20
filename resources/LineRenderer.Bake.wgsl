struct ComputeInput {
	@builtin(global_invocation_id) id: vec3<u32>,
}

struct Uniforms {
	resolution: u32,
	time: f32,
}

struct Counts {
	point_count: atomic<u32>,
	allocated_vertices: atomic<u32>,
}

struct VertexInput {
	position: vec3<f32>,
	normal: vec3<f32>,
}

@group(0) @binding(0) var<uniform> uniforms: Uniforms;
@group(0) @binding(1) var<storage,read_write> counts: Counts;
@group(1) @binding(0) var<storage,read_write> vertices: array<VertexInput>;

@compute @workgroup_size(1)
fn main_generate(in: ComputeInput) {
	vertices[0].position = vec3<f32>(0.0, 0.0, 0.0);
	vertices[1].position = vec3<f32>(1.0, 0.0, 0.0);
}
