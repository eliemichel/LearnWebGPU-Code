/**
 * This file is part of the "Learn WebGPU for C++" book.
 *   https://github.com/eliemichel/LearnWebGPU
 *
 * MIT License
 * Copyright (c) 2022-2023 Elie Michel
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <webgpu/webgpu.hpp>

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include <array>
#include <memory>

class Application {
public:
	// A function called only once at the beginning. Returns false is init failed.
	bool onInit();

	// A function called only once at the very end.
	void onFinish();

	// A function that tells if the application is still running.
	bool isRunning();

	// A function called at each frame, guarantied never to be called before `onInit`.
	void onFrame();

	// Where the GPU computation is actually issued
	void onCompute();

	// A function that tells if the compute pass should be executed
	// (i.e., when filter parameters changed)
	bool shouldCompute();

public:
	// A function called when we should draw the GUI.
	void onGui(wgpu::RenderPassEncoder renderPass);

	// A function called when the window is resized.
	void onResize();

private:
	// Detailed steps
	bool initDevice();
	void terminateDevice();

	bool initWindow();
	void terminateWindow();

	void initSwapChain();
	void terminateSwapChain();

	void initGui();
	void terminateGui();

	void initBuffers();
	void terminateBuffers();

	void initTextures();
	void initTexturesEquirectToCubemap();
	void initTexturesCubemapToEquirect();
	void terminateTextures();

	void initTextureViews();
	void terminateTextureViews();

	void initSampler();
	void terminateSampler();

	void initBindGroups();
	void terminateBindGroups();

	void initBindGroupLayouts();
	void terminateBindGroupLayouts();

	void initComputePipelines();
	void terminateComputePipelines();

private:
	GLFWwindow* m_window = nullptr;
	wgpu::Surface m_surface = nullptr;
	wgpu::Instance m_instance = nullptr;
	wgpu::Adapter m_adapter = nullptr;
	wgpu::Device m_device = nullptr;
	wgpu::Queue m_queue = nullptr;
	wgpu::TextureFormat m_swapChainFormat = wgpu::TextureFormat::Undefined;
	wgpu::SwapChainDescriptor m_swapChainDesc;
	wgpu::SwapChain m_swapChain = nullptr;
	std::vector<wgpu::BindGroup> m_bindGroups;
	wgpu::BindGroupLayout m_bindGroupLayout = nullptr;
	wgpu::BindGroupLayout m_prefilteringBindGroupLayout = nullptr;
	wgpu::PipelineLayout m_pipelineLayout = nullptr;
	wgpu::PipelineLayout m_prefilteringPipelineLayout = nullptr;
	wgpu::ComputePipeline m_pipeline = nullptr;
	wgpu::ComputePipeline m_prefilteringPipeline = nullptr;
	std::unique_ptr<wgpu::ErrorCallback> m_uncapturedErrorCallback;
	std::unique_ptr<wgpu::DeviceLostCallback> m_deviceLostCallback;

	bool m_shouldCompute = true;
	bool m_shouldReallocateTextures = false;
	bool m_shouldRebuildPipeline = false;
	wgpu::Buffer m_uniformBuffer = nullptr;
	wgpu::Texture m_equirectangularTexture = nullptr;
	wgpu::Texture m_cubemapTexture = nullptr;
	wgpu::TextureView m_equirectangularTextureView = nullptr;
	// We generate views for each MIP level, and for each cubemap face
	std::vector<std::array<wgpu::TextureView, 6>> m_cubemapTextureLayers;
	std::vector<wgpu::TextureView> m_cubemapTextureMips;
	// Mip level 0, always as a cube (for read only)
	wgpu::TextureView m_cubemapTextureCubeView = nullptr;
	wgpu::Sampler m_sampler = nullptr;

	enum class CubeFace {
		PositiveX = 0,
		NegativeX = 1,
		PositiveY = 2,
		NegativeY = 3,
		PositiveZ = 4,
		NegativeZ = 5,
	};

	// Values exposed to the UI
	enum class FilterType {
		Sum,
		Maximum,
		Minimum,
	};
	struct Parameters {
		FilterType filterType = FilterType::Sum;
		glm::mat3x4 kernel = glm::mat3x4(1.0);
		bool normalize = true;
	};
	Parameters m_parameters;

	// Computed from the Parameters
	struct Uniforms {
		uint32_t currentMipLevel;
		uint32_t mipLevelCount;
		uint32_t _pad[2];
	};
	static_assert(sizeof(Uniforms) % 16 == 0);
	Uniforms m_uniforms;
	uint32_t m_uniformStride;

	enum class Mode {
		EquirectToCubemap,
		CubemapToEquirect,
	};
	// Similar to parameters, but do not trigger recomputation of the effect
	struct Settings {
		float scale = 0.5f;
		glm::vec2 offset = { 0.0f, 0.0f };
		int outputSizeLog = 9;
		Mode mode = Mode::EquirectToCubemap;
		int mipLevel = 0;
	};
	Settings m_settings;
};
