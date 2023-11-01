struct Uniforms {
    kernel: mat3x3<f32>,
    test: f32,
    filter_type: u32,
}

@group(0) @binding(0) var inputTexture: texture_2d<f32>;
@group(0) @binding(1) var outputTexture: texture_storage_2d<rgba8unorm,write>;
@group(0) @binding(2) var<uniform> uniforms: Uniforms;

@compute @workgroup_size(8, 8)
fn computeFilter(@builtin(global_invocation_id) id: vec3<u32>) {
    var color = vec3<f32>(1., 0., 1.);
    
    textureStore(outputTexture, id.xy, vec4<f32>(color, 1.0));
}
