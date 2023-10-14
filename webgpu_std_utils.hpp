/**
 * This file is part of the "Learn WebGPU for C++" book.
 *   https://github.com/eliemichel/LearnWebGPU
 *
 * MIT License
 * Copyright (c) 2022-2023 Elie Michel
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

 /**
  * This tiny library provides set of unopinionated utility functions that
  * solely depend on `webgpu.hpp` and implement information coming from the
  * WebGPU specification but not directly available in `webgpu.h`.
  *
  * This is a single-file library, for which one must define in **exactly one
  * source file** the preprocessor variable WEBGPU_STD_UTILS_IMPLEMENTATION
  * prior to inclusion:
  *
  *     #define WEBGPU_STD_UTILS_IMPLEMENTATION
  *     #include "webgpu_std_utils.hpp"
  */

#include <webgpu/webgpu.h>

  // Equivalent of std::bit_width that is available from C++20 onward
uint32_t bit_width(uint32_t m);

namespace wgpu {

/**
 * Variant of wgpu::maxMipLevelCount for when one statically knows that the texture
 * dimension is 1D.
 */
uint32_t maxMipLevelCount1D(Extent3D size);

/**
 * Variant of wgpu::maxMipLevelCount for when one statically knows that the texture
 * dimension is 2D.
 */
uint32_t maxMipLevelCount2D(Extent3D size);

/**
 * Variant of wgpu::maxMipLevelCount for when one statically knows that the texture
 * dimension is 3D.
 */
uint32_t maxMipLevelCount3D(Extent3D size);

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

/**
 * Tell how many channels a given texture format has
 *
 * @param format the texel format of the texture
 *
 * @return the number of channels
 */
uint32_t textureFormatChannelCount(TextureFormat format);

/**
 * Indicate what gamma correction must be applied to a linear color prior to
 * writing it in a texture that has the given format.
 *
 * For instance, if your color attachment has the given format, your fragment
 * should end like this (assuming it is operating on linear colors):
 *
 *     let corrected_color = pow(linear_color, vec4f(gamma));
 *     return corrected_color;
 *
 * @param format the format of the texture we write to
 *
 * @return a gamma of 2.2 if the format uses the sRGB scale, 1.0 otherwise
 *         (i.e., it uses a linear scale)
 */
float textureFormatGamma(TextureFormat format);

/**
 * Provide an example of sample type that can be used with the texture
 *
 * @param format the texel format of the texture
 *
 * @return one supported sample type from a given texture format
 *
 * NB: Among the possibilities, UnfilterableFloat is not returned unless it is
 * the only possibility
 *
 * NB: For mixed depth/stencil textures the query is ambiguous because it
 * depends on the aspect. In such a case this function returns Depth.
 */
TextureSampleType textureFormatSupportedSampleType(TextureFormat format);

/**
 * Return a format that is has the same data layout but that can be sampled as
 * Float (basically map int to norm).
 *
 * @param format any format
 *
 * @return a format that is compatible with TextureSampleType::Float and has
 * the same data layout than the input format, or Undefined if not supported.
 */
TextureFormat textureFormatToFloatFormat(TextureFormat format);

/**
 * Tell how many bytes a given vertex format occupies
 *
 * @param format the vertex format
 *
 * @return the number of bytes
 */
size_t vertexFormatByteSize(VertexFormat format);

/**
 * Tell how many bytes a given index format occupies
 *
 * @param format the index format
 *
 * @return the number of bytes
 */
size_t indexFormatByteSize(IndexFormat format);

/**
 * @param value can be any non-negative value
 * 
 * @param alignmentPowOfTwo must be a power of 2
 * 
 * @return the next multiple of 'alignmentPowOfTwo' greater or equal to 'value'
 */
uint32_t alignToNextMultipleOf(uint32_t value, uint32_t alignmentPowOfTwo);

} // namespace wgpu

#ifdef WEBGPU_STD_UTILS_IMPLEMENTATION

#include <algorithm>
#include <cassert>

// Equivalent of std::bit_width that is available from C++20 onward
uint32_t bit_width(uint32_t m) {
	if (m == 0) return 0;
	else { uint32_t w = 0; while (m >>= 1) ++w; return w; }
}

