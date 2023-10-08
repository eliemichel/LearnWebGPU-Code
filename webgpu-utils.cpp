#include "webgpu-utils.h"

#include <algorithm>

// Equivalent of std::bit_width that is available from C++20 onward
static uint32_t bit_width(uint32_t m) {
	if (m == 0) return 0;
	else { uint32_t w = 0; while (m >>= 1) ++w; return w; }
}

namespace wgpu {

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
		return 0;
	}
}

uint32_t maxMipLevelCount1D(WGPUExtent3D size) {
	(void)size; // Size has indeed no impact
	return 1;
}

uint32_t maxMipLevelCount2D(WGPUExtent3D size) {
	return std::max(1U, bit_width(std::max(size.width, size.height)));
}

uint32_t maxMipLevelCount3D(WGPUExtent3D size) {
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

} // namespace wgpu

