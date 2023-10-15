#pragma once

#include "tiny_gltf.h"

#include <webgpu/webgpu.hpp>
#include <glm/glm.hpp>

#include <vector>

/**
 * This holds the GPU-side data corresponding to a tinygltf::Model
 */
class GpuScene {
public:
	struct NodeUniforms {
		glm::mat4 modelMatrix;
	};
	static_assert(sizeof(NodeUniforms) % 16 == 0);

	struct MaterialUniforms {
		glm::vec4 baseColorFactor;
		float metallicFactor;
		float roughnessFactor;
		uint32_t baseColorTexCoords;
		uint32_t metallicRoughnessTexCoords;
		uint32_t normalTexCoords;
		float _pad[3];
	};
	static_assert(sizeof(MaterialUniforms) % 16 == 0);

public:
	// Create from a CPU-side tinygltf model (destroy previous data)
	void createFromModel(
		wgpu::Device device,
		const tinygltf::Model& model,
		wgpu::BindGroupLayout materialBindGroupLayout,
		wgpu::BindGroupLayout nodeBindGroupLayout
	);

	// Draw all nodes that use a given renderPipeline
	void draw(wgpu::RenderPassEncoder renderPass, uint32_t renderPipelineIndex);

	// Destroy and release all resources
	void destroy();

	// Accessors
	uint32_t renderPipelineCount() const;
	std::vector<wgpu::VertexBufferLayout> vertexBufferLayouts(uint32_t renderPipelineIndex) const;
	wgpu::PrimitiveTopology primitiveTopology(uint32_t renderPipelineIndex) const;

private:
	// NB: All init functions assume that the object is new (empty) or that
	// destroy() has just been called.

	void initDevice(wgpu::Device device);
	void terminateDevice();

	void initBuffers(const tinygltf::Model& model);
	void terminateBuffers();

	void initTextures(const tinygltf::Model& model);
	void terminateTextures();

	void initSamplers(const tinygltf::Model& model);
	void terminateSamplers();

	void initMaterials(const tinygltf::Model& model, wgpu::BindGroupLayout bindGroupLayout);
	void terminateMaterials();

	void initNodes(const tinygltf::Model& model, wgpu::BindGroupLayout bindGroupLayout);
	void terminateNodes();

	void initDrawCalls(const tinygltf::Model& model);
	void terminateDrawCalls();

private:
	// Device
	wgpu::Device m_device = nullptr;
	wgpu::Queue m_queue = nullptr;

	// Buffers
	std::vector<wgpu::Buffer> m_buffers;
	wgpu::Buffer m_nullBuffer = nullptr; // for attributes that are not provided

	// Texture
	std::vector<wgpu::Texture> m_textures;
	std::vector<wgpu::TextureView> m_textureViews;
	uint32_t m_defaultTextureIdx; // empty texture bound for materials that do not use a texture
	// This is what GLTF calls a texture, as opposed to wgpu::Texture that corresponds to gltf::Image
	struct SampledTexture {
		uint32_t textureIndex;
		uint32_t samplerIndex;
	};
	std::vector<SampledTexture> m_sampledTextures;

	// Samplers
	std::vector<wgpu::Sampler> m_samplers;
	uint32_t m_defaultSamplerIdx;

	// Materials
	struct Material {
		wgpu::BindGroup bindGroup = nullptr;
		wgpu::Buffer uniformBuffer = nullptr;
		MaterialUniforms uniforms;
	};
	std::vector<Material> m_materials;
	uint32_t m_defaultMaterialIdx;

	// We need to build one Render Pipeline per vertex buffer layout and per
	// shader, we try to detect which calls use the same layout to minimize the
	// number of Render Pipelines.
	struct RenderPipelineSettings {
		std::vector<std::vector<wgpu::VertexAttribute>> vertexAttributes;
		std::vector<wgpu::VertexBufferLayout> vertexBufferLayouts;
		wgpu::PrimitiveTopology primitiveTopology;
	};
	std::vector<RenderPipelineSettings> m_renderPipelines;

	// Draw Calls + Vertex Buffer Layouts
	struct MeshPrimitive {
		// Draw Call Data
		std::vector<tinygltf::BufferView> attributeBufferViews;
		tinygltf::BufferView indexBufferView;
		uint32_t indexBufferByteOffset;
		wgpu::IndexFormat indexFormat;
		uint32_t indexCount;
		uint32_t materialIndex;
		uint32_t renderPipelineIndex;
	};
	struct Mesh {
		std::vector<MeshPrimitive> primitives;
	};
	std::vector<Mesh> m_meshes;

	// Nodes
	struct Node {
		wgpu::BindGroup bindGroup = nullptr;
		wgpu::Buffer uniformBuffer = nullptr;
		NodeUniforms uniforms;
		uint32_t meshIndex;
	};
	std::vector<Node> m_nodes;
	
private:
	uint32_t getOrCreateRenderPipelineIndex(const RenderPipelineSettings& newSettings);
	bool isCompatible(const RenderPipelineSettings& a, const RenderPipelineSettings& b) const;
};

