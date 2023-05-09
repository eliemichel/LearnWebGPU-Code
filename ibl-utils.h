#pragma once

#include "filament/Cubemap.h"

#include <webgpu/webgpu.hpp>

#include <glm/glm.hpp>

#include <vector>

namespace ibl_utils {

using SphericalHarmonics = std::vector<glm::vec3>;
using Cubemap = filament::ibl::Cubemap;

/**
 * Create a 16-bit DFG look up table as described in Filament
 * https://google.github.io/filament/Filament.html#toc5.3.4.3
 * Optionnaly create a texture view
 */
wgpu::Texture createDFGTexture(wgpu::Device device, uint32_t size = 256, wgpu::TextureView *view = nullptr);

/**
 * Compute spherical harmonics of the diffuse factor
 */
SphericalHarmonics computeSH(const Cubemap& cm);

} // namespace ibl_utils
