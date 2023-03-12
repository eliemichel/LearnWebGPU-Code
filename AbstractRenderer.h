#pragma once

#include <webgpu/webgpu.hpp>

class AbstractRenderer {
public:
	struct InitContext {
		wgpu::Device device;
		wgpu::TextureFormat swapChainFormat;
		wgpu::TextureFormat depthTextureFormat;
		wgpu::Buffer cameraUniformBuffer;
		uint32_t cameraUniformBufferSize;
	};

	struct DrawingContext {
		wgpu::RenderPassEncoder renderPass;
		bool showWireframe;
	};

public:
	virtual void draw(const DrawingContext& context) const = 0;
};
