#pragma once

#include "macros.h"

#include <glm/glm.hpp>

#include <webgpu/webgpu.hpp>

struct InitContext {
	wgpu::Device device;
	wgpu::TextureFormat swapChainFormat;
	wgpu::TextureFormat depthTextureFormat;
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
	void initBakingResources(const InitContext& context);
	void initDrawingResources(const InitContext& context);

private:
	struct Uniforms {
		uint32_t resolution;
	};

	struct VertexAttributes {
		glm::vec4 position;
	};

	Uniforms m_uniforms;
	wgpu::Device m_device = nullptr;
	wgpu::RenderPipeline m_drawingPipeline = nullptr;
	wgpu::Buffer m_vertexBuffer = nullptr;
	wgpu::Buffer m_quadVertexBuffer = nullptr;
	wgpu::Buffer m_uniformBuffer = nullptr;
	wgpu::Texture m_texture = nullptr;
	wgpu::TextureView m_textureView = nullptr;
	wgpu::BindGroup m_bakingBindGroup = nullptr;
	wgpu::BindGroup m_drawingBindGroup = nullptr;
	uint32_t m_vertexCount;
	struct BakingPipelines {
		wgpu::ComputePipeline eval = nullptr;
		wgpu::ComputePipeline count = nullptr;
		wgpu::ComputePipeline fill = nullptr;
	};
	BakingPipelines m_bakingPipelines;
};
