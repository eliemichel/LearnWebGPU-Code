@group(0) @binding(0) var previousMipLevel: texture_2d<f32>;
@group(0) @binding(1) var nextMipLevel: texture_storage_2d<rgba8unorm,write>;

@compute @workgroup_size(8, 8)
fn computeMipMap(@builtin(global_invocation_id) id: vec3<u32>) {
    let offset = vec2<u32>(0u, 1u);
    let color = (
        textureLoad(previousMipLevel, 2u * id.xy + offset.xx, 0) +
        textureLoad(previousMipLevel, 2u * id.xy + offset.xy, 0) +
        textureLoad(previousMipLevel, 2u * id.xy + offset.yx, 0) +
        textureLoad(previousMipLevel, 2u * id.xy + offset.yy, 0)
    ) * 0.25;
    textureStore(nextMipLevel, id.xy, color);
}
