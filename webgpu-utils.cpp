#include "webgpu-utils.h"

#include <cassert>
#include <algorithm>

// Equivalent of std::bit_width that is available in C++20
static uint32_t bit_width(uint32_t m) {
	if (m == 0) return 0;
	else { uint32_t w = 0; while (m >>= 1) ++w; return w; }
}

/**
 * Variant of wgpuMaxMipLevels for when one statically knows that the texture
 * dimension is 1D.
 */
uint32_t wgpuMaxMipLevels1D(WGPUExtent3D size) {
	(void)size; // Size has indeed no impact
	return 1;
}

/**
 * Variant of wgpuMaxMipLevels for when one statically knows that the texture
 * dimension is 2D.
 */
uint32_t wgpuMaxMipLevels2D(WGPUExtent3D size) {
	return bit_width(std::max(size.width, size.height));
}

/**
 * Variant of wgpuMaxMipLevels for when one statically knows that the texture
 * dimension is 3D.
 */
uint32_t wgpuMaxMipLevels3D(WGPUExtent3D size) {
	return bit_width(std::max(size.width, std::max(size.height, size.depthOrArrayLayers)));
}

/**
 * Compute the maximum number of MIP levels for a texture as standardized
 * [in WebGPU spec](https://www.w3.org/TR/webgpu/#abstract-opdef-maximum-miplevel-count).
 * 
 * You may specify the dimension either statically in the name of the function
 * or dynamically as a second argument.
 * 
 *     textureDesc.mipLevelCount = wgpuMaxMipLevels(textureDesc.size, textureDesc.dimension);
 * 
 * @param size size of the texture
 * 
 * @param dimension of the texture
 * 
 * @return maximum value allowed for `mipLevelCount` in a texture descriptor
 *         that has the given size and dimension.
 */
uint32_t wgpuMaxMipLevels(WGPUExtent3D size, WGPUTextureDimension dimension) {
	switch (dimension) {
	case WGPUTextureDimension_1D:
		return wgpuMaxMipLevels1D(size);
	case WGPUTextureDimension_2D:
		return wgpuMaxMipLevels2D(size);
	case WGPUTextureDimension_3D:
		return wgpuMaxMipLevels3D(size);
	default:
		// Should not happen
		assert(false);
		return 0;
	}
}
