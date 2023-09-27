#pragma once

#include "tiny_gltf.h"

#include <webgpu/webgpu.hpp>

#include <vector>

/**
 * This holds the GPU-side data corresponding to a tinygltf::Model
 */
class GpuScene {
public:
	// Create from a CPU-side tinygltf model (destroy previous data)
	void createFromModel(wgpu::Device device, const tinygltf::Model& model);

	void draw(wgpu::RenderPassEncoder renderPass);

	// Destroy and release all resources
	void destroy();

	// Accessors
	std::vector<wgpu::VertexBufferLayout> vertexBufferLayouts(uint32_t drawCallIndex) const;

private:
	// NB: All init functions assume that the object is new (empty) or that
	// destroy() has just been called.

	void initDevice(wgpu::Device device);
	void terminateDevice();

	void initBuffers(const tinygltf::Model& model);
	void terminateBuffers();

	void initDrawCalls(const tinygltf::Model& model);
	void terminateDrawCalls();

private:
	// Device
	wgpu::Device m_device = nullptr;
	wgpu::Queue m_queue = nullptr;

	// Buffers
	std::vector<wgpu::Buffer> m_buffers;
	wgpu::Buffer m_nullBuffer = nullptr; // for attributes that are not provided

	// Draw Calls + Vertex Buffer Layouts
	struct DrawCall {
		// Vertex Buffer Layout Data
		std::vector<std::vector<wgpu::VertexAttribute>> vertexAttributes;
		std::vector<wgpu::VertexBufferLayout> vertexBufferLayouts;

		// Draw Call Data
		std::vector<tinygltf::BufferView> attributeBufferViews;
		tinygltf::BufferView indexBufferView;
		wgpu::IndexFormat indexFormat;
		uint32_t indexCount;
	};
	std::vector<DrawCall> m_drawCalls;
	
};

