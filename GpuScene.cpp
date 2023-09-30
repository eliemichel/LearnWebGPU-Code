#include "GpuScene.h"
#include "webgpu-utils.h"
#include "webgpu-gltf-utils.h"

#include "tiny_gltf.h"

#include <cassert>
#include <unordered_map>

using namespace wgpu;
using namespace tinygltf;
using namespace wgpu::gltf; // conversion utils

///////////////////////////////////////////////////////////////////////////////
// Public methods

void GpuScene::createFromModel(wgpu::Device device, const tinygltf::Model& model) {
	destroy();

	initDevice(device);
	initBuffers(model);
	initTextures(model);
	initSamplers(model);
	initDrawCalls(model);
}

void GpuScene::draw(wgpu::RenderPassEncoder renderPass) {
	for (const DrawCall& dc : m_drawCalls) {
		for (size_t layoutIdx = 0; layoutIdx < dc.attributeBufferViews.size(); ++layoutIdx) {
			const auto& view = dc.attributeBufferViews[layoutIdx];
			uint32_t slot = static_cast<uint32_t>(layoutIdx);
			if (view.buffer != -1) {
				renderPass.setVertexBuffer(slot, m_buffers[view.buffer], view.byteOffset, view.byteLength);
			}
			else {
				renderPass.setVertexBuffer(slot, m_nullBuffer, 0, 4 * sizeof(float));
			}
		}
		renderPass.setIndexBuffer(m_buffers[dc.indexBufferView.buffer], dc.indexFormat, dc.indexBufferView.byteOffset, dc.indexBufferView.byteLength);
		renderPass.drawIndexed(dc.indexCount, 1, 0, 0, 0);
	}
}

void GpuScene::destroy() {
	terminateDrawCalls();
	terminateSamplers();
	terminateTextures();
	terminateBuffers();
	terminateDevice();
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

	{
		BufferDescriptor bufferDesc = Default;
		bufferDesc.label = "Null Buffer";
		bufferDesc.size = static_cast<uint32_t>(4 * sizeof(float));
		bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Vertex | BufferUsage::Index;
		m_nullBuffer = m_device.createBuffer(bufferDesc);
	}
}

void GpuScene::terminateBuffers() {
	for (wgpu::Buffer b : m_buffers) {
		b.destroy();
		b.release();
	}
	m_buffers.clear();

	if (m_nullBuffer) {
		m_nullBuffer.destroy();
		m_nullBuffer.release();
	}
	m_nullBuffer = nullptr;
}

void GpuScene::initTextures(const tinygltf::Model& model) {
	TextureDescriptor desc;
	for (const tinygltf::Image& image : model.images) {
		desc.label = image.name.c_str();
		desc.dimension = TextureDimension::_2D;
		desc.format = textureFormatFromGltfImage(image);
		desc.sampleCount = 1;
		desc.size = { static_cast<uint32_t>(image.width) , static_cast<uint32_t>(image.height), 1};
		desc.mipLevelCount = maxMipLevelCount2D(desc.size);
		desc.usage = TextureUsage::CopyDst | TextureUsage::TextureBinding;
		desc.viewFormatCount = 0;
		desc.viewFormats = nullptr;
		wgpu::Texture gpuTexture = m_device.createTexture(desc);
		m_textures.push_back(gpuTexture);

		// Upload
		ImageCopyTexture destination;
		destination.aspect = TextureAspect::All;
		destination.mipLevel = 0;
		destination.origin = { 0, 0, 0 };
		destination.texture = gpuTexture;
		TextureDataLayout sourceLayout;
		sourceLayout.offset = 0;
		uint32_t bitsPerPixel = textureFormatBitsPerTexel(desc.format);
		sourceLayout.bytesPerRow = bitsPerPixel * desc.size.width / 8;
		sourceLayout.rowsPerImage = desc.size.height;
		m_queue.writeTexture(destination, image.image.data(), image.image.size(), sourceLayout, desc.size);
	}
}

void GpuScene::terminateTextures() {
	for (wgpu::Texture t : m_textures) {
		t.destroy();
		t.release();
	}
	m_textures.clear();
}

