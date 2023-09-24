#include "GpuScene.h"

#include "tiny_gltf.h"

#include <cassert>
#include <unordered_map>

using namespace wgpu;
using namespace tinygltf;

///////////////////////////////////////////////////////////////////////////////
// Public methods

void GpuScene::createFromModel(wgpu::Device device, const tinygltf::Model& model) {
	destroy();

	initDevice(device);
	initBuffers(model);
	initDrawCalls(model);
}

void GpuScene::draw(wgpu::RenderPassEncoder renderPass) {
	for (const DrawCall& dc : m_drawCalls) {
		for (size_t layoutIdx = 0; layoutIdx < dc.attributeBufferViews.size(); ++layoutIdx) {
			const auto& view = dc.attributeBufferViews[layoutIdx];
			uint32_t slot = static_cast<uint32_t>(layoutIdx);
			renderPass.setVertexBuffer(slot, m_buffers[view.buffer], view.byteOffset, view.byteLength);
		}
		renderPass.setIndexBuffer(m_buffers[dc.indexBufferView.buffer], dc.indexFormat, dc.indexBufferView.byteOffset, dc.indexBufferView.byteLength);
		renderPass.drawIndexed(dc.indexCount, 1, 0, 0, 0);
	}
}

void GpuScene::destroy() {
	terminateDrawCalls();
	terminateBuffers();
	terminateDevice();
}

///////////////////////////////////////////////////////////////////////////////
// To be moved in a conversion.cpp file or sth

// Return Undefined if not supported
static VertexFormat vertexFormatFromAccessor(const Accessor& accessor) {
	switch (accessor.componentType) {
	case TINYGLTF_COMPONENT_TYPE_FLOAT:
		if (accessor.normalized) return VertexFormat::Undefined;
		switch (accessor.type) {
		case TINYGLTF_TYPE_VEC2:
			return VertexFormat::Float32x2;
			break;
		case TINYGLTF_TYPE_VEC3:
			return VertexFormat::Float32x3;
			break;
		case TINYGLTF_TYPE_VEC4:
			return VertexFormat::Float32x4;
			break;
		default:
			return VertexFormat::Undefined;
		}
		break;
	case TINYGLTF_COMPONENT_TYPE_INT:
		if (accessor.normalized) return VertexFormat::Undefined;
		switch (accessor.type) {
		case TINYGLTF_TYPE_VEC2:
			return VertexFormat::Sint32x2;
			break;
		case TINYGLTF_TYPE_VEC3:
			return VertexFormat::Sint32x3;
			break;
		case TINYGLTF_TYPE_VEC4:
			return VertexFormat::Sint32x4;
			break;
		default:
			return VertexFormat::Undefined;
		}
		break;
	case TINYGLTF_COMPONENT_TYPE_SHORT:
		switch (accessor.type) {
		case TINYGLTF_TYPE_VEC2:
			return accessor.normalized ? VertexFormat::Snorm16x2 : VertexFormat::Sint16x2;
			break;
		case TINYGLTF_TYPE_VEC4:
			return accessor.normalized ? VertexFormat::Snorm16x4 : VertexFormat::Sint16x4;
			break;
		default:
			return VertexFormat::Undefined;
		}
		break;
	case TINYGLTF_COMPONENT_TYPE_BYTE:
		switch (accessor.type) {
		case TINYGLTF_TYPE_VEC2:
			return accessor.normalized ? VertexFormat::Snorm8x2 : VertexFormat::Sint8x2;
			break;
		case TINYGLTF_TYPE_VEC4:
			return accessor.normalized ? VertexFormat::Snorm8x4 : VertexFormat::Sint8x4;
			break;
		default:
			return VertexFormat::Undefined;
		}
		break;
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
		if (accessor.normalized) return VertexFormat::Undefined;
		switch (accessor.type) {
		case TINYGLTF_TYPE_VEC2:
			return VertexFormat::Uint32x2;
			break;
		case TINYGLTF_TYPE_VEC3:
			return VertexFormat::Uint32x3;
			break;
		case TINYGLTF_TYPE_VEC4:
			return VertexFormat::Uint32x4;
			break;
		default:
			return VertexFormat::Undefined;
		}
		break;
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
		switch (accessor.type) {
		case TINYGLTF_TYPE_VEC2:
			return accessor.normalized ? VertexFormat::Unorm16x2 : VertexFormat::Uint16x2;
			break;
		case TINYGLTF_TYPE_VEC4:
			return accessor.normalized ? VertexFormat::Unorm16x4 : VertexFormat::Uint16x4;
			break;
		default:
			return VertexFormat::Undefined;
		}
		break;
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
		switch (accessor.type) {
		case TINYGLTF_TYPE_VEC2:
			return accessor.normalized ? VertexFormat::Unorm8x2 : VertexFormat::Uint8x2;
			break;
		case TINYGLTF_TYPE_VEC4:
			return accessor.normalized ? VertexFormat::Unorm8x4 : VertexFormat::Uint8x4;
			break;
		default:
			return VertexFormat::Undefined;
		}
		break;
	default:
		return VertexFormat::Undefined;
	}
}

