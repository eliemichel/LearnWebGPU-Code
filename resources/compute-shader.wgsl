@group(0) @binding(0) var previousMipLevel: texture_2d<f32>;
@group(0) @binding(1) var nextMipLevel: texture_storage_2d<rgba8unorm,write>;
@group(0) @binding(2) var nextMipLevel_readonly: texture_2d<f32>;

@compute @workgroup_size(8, 8)
fn computeMipMap_OptionA(@builtin(global_invocation_id) id: vec3<u32>) {
    let offset = vec2<u32>(0, 1);
    let color = (
        textureLoad(previousMipLevel, 2 * id.xy + offset.xx, 0) +
        textureLoad(previousMipLevel, 2 * id.xy + offset.xy, 0) +
        textureLoad(previousMipLevel, 2 * id.xy + offset.yx, 0) +
        textureLoad(previousMipLevel, 2 * id.xy + offset.yy, 0)
    ) * 0.25;
    textureStore(nextMipLevel, id.xy, color);
}

@compute @workgroup_size(8, 8)
fn computeMipMap_OptionB(@builtin(global_invocation_id) id: vec3<u32>) {
    let prevCoord = id.xy;
    let nextCoord = id.xy / 2;

    // The value we add to the average
    let prevTexel = textureLoad(previousMipLevel, prevCoord, 0);

    // We load, modify then store the next MIP level to accumulate the average
    var nextTexel = textureLoad(nextMipLevel_readonly, nextCoord, 0);
    nextTexel += prevTexel * 0.25;
    textureStore(nextMipLevel, nextCoord, nextTexel);
}
