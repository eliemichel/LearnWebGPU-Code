struct Uniforms {
    kernel: mat3x3<f32>,
    test: f32,
}

@group(0) @binding(0) var inputTexture: texture_2d<f32>;
@group(0) @binding(1) var outputTexture: texture_storage_2d<rgba8unorm,write>;
@group(0) @binding(2) var<uniform> uniforms: Uniforms;

@compute @workgroup_size(8, 8)
fn computeSobelX(@builtin(global_invocation_id) id: vec3<u32>) {
    let color = abs(
          1 * textureLoad(inputTexture, vec2<u32>(id.x - 1, id.y - 1), 0).rgb
        + 2 * textureLoad(inputTexture, vec2<u32>(id.x - 1, id.y + 0), 0).rgb
        + 1 * textureLoad(inputTexture, vec2<u32>(id.x - 1, id.y + 1), 0).rgb
        - 1 * textureLoad(inputTexture, vec2<u32>(id.x + 1, id.y - 1), 0).rgb
        - 2 * textureLoad(inputTexture, vec2<u32>(id.x + 1, id.y + 0), 0).rgb
        - 1 * textureLoad(inputTexture, vec2<u32>(id.x + 1, id.y + 1), 0).rgb
    );
    textureStore(outputTexture, id.xy, vec4<f32>(color, 1.0));
}

@compute @workgroup_size(8, 8)
fn computeFilter(@builtin(global_invocation_id) id: vec3<u32>) {
    let color = abs(
          uniforms.kernel[0][0] * textureLoad(inputTexture, vec2<u32>(id.x - 1, id.y - 1), 0).rgb
        + uniforms.kernel[1][0] * textureLoad(inputTexture, vec2<u32>(id.x - 1, id.y + 0), 0).rgb
        + uniforms.kernel[2][0] * textureLoad(inputTexture, vec2<u32>(id.x - 1, id.y + 1), 0).rgb
        + uniforms.kernel[0][1] * textureLoad(inputTexture, vec2<u32>(id.x + 0, id.y - 1), 0).rgb
        + uniforms.kernel[1][1] * textureLoad(inputTexture, vec2<u32>(id.x + 0, id.y + 0), 0).rgb
        + uniforms.kernel[2][1] * textureLoad(inputTexture, vec2<u32>(id.x + 0, id.y + 1), 0).rgb
        + uniforms.kernel[0][2] * textureLoad(inputTexture, vec2<u32>(id.x + 1, id.y - 1), 0).rgb
        + uniforms.kernel[1][2] * textureLoad(inputTexture, vec2<u32>(id.x + 1, id.y + 0), 0).rgb
        + uniforms.kernel[2][2] * textureLoad(inputTexture, vec2<u32>(id.x + 1, id.y + 1), 0).rgb
    );
    textureStore(outputTexture, id.xy, vec4<f32>(color, 1.0));
}
