struct ComputeInput {
	@builtin(global_invocation_id) id: vec3<u32>,
}

struct Uniforms {
	resolution: u32,
}

struct Counts {
	point_count: atomic<u32>,
	allocated_vertices: atomic<u32>,
}

struct VertexInput {
	position: vec4<f32>,
}

@group(0) @binding(0) var<uniform> uniforms: Uniforms;
@group(0) @binding(1) var distance_grid_write: texture_storage_3d<rgba16float,write>;
@group(0) @binding(2) var distance_grid_read: texture_3d<f32>;
@group(0) @binding(3) var<storage,read_write> counts: Counts;
@group(1) @binding(0) var<storage,read_write> vertices: array<VertexInput>;

fn evalSdf(pos: vec3<f32>) -> f32 {
	return length(pos) - 0.5;
}

fn allocateVertices(vertex_count: u32) -> u32 {
	let addr = atomicAdd(&counts.allocated_vertices, vertex_count);
	return addr;
}

@compute @workgroup_size(1)
fn main_eval(in: ComputeInput) {
	let position = vec3<f32>(in.id) / f32(uniforms.resolution) * 2.0 - 1.0;
	let d = evalSdf(position);
	textureStore(distance_grid_write, in.id, vec4<f32>(d));
}

@compute @workgroup_size(1)
fn main_reset_count() {
	atomicStore(&counts.point_count, 0);
	atomicStore(&counts.allocated_vertices, 0);
}

@compute @workgroup_size(1)
fn main_count(in: ComputeInput) {
	let depth = textureLoad(distance_grid_read, in.id, 0).r;
	if (depth < 0) {
		atomicAdd(&counts.point_count, 3);
	}
}

@compute @workgroup_size(1)
fn main_fill(in: ComputeInput) {
	let depth = textureLoad(distance_grid_read, in.id, 0).r;
	if (depth < 0) {
		let addr = allocateVertices(3);
		let position = vec3<f32>(in.id) / f32(uniforms.resolution) * 2.0 - 1.0;
		let z = f32(in.id.x) * 0.1;
		vertices[addr + 0].position = vec4<f32>(position + vec3<f32>(1.0, 0.0, 0.0) * 0.05, 1.0);
		vertices[addr + 1].position = vec4<f32>(position + vec3<f32>(1.0, 1.0, 0.0) * 0.05, 1.0);
		vertices[addr + 2].position = vec4<f32>(position + vec3<f32>(0.0, 1.0, 0.0) * 0.05, 1.0);
	}
}
