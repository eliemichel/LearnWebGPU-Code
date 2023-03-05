struct ComputeInput {
	@builtin(global_invocation_id) id: vec3<u32>,
}

struct Uniforms {
	resolution: u32,
}

@group(0) @binding(0) var<uniform> uniforms: Uniforms;

@compute @workgroup_size(1)
fn main_eval(in: ComputeInput) {
}

@compute @workgroup_size(1)
fn main_count(in: ComputeInput) {
}

@compute @workgroup_size(1)
fn main_fill(in: ComputeInput) {
}
