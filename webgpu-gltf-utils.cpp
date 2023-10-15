#include "webgpu-gltf-utils.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

namespace wgpu::gltf {

using namespace tinygltf;

VertexFormat vertexFormatFromAccessor(const Accessor& accessor) {
	switch (accessor.componentType) {
	case TINYGLTF_COMPONENT_TYPE_FLOAT:
		if (accessor.normalized) return VertexFormat::Undefined;
		switch (accessor.type) {
		case TINYGLTF_TYPE_VEC2:
			return VertexFormat::Float32x2;
		case TINYGLTF_TYPE_VEC3:
			return VertexFormat::Float32x3;
		case TINYGLTF_TYPE_VEC4:
			return VertexFormat::Float32x4;
		default:
			return VertexFormat::Undefined;
		}
		break;
	case TINYGLTF_COMPONENT_TYPE_INT:
		if (accessor.normalized) return VertexFormat::Undefined;
		switch (accessor.type) {
		case TINYGLTF_TYPE_VEC2:
			return VertexFormat::Sint32x2;
		case TINYGLTF_TYPE_VEC3:
			return VertexFormat::Sint32x3;
		case TINYGLTF_TYPE_VEC4:
			return VertexFormat::Sint32x4;
		default:
			return VertexFormat::Undefined;
		}
		break;
	case TINYGLTF_COMPONENT_TYPE_SHORT:
		switch (accessor.type) {
		case TINYGLTF_TYPE_VEC2:
			return accessor.normalized ? VertexFormat::Snorm16x2 : VertexFormat::Sint16x2;
		case TINYGLTF_TYPE_VEC4:
			return accessor.normalized ? VertexFormat::Snorm16x4 : VertexFormat::Sint16x4;
		default:
			return VertexFormat::Undefined;
		}
		break;
	case TINYGLTF_COMPONENT_TYPE_BYTE:
		switch (accessor.type) {
		case TINYGLTF_TYPE_VEC2:
			return accessor.normalized ? VertexFormat::Snorm8x2 : VertexFormat::Sint8x2;
		case TINYGLTF_TYPE_VEC4:
			return accessor.normalized ? VertexFormat::Snorm8x4 : VertexFormat::Sint8x4;
		default:
			return VertexFormat::Undefined;
		}
		break;
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
		if (accessor.normalized) return VertexFormat::Undefined;
		switch (accessor.type) {
		case TINYGLTF_TYPE_VEC2:
			return VertexFormat::Uint32x2;
		case TINYGLTF_TYPE_VEC3:
			return VertexFormat::Uint32x3;
		case TINYGLTF_TYPE_VEC4:
			return VertexFormat::Uint32x4;
		default:
			return VertexFormat::Undefined;
		}
		break;
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
		switch (accessor.type) {
		case TINYGLTF_TYPE_VEC2:
			return accessor.normalized ? VertexFormat::Unorm16x2 : VertexFormat::Uint16x2;
		case TINYGLTF_TYPE_VEC4:
			return accessor.normalized ? VertexFormat::Unorm16x4 : VertexFormat::Uint16x4;
		default:
			return VertexFormat::Undefined;
		}
		break;
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
		switch (accessor.type) {
		case TINYGLTF_TYPE_VEC2:
			return accessor.normalized ? VertexFormat::Unorm8x2 : VertexFormat::Uint8x2;
		case TINYGLTF_TYPE_VEC4:
			return accessor.normalized ? VertexFormat::Unorm8x4 : VertexFormat::Uint8x4;
		default:
			return VertexFormat::Undefined;
		}
		break;
	default:
		return VertexFormat::Undefined;
	}
}

IndexFormat indexFormatFromAccessor(const Accessor& accessor) {
	switch (accessor.componentType) {
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
		return IndexFormat::Uint16;
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
		return IndexFormat::Uint32;
	default:
		return IndexFormat::Undefined;
	}
}

FilterMode filterModeFromGltf(int tinygltfFilter) {
	switch (tinygltfFilter) {
	case -1: // no filter was provided, arbitrary default value
		return FilterMode::Linear;
	case TINYGLTF_TEXTURE_FILTER_LINEAR:
	case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
	case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
		return FilterMode::Linear;
	case TINYGLTF_TEXTURE_FILTER_NEAREST:
	case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
	case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
		return FilterMode::Nearest;
	default:
		return FilterMode::Force32;
	}
}

MipmapFilterMode mipmapFilterModeFromGltf(int tinygltfFilter) {
	switch (tinygltfFilter) {
	case -1: // no filter was provided, arbitrary default value
		return MipmapFilterMode::Linear;
	case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
	case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
		return MipmapFilterMode::Linear;
	case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
	case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
		return MipmapFilterMode::Nearest;
	default:
		return MipmapFilterMode::Force32;
	}
}

AddressMode addressModeFromGltf(int tinygltfWrap) {
	switch (tinygltfWrap) {
	case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
		return AddressMode::ClampToEdge;
	case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
		return AddressMode::MirrorRepeat;
	case TINYGLTF_TEXTURE_WRAP_REPEAT:
		return AddressMode::Repeat;
	default:
		return AddressMode::Force32;
	}
}

TextureFormat textureFormatFromGltfImage(const tinygltf::Image& image, bool useClosestMatch) {
	switch (image.pixel_type) {
	case TINYGLTF_COMPONENT_TYPE_BYTE:
		switch (image.component) {
		case 1:
			return TextureFormat::R8Sint;
		case 2:
			return TextureFormat::RG8Sint;
		case 3:
			return useClosestMatch ? TextureFormat::RGBA8Sint : TextureFormat::Undefined;
		case 4:
			return TextureFormat::RGBA8Sint;
		default:
			return TextureFormat::Undefined;
		}
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
		switch (image.component) {
		case 1:
			return TextureFormat::R8Uint;
		case 2:
			return TextureFormat::RG8Uint;
		case 3:
			return useClosestMatch ? TextureFormat::RGBA8Uint : TextureFormat::Undefined;
		case 4:
			return TextureFormat::RGBA8Uint;
		default:
			return TextureFormat::Undefined;
		}
	case TINYGLTF_COMPONENT_TYPE_SHORT:
		switch (image.component) {
		case 1:
			return TextureFormat::R16Sint;
		case 2:
			return TextureFormat::RG16Sint;
		case 3:
			return useClosestMatch ? TextureFormat::RGBA16Sint : TextureFormat::Undefined;
		case 4:
			return TextureFormat::RGBA16Sint;
		default:
			return TextureFormat::Undefined;
		}
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
		switch (image.component) {
		case 1:
			return TextureFormat::R16Uint;
		case 2:
			return TextureFormat::RG16Uint;
		case 3:
			return useClosestMatch ? TextureFormat::RGBA16Uint : TextureFormat::Undefined;
		case 4:
			return TextureFormat::RGBA16Uint;
		default:
			return TextureFormat::Undefined;
		}
	case TINYGLTF_COMPONENT_TYPE_INT:
		switch (image.component) {
		case 1:
			return TextureFormat::R32Sint;
		case 2:
			return TextureFormat::RG32Sint;
		case 3:
			return useClosestMatch ? TextureFormat::RGBA32Sint : TextureFormat::Undefined;
		case 4:
			return TextureFormat::RGBA32Sint;
		default:
			return TextureFormat::Undefined;
		}
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
		switch (image.component) {
		case 1:
			return TextureFormat::R32Uint;
		case 2:
			return TextureFormat::RG32Uint;
		case 3:
			return useClosestMatch ? TextureFormat::RGBA32Uint : TextureFormat::Undefined;
		case 4:
			return TextureFormat::RGBA32Uint;
		default:
			return TextureFormat::Undefined;
		}
	case TINYGLTF_COMPONENT_TYPE_FLOAT:
		switch (image.component) {
		case 1:
			return TextureFormat::R32Float;
		case 2:
			return TextureFormat::RG32Float;
		case 3:
			return useClosestMatch ? TextureFormat::RGBA32Float : TextureFormat::Undefined;
		case 4:
			return TextureFormat::RGBA32Float;
		default:
			return TextureFormat::Undefined;
		}
	default:
		return TextureFormat::Undefined;
	}
}

glm::mat4 nodeMatrix(const tinygltf::Node& node) {
	if (!node.matrix.empty()) {
		return glm::make_mat4(node.matrix.data());
		
	}
	else {
		const glm::vec3 T =
			node.translation.empty()
			? glm::vec3(0.0)
			: glm::make_vec3(node.translation.data());

		const glm::quat R =
			node.rotation.empty()
			? glm::quat()
			: glm::make_quat(node.rotation.data());

		const glm::vec3 S =
			node.scale.empty()
			? glm::vec3(1.0)
			: glm::make_vec3(node.scale.data());

		return glm::translate(glm::mat4(1.0), T) * glm::toMat4(R) * glm::scale(glm::mat4(1.0), S);
	}
}

PrimitiveTopology primitiveTopologyFromGltf(const tinygltf::Primitive& prim) {
	switch (prim.mode) {
	case TINYGLTF_MODE_LINE:
		return PrimitiveTopology::LineList;
	case TINYGLTF_MODE_LINE_LOOP:
		return PrimitiveTopology::Force32;
	case TINYGLTF_MODE_LINE_STRIP:
		return PrimitiveTopology::LineStrip;
	case TINYGLTF_MODE_POINTS:
		return PrimitiveTopology::PointList;
	case TINYGLTF_MODE_TRIANGLES:
		return PrimitiveTopology::TriangleList;
	case TINYGLTF_MODE_TRIANGLE_FAN:
		return PrimitiveTopology::Force32;
	case TINYGLTF_MODE_TRIANGLE_STRIP:
		return PrimitiveTopology::TriangleStrip;
	default:
		return PrimitiveTopology::Force32;
	}
}

} // namespace wgpu::gltf
