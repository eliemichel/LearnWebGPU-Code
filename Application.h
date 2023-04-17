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

class Application {
public:
	// A function called only once at the beginning. Returns false is init failed.
	bool onInit();

	// Where the GPU computation is actually issued
	void onCompute();

	// A function called only once at the very end.
	void onFinish();

private:
	// Detailed steps
	bool initDevice();
	void terminateDevice();

	void initBindGroup();
	void terminateBindGroup();

	void initBindGroupLayout();
	void terminateBindGroupLayout();

	void initComputePipeline();
	void terminateComputePipeline();

	void initBuffers();
	void terminateBuffers();

private:
	uint32_t m_bufferSize;
	// Everything that is initialized in `onInit` and needed in `onCompute`.
	wgpu::Instance m_instance = nullptr;
	wgpu::Device m_device = nullptr;
	wgpu::PipelineLayout m_pipelineLayout = nullptr;
	wgpu::ComputePipeline m_pipeline = nullptr;
	wgpu::Buffer m_inputBuffer = nullptr;
	wgpu::Buffer m_outputBuffer = nullptr;
	wgpu::Buffer m_mapBuffer = nullptr;
	wgpu::BindGroup m_bindGroup = nullptr;
	wgpu::BindGroupLayout m_bindGroupLayout = nullptr;
	std::unique_ptr<wgpu::ErrorCallback> m_uncapturedErrorCallback;
	std::unique_ptr<wgpu::DeviceLostCallback> m_deviceLostCallback;
};
