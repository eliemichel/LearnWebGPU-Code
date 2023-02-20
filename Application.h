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

#include "ResourceManager.h"

#include <webgpu.hpp>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

class Application {
public:
	// A function called only once at the beginning. Returns false is init failed.
	bool onInit();

	// A function called at each frame, guarantied never to be called before `onInit`.
	void onFrame();

	// A function called only once at the very end.
	void onFinish();

	// A function called when the window is resized.
	void onResize();

	// Mouse events
	void onMouseMove(double xpos, double ypos);
	void onMouseButton(int button, int action, int mods);
	void onScroll(double xoffset, double yoffset);

	// A function that tells if the application is still running.
	bool isRunning();

private:
	void buildSwapChain();
	void buildDepthBuffer();
	void updateViewMatrix();

private:
	struct MyUniforms {
		glm::mat4x4 projectionMatrix;
		glm::mat4x4 viewMatrix;
		glm::mat4x4 modelMatrix;
		glm::vec4 color;
		float time;
		float _pad[3];
	};
	static_assert(sizeof(MyUniforms) % 16 == 0);

	// Everything that is initialized in `onInit` and needed in `onFrame`.
	GLFWwindow* window = nullptr;
	wgpu::Instance instance = nullptr;
	wgpu::Surface surface = nullptr;
	wgpu::TextureFormat swapChainFormat = wgpu::TextureFormat::Undefined;
	wgpu::TextureFormat depthTextureFormat = wgpu::TextureFormat::Depth24Plus;
	wgpu::Device device = nullptr;
	wgpu::SwapChain swapChain = nullptr;
	wgpu::Buffer uniformBuffer = nullptr;
	wgpu::TextureView depthTextureView = nullptr;
	wgpu::RenderPipeline pipeline = nullptr;
	wgpu::Buffer vertexBuffer = nullptr;
	wgpu::BindGroup bindGroup = nullptr;
	wgpu::Texture texture = nullptr;
	wgpu::Texture depthTexture = nullptr;
	wgpu::SwapChainDescriptor swapChainDesc;
	MyUniforms uniforms;
	std::vector<ResourceManager::VertexAttributes> vertexData;
	int indexCount;

	bool m_isDragging = false;
	glm::vec2 m_startDragMouse;
	glm::vec2 m_startDragCameraEulerAngles;
	glm::vec2 m_cameraEulerAngles = {0.8f, 0.5f};
	float m_zoom = -1.2f;
};
