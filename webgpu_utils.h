#pragma once

#include <webgpu/webgpu.hpp>

namespace wgpu {

size_t vertexFormatByteSize(VertexFormat format);

uint32_t maximumMipLevelCount(Extent3D size);

} // namespace wgpu
