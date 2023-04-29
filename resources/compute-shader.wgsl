struct Uniforms {
    kernel: mat3x3<f32>,
    test: f32,
    filter_type: u32,
}

@group(0) @binding(0) var inputTexture: texture_2d<f32>;
@group(0) @binding(1) var outputTexture: texture_storage_2d<rgba8unorm,write>;
@group(0) @binding(2) var<uniform> uniforms: Uniforms;

@compute @workgroup_size(8, 8)
fn computeSobelX(@builtin(global_invocation_id) id: vec3<u32>) {
    let color = abs(
          1.0 * textureLoad(inputTexture, vec2<u32>(id.x - 1u, id.y - 1u), 0).rgb
        + 2.0 * textureLoad(inputTexture, vec2<u32>(id.x - 1u, id.y + 0u), 0).rgb
        + 1.0 * textureLoad(inputTexture, vec2<u32>(id.x - 1u, id.y + 1u), 0).rgb
        - 1.0 * textureLoad(inputTexture, vec2<u32>(id.x + 1u, id.y - 1u), 0).rgb
        - 2.0 * textureLoad(inputTexture, vec2<u32>(id.x + 1u, id.y + 0u), 0).rgb
        - 1.0 * textureLoad(inputTexture, vec2<u32>(id.x + 1u, id.y + 1u), 0).rgb
    );
    textureStore(outputTexture, id.xy, vec4<f32>(color, 1.0));
}

fn reduce(v0: vec3<f32>, v1: vec3<f32>, v2: vec3<f32>, v3: vec3<f32>, v4: vec3<f32>, v5: vec3<f32>, v6: vec3<f32>, v7: vec3<f32>, v8: vec3<f32>) -> vec3<f32> {
    switch (uniforms.filter_type) {
        case 0u { // FILTER_TYPE_SUM
            return abs(v0 + v1 + v2 + v3 + v4 + v5 + v6 + v7 + v8);
        }
        case 1u { // FILTER_TYPE_MAXIMUM
            return max(max(max(max(v0, v1), max(v2, v3)), max(max(v4, v5), max(v6, v7))), v8);
        }
        case 2u { // FILTER_TYPE_MINIMUM
            return min(min(min(min(v0, v1), min(v2, v3)), min(min(v4, v5), min(v6, v7))), v8);
        }
        default {
            return vec3<f32>(0.0);
        }
    }
}

@compute @workgroup_size(8, 8)
fn computeFilter(@builtin(global_invocation_id) id: vec3<u32>) {
    let color = reduce(
        uniforms.kernel[0][0] * textureLoad(inputTexture, vec2<u32>(id.x - 1u, id.y - 1u), 0).rgb,
        uniforms.kernel[1][0] * textureLoad(inputTexture, vec2<u32>(id.x - 1u, id.y + 0u), 0).rgb,
        uniforms.kernel[2][0] * textureLoad(inputTexture, vec2<u32>(id.x - 1u, id.y + 1u), 0).rgb,
        uniforms.kernel[0][1] * textureLoad(inputTexture, vec2<u32>(id.x + 0u, id.y - 1u), 0).rgb,
        uniforms.kernel[1][1] * textureLoad(inputTexture, vec2<u32>(id.x + 0u, id.y + 0u), 0).rgb,
        uniforms.kernel[2][1] * textureLoad(inputTexture, vec2<u32>(id.x + 0u, id.y + 1u), 0).rgb,
        uniforms.kernel[0][2] * textureLoad(inputTexture, vec2<u32>(id.x + 1u, id.y - 1u), 0).rgb,
        uniforms.kernel[1][2] * textureLoad(inputTexture, vec2<u32>(id.x + 1u, id.y + 0u), 0).rgb,
        uniforms.kernel[2][2] * textureLoad(inputTexture, vec2<u32>(id.x + 1u, id.y + 1u), 0).rgb
    );
    textureStore(outputTexture, id.xy, vec4<f32>(color, 1.0));
}
