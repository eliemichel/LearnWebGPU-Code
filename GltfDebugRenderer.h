#pragma once

#include "tiny_gltf.h"

#include <webgpu/webgpu.hpp>

/**
 * A renderer that draws debug frame axes for each node of a GLTF scene.
 * 
 * You must call create() before draw(), and destroy() at the end of the
 * program. If create() is called twice, the previously created data gets
 * destroyed.
 */
class GltfDebugRenderer {
public:
	// Create from a CPU-side tinygltf model
	void create(wgpu::Device device, const tinygltf::Model& model);

	// Draw all nodes that use a given renderPipeline
	void draw(wgpu::RenderPassEncoder renderPass);

	// Destroy and release all resources
	void destroy();

private:
	void initVertexBuffer();
	void terminateVertexBuffer();

	void initNodeData(const tinygltf::Model& model);
	void terminateNodeData();

	void initPipeline();
	void terminatePipeline();

private:
	wgpu::Device m_device = nullptr;
	wgpu::RenderPipeline m_pipeline = nullptr;
	wgpu::Buffer m_vertexBuffer = nullptr;
	uint64_t m_vertexBufferByteSize = 0;
	uint32_t m_nodeCount = 0;
	uint32_t m_vertexCount = 0;
};
