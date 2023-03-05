#pragma once

#include "macros.h"

#include <glm/glm.hpp>

#include <webgpu/webgpu.hpp>

#include <vector>

struct InitContext {
	wgpu::Device device;
	wgpu::TextureFormat swapChainFormat;
	wgpu::TextureFormat depthTextureFormat;
	wgpu::Buffer cameraUniformBuffer;
	uint32_t cameraUniformBufferSize;
};

struct DrawingContext {
	wgpu::RenderPassEncoder renderPass;
};

class MarchingCubesRenderer {
public:
	MOVE_ONLY(MarchingCubesRenderer)

	MarchingCubesRenderer(const InitContext& context, uint32_t resolution);
	~MarchingCubesRenderer();
	
	void bake();
	void draw(const DrawingContext& context) const;

private:
	struct BoundComputePipeline {
		wgpu::ComputePipeline pipeline = nullptr;
		wgpu::BindGroup bindGroup = nullptr;
	};

	BoundComputePipeline createBoundComputePipeline(
		wgpu::Device device,
		const char* label,
		const std::vector<wgpu::BindGroupLayoutEntry>& bindingLayouts,
		const std::vector<wgpu::BindGroupEntry>& bindings,
		wgpu::ShaderModule shaderModule,
		const char* entryPoint,
		const std::vector<wgpu::BindGroupLayout>& extraBindGroupLayouts = {}
	);

	void initBakingResources(const InitContext& context);
	void initDrawingResources(const InitContext& context);

private:
	struct Uniforms {
		uint32_t resolution;
	};

	struct VertexAttributes {
		glm::vec4 position;
	};

	struct Counts {
		uint32_t pointCount;
		uint32_t allocatedVertices;
	};

	Uniforms m_uniforms;
	wgpu::Device m_device = nullptr;
	wgpu::RenderPipeline m_drawingPipeline = nullptr;
	wgpu::Buffer m_vertexBuffer = nullptr;
	uint32_t m_vertexBufferSize = 0;
	wgpu::Buffer m_quadVertexBuffer = nullptr;
	wgpu::Buffer m_uniformBuffer = nullptr;
	wgpu::Buffer m_countBuffer = nullptr;
	wgpu::Buffer m_mapBuffer = nullptr;
	wgpu::Texture m_texture = nullptr;
	wgpu::TextureView m_textureView = nullptr;
	wgpu::BindGroup m_drawingBindGroup = nullptr;
	wgpu::BindGroupLayout m_vertexStorageBindGroupLayout = nullptr;
	wgpu::BindGroup m_vertexStorageBindGroup = nullptr;
	uint32_t m_vertexCount;
	struct BakingPipelines {
		BoundComputePipeline eval;
		BoundComputePipeline resetCount;
		BoundComputePipeline count;
		BoundComputePipeline fill;
	};
	BakingPipelines m_bakingPipelines;
};
