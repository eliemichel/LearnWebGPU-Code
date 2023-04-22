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

	void initTexture();
	void terminateTexture();

	void initTextureViews();
	void terminateTextureViews();

	void initBindGroup(uint32_t nextMipLevel);
	void terminateBindGroup();

	void initBindGroupLayout();
	void terminateBindGroupLayout();

	void initComputePipeline();
	void terminateComputePipeline();

private:
	GLFWwindow* m_window = nullptr;
	wgpu::Surface m_surface = nullptr;
	wgpu::Instance m_instance = nullptr;
	wgpu::Device m_device = nullptr;
	wgpu::Queue m_queue = nullptr;
	wgpu::TextureFormat m_swapChainFormat = wgpu::TextureFormat::Undefined;
	wgpu::SwapChainDescriptor m_swapChainDesc;
	wgpu::SwapChain m_swapChain = nullptr;
	wgpu::PipelineLayout m_pipelineLayout = nullptr;
	wgpu::ComputePipeline m_pipeline = nullptr;
	std::unique_ptr<wgpu::ErrorCallback> m_uncapturedErrorCallback;
	std::unique_ptr<wgpu::DeviceLostCallback> m_deviceLostCallback;

	bool m_shouldCompute = true;
	wgpu::Texture m_texture = nullptr;
	std::vector<wgpu::TextureView> m_textureMipViews;
	std::vector<wgpu::Extent3D> m_textureMipSizes;
	wgpu::BindGroup m_bindGroup = nullptr;
	wgpu::BindGroupLayout m_bindGroupLayout = nullptr;

	struct Parameters {
		float test = 0.5f;
	};
	Parameters m_parameters;
};
