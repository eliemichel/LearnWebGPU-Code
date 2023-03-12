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

// Should be a const when Naga allows it
var<private> edge_lut: array<vec2<u32>,12> = array<vec2<u32>,12>(
	vec2<u32>(0u, 1u), // (000, 001)
	vec2<u32>(1u, 3u), // (001, 011)
	vec2<u32>(3u, 2u), // (011, 010)
	vec2<u32>(2u, 0u), // (010, 000)

	vec2<u32>(4u, 5u), // (100, 101)
	vec2<u32>(5u, 7u), // (101, 111)
	vec2<u32>(7u, 6u), // (111, 110)
	vec2<u32>(6u, 4u), // (110, 100)

	vec2<u32>(0u, 4u), // (000, 100)
	vec2<u32>(1u, 5u), // (001, 101)
	vec2<u32>(3u, 7u), // (011, 111)
	vec2<u32>(2u, 6u), // (010, 110)
);

fn sdBox(p: vec3<f32>, b: vec3<f32>) -> f32 {
	let q = abs(p) - b;
	return length(max(q,vec3<f32>(0.0))) + min(max(q.x,max(q.y,q.z)),0.0);
}

fn opUnion(d1: f32, d2: f32) -> f32 {
	return min(d1, d2);
}

fn opSubtraction(d1: f32, d2: f32) -> f32 {
	return max(d1, -d2);
}

fn evalSdf(pos: vec3<f32>) -> f32 {
	let offset1 = vec3<f32>(0.0);
	let alpha = atan2(pos.y, pos.x);
	let wave = cos(5.0 * alpha - 2.0 * uniforms.time + 10.0 * pos.z);
	let radius1 = 0.55 + 0.05 * wave + 0.1 * cos(3.0 * uniforms.time);
	let d1 = length(pos - offset1) - radius1;

	let offset2 = vec3<f32>(0.0, 0.6, 0.6);
	let radius2 = 0.3;
	let d2 = length(pos - offset2) - radius2;

	let th1 = 0.25 * 3.1415 * uniforms.time;
	let c1 = cos(th1);
	let s1 = sin(th1);
	let th2 = 0.3 * 3.1415;
	let c2 = cos(th2);
	let s2 = sin(th2);
	let M = mat3x3<f32>(
		1.0, 0.0, 0.0,
		0.0, c2, s2,
		0.0, -s2, c2,
	) * mat3x3<f32>(
		c1, s1, 0.0,
		-s1, c1, 0.0,
		0.0, 0.0, 1.0,
	);
	let box_pos = M * (pos - vec3<f32>(0.0, 0.0, 0.8));
	let box = sdBox(box_pos, vec3<f32>(0.15, 0.4, 1.2));

	return opSubtraction(opUnion(d1, d2), box);
}

fn evalNormal(pos: vec3<f32>) -> vec3<f32> {
	let eps = 0.0001;
    let k = vec2<f32>(1.0, -1.0);
    return normalize(k.xyy * evalSdf(pos + k.xyy * eps) +
                     k.yyx * evalSdf(pos + k.yyx * eps) +
                     k.yxy * evalSdf(pos + k.yxy * eps) +
                     k.xxx * evalSdf(pos + k.xxx * eps));
}

fn allocateVertices(vertex_count: u32) -> u32 {
	let addr = atomicAdd(&counts.allocated_vertices, vertex_count);
	return addr;
}

// Transform a corner index into a grid index offset
fn cornerOffset(i: u32) -> vec3<u32> {
	return vec3<u32>(
		(i & (1u << 0u)) >> 0u,
		(i & (1u << 1u)) >> 1u,
		(i & (1u << 2u)) >> 2u,
	);
}
fn cornerOffsetF(i: u32) -> vec3<f32> {
	return vec3<f32>(cornerOffset(i));
}

fn positionFromGridCoord(grid_coord: vec3<f32>) -> vec3<f32> {
	return grid_coord / f32(uniforms.resolution) * 2.0 - 1.0;
}

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
	atomicStore(&counts.point_count, 0u);
	atomicStore(&counts.allocated_vertices, 0u);
}

@compute @workgroup_size(1)
fn main_count(in: ComputeInput) {
	let d = textureLoad(distance_grid_read, in.id, 0).r;

	var local_vertex_count = 0u;

	for (var dim = 0u ; dim < 3u ; dim++) {
		if (in.id[dim] < uniforms.resolution - 1u) {
			var neighborId = in.id;
			neighborId[dim]++;
			let neighborD = textureLoad(distance_grid_read, neighborId, 0).r;
			if ((d < 0.0) != (neighborD < 0.0)) {
				local_vertex_count++;
			}
		}
	}

	atomicAdd(&counts.point_count, 6u * local_vertex_count);

	// Compute the position of the vertex created for the grid cell (if any)
	// Option A: "Bloxel"
	//textureStore(position_grid_write, in.id, vec4<f32>(0.5, 0.5, 0.5, 1.0));
	//return;

	// Compute the position of the center cell by solving QEF
	// This sums the contributions of the 12 edges of the cube
	var cornerDepth: array<f32,8>;
	for (var i = 0u ; i < 8u ; i++) {
		cornerDepth[i] = textureLoad(distance_grid_read, in.id + cornerOffset(i), 0).r;
	}
	//var qef = 0.0;
	let X = vec3<f32>(0.5, 0.5, 0.5);
	var sample_sum = vec3<f32>(0.0);
	var sample_count = 0u;
	for (var k = 0u ; k < 12u ; k++) {
		let iA = edge_lut[k].x;
		let iB = edge_lut[k].y;
		let dA = cornerDepth[iA];
		let dB = cornerDepth[iB];
		let offset = (vec3<f32>(cornerOffset(iA)) + vec3<f32>(cornerOffset(iB))) / 2.0;
		if ((dA < 0.0) != (dB < 0.0)) {
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

	for (var dim = 0u ; dim < 3u ; dim++) {
		if (in.id[dim] < uniforms.resolution - 1u) {
			var neighborId = in.id;
			neighborId[dim]++;
			var rightId = vec3<u32>(0u);
			rightId[(dim + 1u) % 3u] = 1u;
			var upId = vec3<u32>(0u);
			upId[(dim + 2u) % 3u] = 1u;

			let neighborD = textureLoad(distance_grid_read, neighborId, 0).r;
			if ((d < 0.0) != (neighborD < 0.0)) {
				let addr = allocateVertices(6u);
				let grid_coord = (vec3<f32>(in.id) + vec3<f32>(neighborId)) / 2.0;
				let position = positionFromGridCoord(grid_coord);
				let size = 1.0 / f32(uniforms.resolution);

				let vertex11 = loadVertexPosition(in.id);
				let vertex01 = loadVertexPosition(in.id - rightId);
				let vertex10 = loadVertexPosition(in.id - upId);
				let vertex00 = loadVertexPosition(in.id - rightId - upId);

				vertices[addr + 0u].position = vertex00;
				vertices[addr + 1u].position = vertex10;
				vertices[addr + 2u].position = vertex11;
				vertices[addr + 3u].position = vertex00;
				vertices[addr + 4u].position = vertex11;
				vertices[addr + 5u].position = vertex01;
				for (var i = 0u ; i < 6u ; i++) {
					vertices[addr + i].normal = evalNormal(vertices[addr + i].position);
					vertices[addr + i].position += vec3<f32>(1.0, 0.0, 0.0); // global offset
				}
			}
		}
	}
}
