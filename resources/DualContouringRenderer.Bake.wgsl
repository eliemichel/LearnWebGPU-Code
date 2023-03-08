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
@group(0) @binding(1) var distance_grid_write: texture_storage_3d<rgba16float,write>;
@group(0) @binding(2) var distance_grid_read: texture_3d<f32>;
@group(0) @binding(3) var<storage,read_write> counts: Counts;
@group(0) @binding(5) var position_grid_write: texture_storage_3d<rgba16float,write>;
@group(0) @binding(6) var position_grid_read: texture_3d<f32>;
@group(1) @binding(0) var<storage,read_write> vertices: array<VertexInput>;

const edge_lut = array<vec2<u32>,12>(
	vec2<u32>(0, 1), // (000, 001)
	vec2<u32>(1, 3), // (001, 011)
	vec2<u32>(3, 2), // (011, 010)
	vec2<u32>(2, 0), // (010, 000)

	vec2<u32>(4, 5), // (100, 101)
	vec2<u32>(5, 7), // (101, 111)
	vec2<u32>(7, 6), // (111, 110)
	vec2<u32>(6, 4), // (110, 100)

	vec2<u32>(0, 4), // (000, 100)
	vec2<u32>(1, 5), // (001, 101)
	vec2<u32>(3, 7), // (011, 111)
	vec2<u32>(2, 6), // (010, 110)
);

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
fn cornerOffset(i: u32) -> vec3<u32> {
	return vec3<u32>(
		(i & (1u << 0)) >> 0,
		(i & (1u << 1)) >> 1,
		(i & (1u << 2)) >> 2,
	);
}
fn cornerOffsetF(i: u32) -> vec3<f32> {
	return vec3<f32>(cornerOffset(i));
}

fn positionFromGridCoord(grid_coord: vec3<f32>) -> vec3<f32> {
	return grid_coord / f32(uniforms.resolution) * 2.0 - 1.0;
}

/*
fn computeQef(position: vec3<f32>, normal: vec3<f32>) -> vec3<f32> {
	// TODO: put in a filterable form
	// qef(X) = pow(dot(X - position, normal), 2);

	= dot(X, normal)^2 - 2 * dot(X, normal) * dot(position, normal) + dot(position, normal)^2
	= dot(X, normal)^2 - 2 * dot(X, normal) * dot(position, normal) + dot(position, normal)^2

	return vec3<f32>(0.0, 0.0, 0.0);
}

fn accumulateQef(cell_id, qef) {
	// TODO: atomic
	let d = textureLoad(distance_grid_read, cell_id, 0).r;
	textureStore(distance_grid_write, cell_id, vec4<f32>(d, qef));
}
*/

fn loadVertexPosition(cell_coord: vec3<u32>) -> vec3<f32> {
	let offset = textureLoad(position_grid_read, cell_coord, 0).xyz;
	return positionFromGridCoord(vec3<f32>(cell_coord) + offset);
}

@compute @workgroup_size(1)
fn main_eval(in: ComputeInput) {
	let position = positionFromGridCoord(vec3<f32>(in.id));
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
	let d = textureLoad(distance_grid_read, in.id, 0).r;

	var local_vertex_count = 0u;

	for (var dim = 0u ; dim < 3 ; dim++) {
		if (in.id[dim] < uniforms.resolution - 1) {
			var neighborId = in.id;
			neighborId[dim]++;
			let neighborD = textureLoad(distance_grid_read, neighborId, 0).r;
			if ((d < 0) != (neighborD < 0)) {
				local_vertex_count++;
			}
		}
	}

	atomicAdd(&counts.point_count, 6 * local_vertex_count);

	// Compute the position of the vertex created for the grid cell (if any)
	// Option A: "Bloxel"
	//textureStore(position_grid_write, in.id, vec4<f32>(0.5, 0.5, 0.5, 1.0));
	//return;

	// Compute the position of the center cell by solving QEF
	// This sums the contributions of the 12 edges of the cube
	var cornerDepth: array<f32,8>;
	for (var i = 0u ; i < 8 ; i++) {
		cornerDepth[i] = textureLoad(distance_grid_read, in.id + cornerOffset(i), 0).r;
	}
	//var qef = 0.0;
	let X = vec3<f32>(0.5, 0.5, 0.5);
	var sample_sum = vec3<f32>(0.0);
	var sample_count = 0u;
	for (var k = 0u ; k < 12 ; k++) {
		let iA = edge_lut[k].x;
		let iB = edge_lut[k].y;
		let dA = cornerDepth[iA];
		let dB = cornerDepth[iB];
		let offset = (vec3<f32>(cornerOffset(iA)) + vec3<f32>(cornerOffset(iB))) / 2.0;
		if ((dA < 0) != (dB < 0)) {
			// Option B: Simple average
			sample_sum += offset;
			sample_count++;

			// Option C: ?
			//let normal = evalNormal(positionFromGridCoord(vec3<f32>(in.id) + offset));
			//let err = dot(X - position, normal);
			//qef += err * err;
		}
	}
	let vertex = sample_sum / f32(sample_count);
	textureStore(position_grid_write, in.id, vec4<f32>(vertex, 1.0));
}

@compute @workgroup_size(1)
fn main_fill(in: ComputeInput) {
	let d = textureLoad(distance_grid_read, in.id, 0).r;

	for (var dim = 0u ; dim < 3 ; dim++) {
		if (in.id[dim] < uniforms.resolution - 1) {
			var neighborId = in.id;
			neighborId[dim]++;
			var rightId = vec3<u32>(0, 0, 0);
			rightId[(dim + 1) % 3] = 1;
			var upId = vec3<u32>(0, 0, 0);
			upId[(dim + 2) % 3] = 1;

			let neighborD = textureLoad(distance_grid_read, neighborId, 0).r;
			if ((d < 0) != (neighborD < 0)) {
				let addr = allocateVertices(6);
				let grid_coord = (vec3<f32>(in.id) + vec3<f32>(neighborId)) / 2.0;
				let position = positionFromGridCoord(grid_coord);
				let size = 1.0 / f32(uniforms.resolution);

				let vertex11 = loadVertexPosition(in.id);
				let vertex01 = loadVertexPosition(in.id - rightId);
				let vertex10 = loadVertexPosition(in.id - upId);
				let vertex00 = loadVertexPosition(in.id - rightId - upId);

				vertices[addr + 0].position = vertex00;
				vertices[addr + 1].position = vertex10;
				vertices[addr + 2].position = vertex11;
				vertices[addr + 3].position = vertex00;
				vertices[addr + 4].position = vertex11;
				vertices[addr + 5].position = vertex01;
				for (var i = 0u ; i < 6 ; i++) {
					vertices[addr + i].normal = evalNormal(vertices[addr + i].position);
					vertices[addr + i].position += vec3<f32>(2.0, 0.0, 0.0); // global offset
				}
			}
		}
	}
}
