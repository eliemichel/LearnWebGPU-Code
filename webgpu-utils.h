// This is a bunch of utility function imported from https://github.com/eliemichel/WebGPU-utils
#pragma once

#include <webgpu/webgpu.hpp>

namespace wgpu {

size_t vertexFormatByteSize(VertexFormat format);

/**
 * Variant of wgpu::maxMipLevelCount for when one statically knows that the texture
 * dimension is 1D.
 */
uint32_t maxMipLevelCount1D(WGPUExtent3D size);

/**
 * Variant of wgpu::maxMipLevelCount for when one statically knows that the texture
 * dimension is 2D.
 */
uint32_t maxMipLevelCount2D(WGPUExtent3D size);

/**
 * Variant of wgpu::maxMipLevelCount for when one statically knows that the texture
 * dimension is 3D.
 */
uint32_t maxMipLevelCount3D(WGPUExtent3D size);

/**
 * Compute the maximum number of MIP levels for a texture as standardized
 * [in WebGPU spec](https://www.w3.org/TR/webgpu/#abstract-opdef-maximum-miplevel-count).
 *
 * You may specify the dimension either statically in the name of the function
 * or dynamically as a second argument.
 *
 *     textureDesc.mipLevelCount = wgpu::maxMipLevelCount(textureDesc.size, textureDesc.dimension);
 *
 * @param size size of the texture
 *
 * @param dimension of the texture
 *
 * @return maximum value allowed for `mipLevelCount` in a texture descriptor
 *         that has the given size and dimension.
 */
uint32_t maxMipLevelCount(Extent3D size, TextureDimension dimension);

/**
 * Tell how many channels a given texture format has
 *
 * @param format the texel format of the texture
 *
 * @return the number of channels
 */
uint32_t textureFormatChannelCount(TextureFormat format);

/**
 * Tell how many bits each texel occupies for a given texture format has.
 *
 * @param format the texel format of the texture
 *
 * @return the number of bits
 *
 * NB: Some compressed format use less than 1 byte (8 bits) per texel, which
 * is why this function cannot return a byte number instead of bits.
 */
uint32_t textureFormatBitsPerTexel(TextureFormat format);

} // namespace wgpu
