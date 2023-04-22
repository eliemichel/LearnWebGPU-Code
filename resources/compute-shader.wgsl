@group(0) @binding(0) var inputTexture: texture_2d<f32>;
@group(0) @binding(1) var outputTexture: texture_storage_2d<rgba8unorm,write>;

@compute @workgroup_size(8, 8)
fn computeMipMap(@builtin(global_invocation_id) id: vec3<u32>) {
    let offset = vec3<i32>(-1, 0, 1);
    let coord = vec2<i32>(id.xy);
    let color = abs(
          1 * textureLoad(inputTexture, vec2<u32>(coord + offset.xx), 0).rgb
        + 2 * textureLoad(inputTexture, vec2<u32>(coord + offset.xy), 0).rgb
        + 1 * textureLoad(inputTexture, vec2<u32>(coord + offset.xz), 0).rgb
        - 1 * textureLoad(inputTexture, vec2<u32>(coord + offset.zx), 0).rgb
        - 2 * textureLoad(inputTexture, vec2<u32>(coord + offset.zy), 0).rgb
        - 1 * textureLoad(inputTexture, vec2<u32>(coord + offset.zz), 0).rgb
    );
    textureStore(outputTexture, id.xy, vec4<f32>(color, 1.0));
}