namespace wgpu {

uint32_t maxMipLevelCount1D(Extent3D size) {
	(void)size; // Size has indeed no impact
	return 1;
}

uint32_t maxMipLevelCount2D(Extent3D size) {
	return std::max(1U, bit_width(std::max(size.width, size.height)));
}

uint32_t maxMipLevelCount3D(Extent3D size) {
	return std::max(1U, bit_width(std::max(size.width, std::max(size.height, size.depthOrArrayLayers))));
}

uint32_t maxMipLevelCount(Extent3D size, TextureDimension dimension) {
	switch (dimension) {
	case TextureDimension::_1D:
		return maxMipLevelCount1D(size);
	case TextureDimension::_2D:
		return maxMipLevelCount2D(size);
	case TextureDimension::_3D:
		return maxMipLevelCount3D(size);
	default:
		// Should not happen
		assert(false);
		return 0;
	}
}

uint32_t textureFormatBitsPerTexel(TextureFormat format) {
	switch (format) {
	case TextureFormat::Undefined:
		return 0;
	case TextureFormat::BC1RGBAUnorm:
	case TextureFormat::BC1RGBAUnormSrgb:
	case TextureFormat::BC4RUnorm:
	case TextureFormat::BC4RSnorm:
	case TextureFormat::ETC2RGB8Unorm:
	case TextureFormat::ETC2RGB8UnormSrgb:
	case TextureFormat::ETC2RGB8A1Unorm:
	case TextureFormat::ETC2RGB8A1UnormSrgb:
	case TextureFormat::EACR11Unorm:
	case TextureFormat::EACR11Snorm:
	case TextureFormat::ASTC4x4Unorm:
	case TextureFormat::ASTC4x4UnormSrgb:
	case TextureFormat::ASTC5x4Unorm:
	case TextureFormat::ASTC5x4UnormSrgb:
	case TextureFormat::ASTC5x5Unorm:
	case TextureFormat::ASTC5x5UnormSrgb:
	case TextureFormat::ASTC6x5Unorm:
	case TextureFormat::ASTC6x5UnormSrgb:
	case TextureFormat::ASTC6x6Unorm:
	case TextureFormat::ASTC6x6UnormSrgb:
	case TextureFormat::ASTC8x5Unorm:
	case TextureFormat::ASTC8x5UnormSrgb:
	case TextureFormat::ASTC8x6Unorm:
	case TextureFormat::ASTC8x6UnormSrgb:
	case TextureFormat::ASTC8x8Unorm:
	case TextureFormat::ASTC8x8UnormSrgb:
	case TextureFormat::ASTC10x5Unorm:
	case TextureFormat::ASTC10x5UnormSrgb:
	case TextureFormat::ASTC10x6Unorm:
	case TextureFormat::ASTC10x6UnormSrgb:
	case TextureFormat::ASTC10x8Unorm:
	case TextureFormat::ASTC10x8UnormSrgb:
	case TextureFormat::ASTC10x10Unorm:
	case TextureFormat::ASTC10x10UnormSrgb:
	case TextureFormat::ASTC12x10Unorm:
	case TextureFormat::ASTC12x10UnormSrgb:
	case TextureFormat::ASTC12x12Unorm:
	case TextureFormat::ASTC12x12UnormSrgb:
		return 4;
	case TextureFormat::R8Unorm:
	case TextureFormat::R8Snorm:
	case TextureFormat::R8Uint:
	case TextureFormat::R8Sint:
	case TextureFormat::Stencil8:
	case TextureFormat::BC2RGBAUnorm:
	case TextureFormat::BC2RGBAUnormSrgb:
	case TextureFormat::BC3RGBAUnorm:
	case TextureFormat::BC3RGBAUnormSrgb:
	case TextureFormat::BC5RGUnorm:
	case TextureFormat::BC5RGSnorm:
	case TextureFormat::BC6HRGBUfloat:
	case TextureFormat::BC6HRGBFloat:
	case TextureFormat::BC7RGBAUnorm:
	case TextureFormat::BC7RGBAUnormSrgb:
	case TextureFormat::ETC2RGBA8Unorm:
	case TextureFormat::ETC2RGBA8UnormSrgb:
	case TextureFormat::EACRG11Unorm:
	case TextureFormat::EACRG11Snorm:
		return 8;
	case TextureFormat::R16Uint:
	case TextureFormat::R16Sint:
	case TextureFormat::R16Float:
	case TextureFormat::RG8Unorm:
	case TextureFormat::RG8Snorm:
	case TextureFormat::RG8Uint:
	case TextureFormat::RG8Sint:
	case TextureFormat::Depth16Unorm:
		return 16;
	case TextureFormat::Depth24Plus:
		return 24;
	case TextureFormat::R32Float:
	case TextureFormat::R32Uint:
	case TextureFormat::R32Sint:
	case TextureFormat::RG16Uint:
	case TextureFormat::RG16Sint:
	case TextureFormat::RG16Float:
	case TextureFormat::RGBA8Unorm:
	case TextureFormat::RGBA8UnormSrgb:
	case TextureFormat::RGBA8Snorm:
	case TextureFormat::RGBA8Uint:
	case TextureFormat::RGBA8Sint:
	case TextureFormat::BGRA8Unorm:
	case TextureFormat::BGRA8UnormSrgb:
	case TextureFormat::Depth32Float:
	case TextureFormat::Depth24PlusStencil8:
	case TextureFormat::RG11B10Ufloat:
	case TextureFormat::RGB9E5Ufloat:
	case TextureFormat::RGB10A2Unorm:
		return 32;
	case TextureFormat::Depth32FloatStencil8:
		return 40;
	case TextureFormat::RG32Float:
	case TextureFormat::RG32Uint:
	case TextureFormat::RG32Sint:
	case TextureFormat::RGBA16Uint:
	case TextureFormat::RGBA16Sint:
	case TextureFormat::RGBA16Float:
		return 64;
	case TextureFormat::RGBA32Float:
	case TextureFormat::RGBA32Uint:
	case TextureFormat::RGBA32Sint:
		return 128;
	default:
		throw std::runtime_error("Unhandled format");
	}
}

uint32_t textureFormatChannelCount(TextureFormat format) {
	switch (format) {
	case TextureFormat::Undefined:
		return 0;
	case TextureFormat::R8Unorm:
	case TextureFormat::R8Snorm:
	case TextureFormat::R8Uint:
	case TextureFormat::R8Sint:
	case TextureFormat::R16Uint:
	case TextureFormat::R16Sint:
	case TextureFormat::R16Float:
	case TextureFormat::R32Float:
	case TextureFormat::R32Uint:
	case TextureFormat::R32Sint:
	case TextureFormat::Stencil8:
	case TextureFormat::Depth16Unorm:
	case TextureFormat::Depth24Plus:
	case TextureFormat::Depth32Float:
	case TextureFormat::BC4RUnorm:
	case TextureFormat::BC4RSnorm:
	case TextureFormat::EACR11Unorm:
	case TextureFormat::EACR11Snorm:
		return 1;
	case TextureFormat::RG8Unorm:
	case TextureFormat::RG8Snorm:
	case TextureFormat::RG8Uint:
	case TextureFormat::RG8Sint:
	case TextureFormat::RG16Uint:
	case TextureFormat::RG16Sint:
	case TextureFormat::RG16Float:
	case TextureFormat::RG32Float:
	case TextureFormat::RG32Uint:
	case TextureFormat::RG32Sint:
	case TextureFormat::Depth24PlusStencil8:
	case TextureFormat::Depth32FloatStencil8:
	case TextureFormat::BC5RGUnorm:
	case TextureFormat::BC5RGSnorm:
	case TextureFormat::EACRG11Unorm:
	case TextureFormat::EACRG11Snorm:
		return 2;
	case TextureFormat::RG11B10Ufloat:
	case TextureFormat::RGB9E5Ufloat:
	case TextureFormat::BC6HRGBUfloat:
	case TextureFormat::BC6HRGBFloat:
	case TextureFormat::ETC2RGB8Unorm:
	case TextureFormat::ETC2RGB8UnormSrgb:
		return 3;
	case TextureFormat::RGBA8Unorm:
	case TextureFormat::RGBA8UnormSrgb:
	case TextureFormat::RGBA8Snorm:
	case TextureFormat::RGBA8Uint:
	case TextureFormat::RGBA8Sint:
	case TextureFormat::BGRA8Unorm:
	case TextureFormat::BGRA8UnormSrgb:
	case TextureFormat::RGB10A2Unorm:
	case TextureFormat::RGBA16Uint:
	case TextureFormat::RGBA16Sint:
	case TextureFormat::RGBA16Float:
	case TextureFormat::RGBA32Float:
	case TextureFormat::RGBA32Uint:
	case TextureFormat::RGBA32Sint:
	case TextureFormat::BC1RGBAUnorm:
	case TextureFormat::BC1RGBAUnormSrgb:
	case TextureFormat::BC2RGBAUnorm:
	case TextureFormat::BC2RGBAUnormSrgb:
	case TextureFormat::BC3RGBAUnorm:
	case TextureFormat::BC3RGBAUnormSrgb:
	case TextureFormat::BC7RGBAUnorm:
	case TextureFormat::BC7RGBAUnormSrgb:
	case TextureFormat::ETC2RGB8A1Unorm:
	case TextureFormat::ETC2RGB8A1UnormSrgb:
	case TextureFormat::ETC2RGBA8Unorm:
	case TextureFormat::ETC2RGBA8UnormSrgb:
	case TextureFormat::ASTC4x4Unorm:
	case TextureFormat::ASTC4x4UnormSrgb:
	case TextureFormat::ASTC5x4Unorm:
	case TextureFormat::ASTC5x4UnormSrgb:
	case TextureFormat::ASTC5x5Unorm:
	case TextureFormat::ASTC5x5UnormSrgb:
	case TextureFormat::ASTC6x5Unorm:
	case TextureFormat::ASTC6x5UnormSrgb:
	case TextureFormat::ASTC6x6Unorm:
	case TextureFormat::ASTC6x6UnormSrgb:
	case TextureFormat::ASTC8x5Unorm:
	case TextureFormat::ASTC8x5UnormSrgb:
	case TextureFormat::ASTC8x6Unorm:
	case TextureFormat::ASTC8x6UnormSrgb:
	case TextureFormat::ASTC8x8Unorm:
	case TextureFormat::ASTC8x8UnormSrgb:
	case TextureFormat::ASTC10x5Unorm:
	case TextureFormat::ASTC10x5UnormSrgb:
	case TextureFormat::ASTC10x6Unorm:
	case TextureFormat::ASTC10x6UnormSrgb:
	case TextureFormat::ASTC10x8Unorm:
	case TextureFormat::ASTC10x8UnormSrgb:
	case TextureFormat::ASTC10x10Unorm:
	case TextureFormat::ASTC10x10UnormSrgb:
	case TextureFormat::ASTC12x10Unorm:
	case TextureFormat::ASTC12x10UnormSrgb:
	case TextureFormat::ASTC12x12Unorm:
	case TextureFormat::ASTC12x12UnormSrgb:
		return 4;
	default:
		throw std::runtime_error("Unhandled format");
	}
}

float textureFormatGamma(TextureFormat format) {
	switch (format)
	{
	case TextureFormat::ASTC10x10UnormSrgb:
	case TextureFormat::ASTC10x5UnormSrgb:
	case TextureFormat::ASTC10x6UnormSrgb:
	case TextureFormat::ASTC10x8UnormSrgb:
	case TextureFormat::ASTC12x10UnormSrgb:
	case TextureFormat::ASTC12x12UnormSrgb:
	case TextureFormat::ASTC4x4UnormSrgb:
	case TextureFormat::ASTC5x5UnormSrgb:
	case TextureFormat::ASTC6x5UnormSrgb:
	case TextureFormat::ASTC6x6UnormSrgb:
	case TextureFormat::ASTC8x5UnormSrgb:
	case TextureFormat::ASTC8x6UnormSrgb:
	case TextureFormat::ASTC8x8UnormSrgb:
	case TextureFormat::BC1RGBAUnormSrgb:
	case TextureFormat::BC2RGBAUnormSrgb:
	case TextureFormat::BC3RGBAUnormSrgb:
	case TextureFormat::BC7RGBAUnormSrgb:
	case TextureFormat::BGRA8UnormSrgb:
	case TextureFormat::ETC2RGB8A1UnormSrgb:
	case TextureFormat::ETC2RGB8UnormSrgb:
	case TextureFormat::ETC2RGBA8UnormSrgb:
	case TextureFormat::RGBA8UnormSrgb:
		return 2.2f;
	default:
		return 1.0f;
	}
}

TextureSampleType textureFormatSupportedSampleType(TextureFormat format) {
	switch (format) {
	case TextureFormat::Undefined:
		return TextureSampleType::Undefined;
	case TextureFormat::BC1RGBAUnorm:
	case TextureFormat::BC1RGBAUnormSrgb:
	case TextureFormat::BC4RUnorm:
	case TextureFormat::BC4RSnorm:
	case TextureFormat::ETC2RGB8Unorm:
	case TextureFormat::ETC2RGB8UnormSrgb:
	case TextureFormat::ETC2RGB8A1Unorm:
	case TextureFormat::ETC2RGB8A1UnormSrgb:
	case TextureFormat::EACR11Unorm:
	case TextureFormat::EACR11Snorm:
	case TextureFormat::ASTC4x4Unorm:
	case TextureFormat::ASTC4x4UnormSrgb:
	case TextureFormat::ASTC5x4Unorm:
	case TextureFormat::ASTC5x4UnormSrgb:
	case TextureFormat::ASTC5x5Unorm:
	case TextureFormat::ASTC5x5UnormSrgb:
	case TextureFormat::ASTC6x5Unorm:
	case TextureFormat::ASTC6x5UnormSrgb:
	case TextureFormat::ASTC6x6Unorm:
	case TextureFormat::ASTC6x6UnormSrgb:
	case TextureFormat::ASTC8x5Unorm:
	case TextureFormat::ASTC8x5UnormSrgb:
	case TextureFormat::ASTC8x6Unorm:
	case TextureFormat::ASTC8x6UnormSrgb:
	case TextureFormat::ASTC8x8Unorm:
	case TextureFormat::ASTC8x8UnormSrgb:
	case TextureFormat::ASTC10x5Unorm:
	case TextureFormat::ASTC10x5UnormSrgb:
	case TextureFormat::ASTC10x6Unorm:
	case TextureFormat::ASTC10x6UnormSrgb:
	case TextureFormat::ASTC10x8Unorm:
	case TextureFormat::ASTC10x8UnormSrgb:
	case TextureFormat::ASTC10x10Unorm:
	case TextureFormat::ASTC10x10UnormSrgb:
	case TextureFormat::ASTC12x10Unorm:
	case TextureFormat::ASTC12x10UnormSrgb:
	case TextureFormat::ASTC12x12Unorm:
	case TextureFormat::ASTC12x12UnormSrgb:
	case TextureFormat::R8Unorm:
	case TextureFormat::R8Snorm:
	case TextureFormat::BC2RGBAUnorm:
	case TextureFormat::BC2RGBAUnormSrgb:
	case TextureFormat::BC3RGBAUnorm:
	case TextureFormat::BC3RGBAUnormSrgb:
	case TextureFormat::BC5RGUnorm:
	case TextureFormat::BC5RGSnorm:
	case TextureFormat::BC6HRGBUfloat:
	case TextureFormat::BC6HRGBFloat:
	case TextureFormat::BC7RGBAUnorm:
	case TextureFormat::BC7RGBAUnormSrgb:
	case TextureFormat::ETC2RGBA8Unorm:
	case TextureFormat::ETC2RGBA8UnormSrgb:
	case TextureFormat::EACRG11Unorm:
	case TextureFormat::EACRG11Snorm:
	case TextureFormat::R16Float:
	case TextureFormat::RG8Unorm:
	case TextureFormat::RG8Snorm:
	case TextureFormat::RG16Float:
	case TextureFormat::RGBA8Unorm:
	case TextureFormat::RGBA8UnormSrgb:
	case TextureFormat::RGBA8Snorm:
	case TextureFormat::BGRA8Unorm:
	case TextureFormat::BGRA8UnormSrgb:
	case TextureFormat::RG11B10Ufloat:
	case TextureFormat::RGB9E5Ufloat:
	case TextureFormat::RGB10A2Unorm:
	case TextureFormat::RGBA16Float:
		return TextureSampleType::Float;
	case TextureFormat::R8Uint:
	case TextureFormat::R16Uint:
	case TextureFormat::RG8Uint:
	case TextureFormat::R32Uint:
	case TextureFormat::RG16Uint:
	case TextureFormat::RGBA8Uint:
	case TextureFormat::RG32Uint:
	case TextureFormat::RGBA16Uint:
	case TextureFormat::RGBA32Uint:
	case TextureFormat::Stencil8:
		return TextureSampleType::Uint;
	case TextureFormat::R8Sint:
	case TextureFormat::R16Sint:
	case TextureFormat::RG8Sint:
	case TextureFormat::R32Sint:
	case TextureFormat::RG16Sint:
	case TextureFormat::RGBA8Sint:
	case TextureFormat::RG32Sint:
	case TextureFormat::RGBA16Sint:
	case TextureFormat::RGBA32Sint:
		return TextureSampleType::Sint;
	case TextureFormat::Depth16Unorm:
	case TextureFormat::Depth24Plus:
	case TextureFormat::Depth32Float:
	case TextureFormat::Depth24PlusStencil8:
	case TextureFormat::Depth32FloatStencil8:
		return TextureSampleType::Depth;
	case TextureFormat::R32Float:
	case TextureFormat::RG32Float:
	case TextureFormat::RGBA32Float:
		return TextureSampleType::UnfilterableFloat;
	default:
		throw std::runtime_error("Unhandled format");
	}
}

TextureFormat textureFormatToFloatFormat(TextureFormat format) {
	switch (format) {
	case TextureFormat::R8Sint:
		return TextureFormat::R8Snorm;
	case TextureFormat::R8Uint:
		return TextureFormat::R8Unorm;
	case TextureFormat::RG8Sint:
		return TextureFormat::RG8Snorm;
	case TextureFormat::RG8Uint:
		return TextureFormat::RG8Unorm;
	case TextureFormat::RGBA8Sint:
		return TextureFormat::RGBA8Snorm;
	case TextureFormat::RGBA8Uint:
		return TextureFormat::RGBA8Unorm;
	case TextureFormat::R16Sint:
	case TextureFormat::R16Uint:
	case TextureFormat::R32Sint:
	case TextureFormat::R32Uint:
	case TextureFormat::RG16Sint:
	case TextureFormat::RG16Uint:
	case TextureFormat::RG32Sint:
	case TextureFormat::RG32Uint:
	case TextureFormat::RGBA16Sint:
	case TextureFormat::RGBA16Uint:
	case TextureFormat::RGBA32Sint:
	case TextureFormat::RGBA32Uint:
	case TextureFormat::Stencil8:
		// no I/Unorm counterpart exists
		return TextureFormat::Undefined;
	default:
		// format is already float
		return format;
	}
}

size_t vertexFormatByteSize(VertexFormat format) {
	switch (format) {
	case VertexFormat::Sint8x2:
	case VertexFormat::Uint8x2:
	case VertexFormat::Snorm8x2:
	case VertexFormat::Unorm8x2:
		return 8 / 8 * 2;
	case VertexFormat::Sint8x4:
	case VertexFormat::Uint8x4:
	case VertexFormat::Snorm8x4:
	case VertexFormat::Unorm8x4:
		return 8 / 8 * 4;
	case VertexFormat::Float16x2:
	case VertexFormat::Sint16x2:
	case VertexFormat::Uint16x2:
	case VertexFormat::Snorm16x2:
	case VertexFormat::Unorm16x2:
		return 16 / 8 * 2;
	case VertexFormat::Float16x4:
	case VertexFormat::Sint16x4:
	case VertexFormat::Uint16x4:
	case VertexFormat::Snorm16x4:
	case VertexFormat::Unorm16x4:
		return 16 / 8 * 4;
	case VertexFormat::Float32x2:
	case VertexFormat::Sint32x2:
	case VertexFormat::Uint32x2:
		return 32 / 8 * 2;
	case VertexFormat::Float32x3:
	case VertexFormat::Sint32x3:
	case VertexFormat::Uint32x3:
		return 32 / 8 * 3;
	case VertexFormat::Float32x4:
	case VertexFormat::Sint32x4:
	case VertexFormat::Uint32x4:
		return 32 / 8 * 4;
	default:
		throw std::runtime_error("Unhandled format");
	}
}

size_t indexFormatByteSize(IndexFormat format) {
	switch (format) {
	case IndexFormat::Uint16:
		return 8 / 8 * 2;
	case IndexFormat::Uint32:
		return 8 / 8 * 4;
	default:
		throw std::runtime_error("Unhandled format");
	}
}

uint32_t alignToNextMultipleOf(uint32_t value, uint32_t alignmentPowOfTwo) {
	assert((alignmentPowOfTwo & (alignmentPowOfTwo - 1)) == 0); // must be a power of 2
	uint32_t bits = alignmentPowOfTwo - 1;
	return (value + bits) & ~bits;
}

} // namespace wgpu

#endif // WEBGPU_STD_UTILS_IMPLEMENTATION
