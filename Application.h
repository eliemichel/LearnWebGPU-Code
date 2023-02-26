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

#include <array>
#include <filesystem>

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
	void updateDragInertia();

	void initGui(); // called in onInit
	void updateGui(wgpu::RenderPassEncoder renderPass); // called in onFrame

	bool initTexture(const std::filesystem::path& path);

	void initLighting();
	void updateLighting();

private:
	using vec2 = glm::vec2;
	using vec3 = glm::vec3;
	using vec4 = glm::vec4;
	using mat4x4 = glm::mat4x4;

	struct MyUniforms {
		mat4x4 projectionMatrix;
		mat4x4 viewMatrix;
		mat4x4 modelMatrix;
		vec4 color;
		vec3 cameraWorldPosition;
		float time;
	};
	static_assert(sizeof(MyUniforms) % 16 == 0);

	// Everything that is initialized in `onInit` and needed in `onFrame`.
	GLFWwindow* m_window = nullptr;
	wgpu::Instance m_instance = nullptr;
	wgpu::Surface m_surface = nullptr;
	wgpu::TextureFormat m_swapChainFormat = wgpu::TextureFormat::Undefined;
	wgpu::TextureFormat m_depthTextureFormat = wgpu::TextureFormat::Depth24Plus;
	wgpu::Device m_device = nullptr;
	wgpu::SwapChain m_swapChain = nullptr;
	wgpu::Buffer m_uniformBuffer = nullptr;
	wgpu::TextureView m_depthTextureView = nullptr;
	wgpu::RenderPipeline m_pipeline = nullptr;
	wgpu::Buffer m_vertexBuffer = nullptr;
	wgpu::BindGroup m_bindGroup = nullptr;
	std::vector<wgpu::Texture> m_textures;
	wgpu::Texture m_depthTexture = nullptr;
	wgpu::SwapChainDescriptor m_swapChainDesc;
	MyUniforms m_uniforms;
	std::vector<ResourceManager::VertexAttributes> m_vertexData;
	int m_indexCount;
	std::unique_ptr<wgpu::ErrorCallback> m_uncapturedErrorCallback;

	std::vector<wgpu::BindGroupLayoutEntry> m_bindingLayoutEntries;
	std::vector<wgpu::BindGroupEntry> m_bindings;

	// Lighting
	struct LightingUniforms {
		std::array<vec4, 2> directions;
		std::array<vec4, 2> colors;
		float hardness;
		float kd;
		float ks;
		float _pad;
	};
	static_assert(sizeof(LightingUniforms) % 16 == 0);
	wgpu::Buffer m_lightingUniformBuffer = nullptr;
	LightingUniforms m_lightingUniforms;
	bool m_lightingUniformsChanged = false;

	struct CameraState {
		// angles.x is the rotation of the camera around the global vertical axis, affected by mouse.x
		// angles.y is the rotation of the camera around its local horizontal axis, affected by mouse.y
		vec2 angles = { 0.8f, 0.5f };
		// zoom is the position of the camera along its local forward axis, affected by the scroll wheel
		float zoom = -1.2f;
	};

	struct DragState {
		// Whether a drag action is ongoing (i.e., we are between mouse press and mouse release)
		bool active = false;
		// The position of the mouse at the beginning of the drag action
		vec2 startMouse;
		// The camera state at the beginning of the drag action
		CameraState startCameraState;

		// Constant settings
		float sensitivity = 0.01f;
		float scrollSensitivity = 0.1f;

		// Inertia
		vec2 velocity = {0.0, 0.0};
		vec2 previousDelta;
		float intertia = 0.9f;
	};

	CameraState m_cameraState;
	DragState m_drag;
};