void GpuScene::initSamplers(const tinygltf::Model& model) {
	SamplerDescriptor desc;
	for (const tinygltf::Sampler& sampler : model.samplers) {
		desc.label = sampler.name.c_str();
		desc.magFilter = filterModeFromGltf(sampler.magFilter);
		desc.minFilter = filterModeFromGltf(sampler.minFilter);
		desc.mipmapFilter = mipmapFilterModeFromGltf(sampler.minFilter);
		desc.addressModeU = addressModeFromGltf(sampler.wrapS);
		desc.addressModeV = addressModeFromGltf(sampler.wrapT);
		desc.addressModeW = AddressMode::Repeat;
		desc.lodMinClamp = 0.0;
		desc.lodMaxClamp = 1.0;
		desc.maxAnisotropy = 1.0;
		wgpu::Sampler gpuSampler = m_device.createSampler(desc);
		m_samplers.push_back(gpuSampler);
	}
}

void GpuScene::terminateSamplers() {
	for (wgpu::Sampler s : m_samplers) {
		s.release();
	}
	m_samplers.clear();
}

void GpuScene::initDrawCalls(const tinygltf::Model& model) {
	const Mesh& mesh = model.meshes[0];
	const Primitive& prim = mesh.primitives[0];

	std::vector<std::tuple<std::string, uint32_t, VertexFormat>> semanticToLocation = {
		// GLTF Semantic, Shader input location, Default format
		{"POSITION", 0, VertexFormat::Float32x3},
		{"NORMAL", 1, VertexFormat::Float32x3},
		{"COLOR", 2, VertexFormat::Float32x3},
		{"TEXCOORD_0", 3, VertexFormat::Float32x2},
	};

	// We create one vertex buffer layout per bufferView
	std::vector<VertexBufferLayout> vertexBufferLayouts;
	// For each layout, there are multiple attributes
	std::vector<std::vector<VertexAttribute>> vertexBufferLayoutToAttributes;
	// For each layout, there is a single buffer view
	std::vector<tinygltf::BufferView> vertexBufferLayoutToBufferView;
	// And we keep the map from bufferView to layout index
	// NB: The index '-1' is used for attributes expected by the shader but not
	// provided by the GLTF file.
	std::unordered_map<int, size_t> bufferViewToVertexBufferLayout;

	for (const auto& [attrSemantic, location, defaultFormat] : semanticToLocation) {
		int bufferViewIdx;
		BufferView bufferView = {};
		VertexFormat format = VertexFormat::Undefined;
		uint64_t byteOffset;

		auto accessorIt = prim.attributes.find(attrSemantic);
		if (accessorIt != prim.attributes.end()) {
			Accessor accessor = model.accessors[accessorIt->second];
			bufferViewIdx = accessor.bufferView;
			bufferView = model.bufferViews[bufferViewIdx];
			format = vertexFormatFromAccessor(accessor);
			byteOffset = static_cast<uint64_t>(accessor.byteOffset);
			if (bufferView.byteStride == 0) {
				bufferView.byteStride = vertexFormatByteSize(format);
			}
		}
		else {
			bufferViewIdx = -1;
			bufferView.buffer = -1;
			format = defaultFormat;
			byteOffset = 0;
		}

		// Group attributes by bufferView
		size_t layoutIdx = 0;
		auto vertexBufferLayoutIt = bufferViewToVertexBufferLayout.find(bufferViewIdx);
		if (vertexBufferLayoutIt == bufferViewToVertexBufferLayout.end()) {
			layoutIdx = vertexBufferLayouts.size();
			VertexBufferLayout layout;
			layout.arrayStride = static_cast<uint64_t>(bufferView.byteStride);
			layout.stepMode = VertexStepMode::Vertex;
			vertexBufferLayouts.push_back(layout);
			vertexBufferLayoutToAttributes.push_back({});
			vertexBufferLayoutToBufferView.push_back(bufferView);
			bufferViewToVertexBufferLayout[bufferViewIdx] = layoutIdx;
		}
		else {
			layoutIdx = vertexBufferLayoutIt->second;
		}

		VertexAttribute attrib;
		attrib.shaderLocation = location;
		attrib.format = format;
		attrib.offset = byteOffset;
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
