/**
 * This is a set of utility functions coming from
 *   https://github.com/eliemichel/WebGPU-utils
 */
#pragma once

#include <webgpu/webgpu.h>

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
uint32_t wgpuMaxMipLevels(WGPUExtent3D size, WGPUTextureDimension dimension);

/**
 * Variant of wgpuMaxMipLevels for when one statically knows that the texture
 * dimension is 1D.
 */
uint32_t wgpuMaxMipLevels1D(WGPUExtent3D size);

/**
 * Variant of wgpuMaxMipLevels for when one statically knows that the texture
 * dimension is 2D.
 */
uint32_t wgpuMaxMipLevels2D(WGPUExtent3D size);

/**
 * Variant of wgpuMaxMipLevels for when one statically knows that the texture
 * dimension is 3D.
 */
uint32_t wgpuMaxMipLevels3D(WGPUExtent3D size);