// Return Undefined if not supported
static IndexFormat indexFormatFromAccessor(const Accessor& accessor) {
	switch (accessor.componentType) {
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
		return IndexFormat::Uint16;
		break;
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
		return IndexFormat::Uint32;
		break;
	default:
		return IndexFormat::Undefined;
		break;
	}
}

///////////////////////////////////////////////////////////////////////////////
// Private methods

void GpuScene::initDevice(wgpu::Device device) {
	m_device = device;
	m_device.reference();
	m_queue = m_device.getQueue();
}

void GpuScene::terminateDevice() {
	if (m_queue) {
		m_queue.release();
		m_queue = nullptr;
	}
	if (m_device) {
		m_device.release();
		m_device = nullptr;
	}
}

void GpuScene::initBuffers(const tinygltf::Model& model) {
	for (const tinygltf::Buffer& buffer : model.buffers) {
		BufferDescriptor bufferDesc = Default;
		bufferDesc.label = buffer.name.c_str();
		bufferDesc.size = static_cast<uint32_t>(buffer.data.size());
		bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Vertex | BufferUsage::Index;
		wgpu::Buffer gpuBuffer = m_device.createBuffer(bufferDesc);
		m_buffers.push_back(gpuBuffer);
		m_queue.writeBuffer(gpuBuffer, 0, buffer.data.data(), buffer.data.size());
	}
}

void GpuScene::terminateBuffers() {
	for (wgpu::Buffer b : m_buffers) {
		b.destroy();
		b.release();
	}
	m_buffers.clear();
}

void GpuScene::initDrawCalls(const tinygltf::Model& model) {
	const Mesh& mesh = model.meshes[0];
	const Primitive& prim = mesh.primitives[0];

	std::unordered_map<std::string, uint32_t> semanticToLocation = {
		{"POSITION", 0},
		{"NORMAL", 1},
		{"COLOR", 2},
		{"TEXCOORD_0", 3},
	};

	// We create one vertex buffer layout per bufferView
	std::vector<VertexBufferLayout> vertexBufferLayouts;
	// For each layout, there are multiple attributes
	std::vector<std::vector<VertexAttribute>> vertexBufferLayoutToAttributes;
	// For each layout, there is a single buffer view
	std::vector<tinygltf::BufferView> vertexBufferLayoutToBufferView;
	// And we keep the map from bufferView to layout index
	std::unordered_map<int, size_t> bufferViewToVertexBufferLayout;

	for (const auto& [attrSemantic, accessorIdx] : prim.attributes) {
		auto locationIt = semanticToLocation.find(attrSemantic);
		if (locationIt == semanticToLocation.end()) continue;

		Accessor accessor = model.accessors[accessorIdx];
		BufferView bufferView = model.bufferViews[accessor.bufferView];

		size_t layoutIdx = 0;
		auto vertexBufferLayoutIt = bufferViewToVertexBufferLayout.find(accessor.bufferView);
		if (vertexBufferLayoutIt == bufferViewToVertexBufferLayout.end()) {
			layoutIdx = vertexBufferLayouts.size();
			VertexBufferLayout layout;
			layout.arrayStride = bufferView.byteStride;
			layout.stepMode = VertexStepMode::Vertex;
			vertexBufferLayouts.push_back(layout);
			vertexBufferLayoutToAttributes.push_back({});
			vertexBufferLayoutToBufferView.push_back(bufferView);
			bufferViewToVertexBufferLayout[accessor.bufferView] = layoutIdx;
		}
		else {
			layoutIdx = vertexBufferLayoutIt->second;
		}

		VertexAttribute attrib;
		attrib.shaderLocation = locationIt->second;
		attrib.format = vertexFormatFromAccessor(accessor);
		attrib.offset = accessor.byteOffset;
		vertexBufferLayoutToAttributes[layoutIdx].push_back(attrib);
	}

	// Index data
	const Accessor& indexAccessor = model.accessors[prim.indices];
	const BufferView& indexBufferView = model.bufferViews[indexAccessor.bufferView];
	IndexFormat indexFormat = indexFormatFromAccessor(indexAccessor);
	assert(indexFormat != IndexFormat::Undefined);
	assert(indexAccessor.type == TINYGLTF_TYPE_SCALAR);

	m_drawCalls.push_back(DrawCall{
		vertexBufferLayoutToAttributes,
		vertexBufferLayouts,
		vertexBufferLayoutToBufferView,
		indexBufferView,
		indexFormat,
		static_cast<uint32_t>(indexAccessor.count)
	});
}

void GpuScene::terminateDrawCalls() {
	m_drawCalls.clear();
}

std::vector<wgpu::VertexBufferLayout> GpuScene::vertexBufferLayouts(uint32_t drawCallIndex) const {
	const auto& dc = m_drawCalls[drawCallIndex];
	std::vector<wgpu::VertexBufferLayout> vertexBufferLayouts = dc.vertexBufferLayouts;

	// Assign pointer addresses (should be done upon foreign access)
	for (size_t layoutIdx = 0; layoutIdx < dc.vertexBufferLayouts.size(); ++layoutIdx) {
		VertexBufferLayout& layout = vertexBufferLayouts[layoutIdx];
		const auto& attributes = dc.vertexAttributes[layoutIdx];
		layout.attributeCount = static_cast<uint32_t>(attributes.size());
		layout.attributes = attributes.data();
	}

	return vertexBufferLayouts;
}
