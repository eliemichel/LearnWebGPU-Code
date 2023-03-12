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

struct ModuleLutEntry {
	// Represent a point on an edge of the unit cube
	edge_start_corner: u32,
	edge_end_corner: u32,
};
struct ModuleLut {
	// end_offset[i] is the beginning of the i+1 th entry
	end_offset: array<u32, 256>,
	// Each entry represents a point, to be grouped by 3 to form triangles
	entries: array<ModuleLutEntry>,
};

@group(0) @binding(0) var<uniform> uniforms: Uniforms;
@group(0) @binding(1) var distance_grid_write: texture_storage_2d<rgba16float,write>;
@group(0) @binding(2) var distance_grid_read: texture_2d<f32>;
@group(0) @binding(3) var<storage,read_write> counts: Counts;
@group(0) @binding(4) var<storage,read> moduleLut: ModuleLut;
@group(1) @binding(0) var<storage,read_write> vertices: array<VertexInput>;

fn evalSdf(pos: vec3<f32>) -> f32 {
	let offset1 = vec3<f32>(0.0);
	let radius1 = 0.55 + 0.1 * cos(3.0 * uniforms.time);
	let d1 = length(pos - offset1) - radius1;

	let offset2 = vec3<f32>(0.0, 0.6, 0.6);
	let radius2 = 0.3;
	let d2 = length(pos - offset2) - radius2;
	return min(d1, d2);
}

fn evalNormal(pos: vec3<f32>) -> vec3<f32> {
	let offset1 = vec3<f32>(0.0);
	let radius1 = 0.55 + 0.1 * cos(3.0 * uniforms.time);
	let d1 = length(pos - offset1) - radius1;

	let offset2 = vec3<f32>(0.0, 0.6, 0.6);
	let radius2 = 0.3;
	let d2 = length(pos - offset2) - radius2;

	if (d1 < d2) {
		return normalize(pos - offset1);
	} else {
		return normalize(pos - offset2);
	}
}

fn allocateVertices(vertex_count: u32) -> u32 {
	let addr = atomicAdd(&counts.allocated_vertices, vertex_count);
	return addr;
}

// Transform a corner index into a grid index offset
fn cornerOffset(i: u32) -> vec2<u32> {
	return vec2<u32>(
		(i & (1u << 0u)) >> 0u,
		(i & (1u << 1u)) >> 1u,
	);
}
fn cornerOffsetF(i: u32) -> vec2<f32> {
	return vec2<f32>(cornerOffset(i));
}

fn positionFromGridCoord(grid_coord: vec2<f32>) -> vec3<f32> {
	return vec3<f32>(grid_coord, 0.0) / f32(uniforms.resolution) * 2.0 - 1.0;
}

@compute @workgroup_size(1)
fn main_eval(in: ComputeInput) {
	let position = positionFromGridCoord(vec2<f32>(in.id.xy));
	let d = evalSdf(position);
	textureStore(distance_grid_write, in.id.xy, vec4<f32>(d));
}

@compute @workgroup_size(1)
fn main_reset_count() {
	atomicStore(&counts.point_count, 0u);
	atomicStore(&counts.allocated_vertices, 0u);
}

@compute @workgroup_size(1)
fn main_count(in: ComputeInput) {
	var cornerDepth: array<f32,8>;
	var module_code: u32 = 0u;
	for (var i: u32 = 0u ; i < 8u ; i++) {
		cornerDepth[i] = textureLoad(distance_grid_read, in.id.xy + cornerOffset(i), 0).r;
		if (cornerDepth[i] < 0.0) {
			module_code += 1u << i;
		}
	}

	var begin_offset = 0u;
	if (module_code > 0u) {
		begin_offset = moduleLut.end_offset[module_code - 1u];
	}
	let module_point_count = moduleLut.end_offset[module_code] - begin_offset;

	atomicAdd(&counts.point_count, module_point_count);
}

@compute @workgroup_size(1)
fn main_fill(in: ComputeInput) {
	var cornerDepth: array<f32,8>;
	var module_code: u32 = 0u;
	for (var i = 0u ; i < 8u ; i++) {
		cornerDepth[i] = textureLoad(distance_grid_read, in.id.xy + cornerOffset(i), 0).r;
		if (cornerDepth[i] < 0.0) {
			module_code += 1u << i;
		}
	}

	var begin_offset = 0u;
	if (module_code > 0u) {
		begin_offset = moduleLut.end_offset[module_code - 1u];
	}
	let module_point_count = moduleLut.end_offset[module_code] - begin_offset;

	let addr = allocateVertices(module_point_count);
	for (var i = 0u ; i < module_point_count ; i++) {
		let entry = moduleLut.entries[begin_offset + i];
		let edge_start_corner = cornerOffsetF(entry.edge_start_corner);
		let edge_end_corner = cornerOffsetF(entry.edge_end_corner);
		let start_depth = cornerDepth[entry.edge_start_corner];
		let end_depth = cornerDepth[entry.edge_end_corner];

		let fac = -start_depth / (end_depth - start_depth);
		let grid_offset = edge_start_corner * (1.0 - fac) + edge_end_corner * fac;
		
		var grid_coord = vec2<f32>(in.id.xy) + grid_offset;
		let position = positionFromGridCoord(grid_coord);
		vertices[addr + i].position = position + vec3<f32>(0.0, 0.0, 0.0); // global offset for debug
		vertices[addr + i].normal = evalNormal(position);
	}
}
