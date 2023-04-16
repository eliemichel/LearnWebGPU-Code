@group(0) @binding(0) var<storage,read> inputBuffer: array<f32,32>;
@group(0) @binding(1) var<storage,read_write> outputBuffer: array<f32,32>;

@compute @workgroup_size(32)
fn computeStuff(@builtin(global_invocation_id) id: vec3<u32>) {
    // Compute stuff
    outputBuffer[id.x] = 2.0 * inputBuffer[id.x] + 1.0;
}
