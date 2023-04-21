@group(0) @binding(0) var<storage,read> inputBuffer: array<f32,64>;
@group(0) @binding(1) var<storage,read_write> outputBuffer: array<f32,64>;

// The function to evaluate for each element of the processed buffer
fn f(x: f32) -> f32 {
    return 2.0 * x + 1.0;
}

@compute @workgroup_size(32)
fn computeStuff(@builtin(global_invocation_id) id: vec3<u32>) {
    // Apply the function f to the buffer element at index id.x:
    outputBuffer[id.x] = f(inputBuffer[id.x]);
}
