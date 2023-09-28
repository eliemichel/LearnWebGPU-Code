#include "webgpu_utils.h"

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

uint32_t maximumMipLevelCount(Extent3D size) {
	// TODO
}

} // namespace wgpu

