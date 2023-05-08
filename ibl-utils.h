#pragma once

#include <webgpu/webgpu.hpp>

namespace ibl_utils {

/**
 * Create a 16-bit DFG look up table as described in Filament
 * https://google.github.io/filament/Filament.html#toc5.3.4.3
 * Optionnaly create a texture view
 */
wgpu::Texture createDFGTexture(wgpu::Device device, uint32_t size = 256, wgpu::TextureView *view = nullptr);

} // namespace ibl_utils
