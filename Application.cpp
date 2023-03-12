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

#include "Application.h"
#include "ResourceManager.h"
#include "MarchingSquaresRenderer.h"
#include "MarchingCubesRenderer.h"
#include "DualContouringRenderer.h"

#include <GLFW/glfw3.h>

#ifndef __EMSCRIPTEN__
#include "glfw3webgpu.h"
#endif

#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/polar_coordinates.hpp>

#include <imgui.h>
#include <backends/imgui_impl_wgpu.h>
#include <backends/imgui_impl_glfw.h>

#include <webgpu/webgpu.hpp>
#if defined(WEBGPU_BACKEND_WGPU)
#include <webgpu/wgpu.h> // wgpuTextureViewDrop
#endif

#include <iostream>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <array>

constexpr float PI = 3.14159265358979323846f;

using namespace wgpu;
using VertexAttributes = ResourceManager::VertexAttributes;
using glm::mat4x4;
using glm::vec4;
using glm::vec3;
using glm::vec2;

// GLFW callbacks
void onWindowResize(GLFWwindow* window, int width, int height) {
	(void)width; (void)height;
	auto pApp = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
	if (pApp != nullptr) pApp->onResize();
}
void onWindowMouseMove(GLFWwindow* window, double xpos, double ypos) {
	auto pApp = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
	if (pApp != nullptr) pApp->onMouseMove(xpos, ypos);
}
void onWindowMouseButton(GLFWwindow* window, int button, int action, int mods) {
	auto pApp = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
	if (pApp != nullptr) pApp->onMouseButton(button, action, mods);
}
void onWindowScroll(GLFWwindow* window, double xoffset, double yoffset) {
	auto pApp = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
	if (pApp != nullptr) pApp->onScroll(xoffset, yoffset);
}

bool Application::onInit() {
	// Create instance
	m_instance = createInstance(InstanceDescriptor{});
	if (!m_instance) {
		std::cerr << "Could not initialize WebGPU!" << std::endl;
		return false;
	}

	if (!glfwInit()) {
		std::cerr << "Could not initialize GLFW!" << std::endl;
		return false;
	}

	// Create window
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	m_window = glfwCreateWindow(640, 480, "Learn WebGPU", NULL, NULL);
	if (!m_window) {
		std::cerr << "Could not open window!" << std::endl;
		return false;
	}

	// Add window callbacks
	glfwSetWindowUserPointer(m_window, this);
	glfwSetFramebufferSizeCallback(m_window, onWindowResize);
	glfwSetCursorPosCallback(m_window, onWindowMouseMove);
	glfwSetMouseButtonCallback(m_window, onWindowMouseButton);
	glfwSetScrollCallback(m_window, onWindowScroll);

	// Create surface and adapter
	std::cout << "Requesting adapter..." << std::endl;
#if !defined(WEBGPU_BACKEND_EMSCRIPTEN)
	m_surface = glfwGetWGPUSurface(m_instance, m_window);
#endif
	RequestAdapterOptions adapterOpts{};
	adapterOpts.compatibleSurface = m_surface;
	Adapter adapter = m_instance.requestAdapter(adapterOpts);
	std::cout << "Got adapter: " << adapter << std::endl;

	std::cout << "Requesting device..." << std::endl;
	SupportedLimits supportedLimits;
	adapter.getLimits(&supportedLimits);
	RequiredLimits requiredLimits = Default;
	requiredLimits.limits.maxVertexAttributes = 4;
	requiredLimits.limits.maxVertexBuffers = 1;
	requiredLimits.limits.maxBindGroups = 2;
	requiredLimits.limits.maxUniformBuffersPerShaderStage = 2;
	requiredLimits.limits.maxUniformBufferBindingSize = 16 * 4 * sizeof(float);
	requiredLimits.limits.minStorageBufferOffsetAlignment = supportedLimits.limits.minStorageBufferOffsetAlignment;
	requiredLimits.limits.maxBufferSize = 32 * 1024 * 1024;
	requiredLimits.limits.maxTextureDimension2D = 4096;
	requiredLimits.limits.maxTextureDimension3D = 256;
	requiredLimits.limits.maxTextureArrayLayers = 1;
	requiredLimits.limits.maxStorageBuffersPerShaderStage = 2;
	requiredLimits.limits.maxStorageTexturesPerShaderStage = 1;
	requiredLimits.limits.maxSampledTexturesPerShaderStage = 2;
	requiredLimits.limits.maxSamplersPerShaderStage = 1;
	requiredLimits.limits.maxComputeWorkgroupSizeX = 1;
	requiredLimits.limits.maxComputeWorkgroupSizeY = 1;
	requiredLimits.limits.maxComputeWorkgroupSizeZ = 1;
	requiredLimits.limits.maxComputeInvocationsPerWorkgroup = 1;
	requiredLimits.limits.maxComputeWorkgroupsPerDimension = 1024;
	requiredLimits.limits.maxStorageBufferBindingSize = 32 * 1024 * 1024;
	requiredLimits.limits.maxVertexBufferArrayStride = 64;
	requiredLimits.limits.maxInterStageShaderComponents = 6;
	//requiredLimits.limits.maxInterStageShaderVariables = 4;

	// Create device
	DeviceDescriptor deviceDesc{};
	deviceDesc.label = "My Device";
	deviceDesc.requiredFeaturesCount = 0;
	deviceDesc.requiredLimits = &requiredLimits;
	deviceDesc.defaultQueue.label = "The default queue";
	m_device = adapter.requestDevice(deviceDesc);
	std::cout << "Got device: " << m_device << std::endl;

	// Add an error callback for more debug info
	m_uncapturedErrorCallback = m_device.setUncapturedErrorCallback([](ErrorType type, char const* message) {
		std::cout << "Device error: type " << type;
		if (message) std::cout << " (message: " << message << ")";
		std::cout << std::endl;
	});

	m_deviceLostCallback = m_device.setDeviceLostCallback([](DeviceLostReason reason, char const* message) {
		std::cout << "Device lost: reason " << reason;
		if (message) std::cout << " (message: " << message << ")";
		std::cout << std::endl;
	});

	// Create swapchain
#if defined(WEBGPU_BACKEND_WGPU)
	m_swapChainFormat = m_surface.getPreferredFormat(adapter);
#else
	m_swapChainFormat = TextureFormat::BGRA8Unorm;
#endif

	buildSwapChain();
	buildCameraUniformBuffer();
	buildDepthBuffer();

	AbstractRenderer::InitContext ctx{
		m_device,
		m_swapChainFormat,
		m_depthTextureFormat,
		m_uniformBuffer,
		sizeof(CameraUniforms)
	};
	m_marchingSquaresRenderer = std::make_shared<MarchingSquaresRenderer>(ctx, 128);
	m_marchingCubesRenderer = std::make_shared<MarchingCubesRenderer>(ctx, 128);
	m_dualContouringRenderer = std::make_shared<DualContouringRenderer>(ctx, 128);

	initGui();

	return true;
}

void Application::buildSwapChain() {
	int width, height;
	glfwGetFramebufferSize(m_window, &width, &height);

	std::cout << "Creating swapchain..." << std::endl;
	m_swapChainDesc.width = (uint32_t)width;
	m_swapChainDesc.height = (uint32_t)height;
	m_swapChainDesc.usage = TextureUsage::RenderAttachment;
	m_swapChainDesc.format = m_swapChainFormat;
	m_swapChainDesc.presentMode = PresentMode::Fifo;
	m_swapChain = m_device.createSwapChain(m_surface, m_swapChainDesc);
	std::cout << "Swapchain: " << m_swapChain << std::endl;
}

void Application::buildDepthBuffer() {
	// Destroy previously allocated texture
	if (m_depthTexture != nullptr) m_depthTexture.destroy();

	std::cout << "Creating depth texture..." << std::endl;
	// Create the depth texture
	TextureDescriptor depthTextureDesc;
	depthTextureDesc.dimension = TextureDimension::_2D;
	depthTextureDesc.format = m_depthTextureFormat;
	depthTextureDesc.mipLevelCount = 1;
	depthTextureDesc.sampleCount = 1;
	depthTextureDesc.size = { m_swapChainDesc.width, m_swapChainDesc.height, 1 };
	depthTextureDesc.usage = TextureUsage::RenderAttachment;
	depthTextureDesc.viewFormatCount = 1;
	depthTextureDesc.viewFormats = (WGPUTextureFormat*)&m_depthTextureFormat;
	m_depthTexture = m_device.createTexture(depthTextureDesc);
	std::cout << "Depth texture: " << m_depthTexture << std::endl;

	// Create the view of the depth texture manipulated by the rasterizer
	TextureViewDescriptor depthTextureViewDesc;
	depthTextureViewDesc.aspect = TextureAspect::DepthOnly;
	depthTextureViewDesc.baseArrayLayer = 0;
	depthTextureViewDesc.arrayLayerCount = 1;
	depthTextureViewDesc.baseMipLevel = 0;
	depthTextureViewDesc.mipLevelCount = 1;
	depthTextureViewDesc.dimension = TextureViewDimension::_2D;
	depthTextureViewDesc.format = m_depthTextureFormat;
	m_depthTextureView = m_depthTexture.createView(depthTextureViewDesc);
}

void Application::buildCameraUniformBuffer() {
	// Create camera uniform buffer
	BufferDescriptor bufferDesc;
	bufferDesc.size = sizeof(CameraUniforms);
	bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Uniform;
	bufferDesc.mappedAtCreation = false;
	m_uniformBuffer = m_device.createBuffer(bufferDesc);

	// Upload the initial value of the uniforms
	m_uniforms.time = 1.0f;
	m_uniforms.color = { 0.0f, 1.0f, 0.4f, 1.0f };

	// Matrices
	m_uniforms.modelMatrix = mat4x4(1.0);
	m_uniforms.viewMatrix = glm::lookAt(vec3(-2.0f, -3.0f, 2.0f), vec3(0.0f), vec3(0, 0, 1));
	m_uniforms.projectionMatrix = glm::perspective(45 * PI / 180, 640.0f / 480.0f, 0.01f, 100.0f);

	Queue queue = m_device.getQueue();
	queue.writeBuffer(m_uniformBuffer, 0, &m_uniforms, sizeof(CameraUniforms));
	updateViewMatrix();
}

void Application::onFrame() {
	glfwPollEvents();
	Queue queue = m_device.getQueue();

	updateDragInertia();
	m_marchingSquaresRenderer->bake();
	m_marchingCubesRenderer->bake();
	m_dualContouringRenderer->bake();

	// Update uniform buffer
	m_uniforms.time = static_cast<float>(glfwGetTime());
	queue.writeBuffer(m_uniformBuffer, offsetof(CameraUniforms, time), &m_uniforms.time, sizeof(CameraUniforms::time));

	TextureView nextTexture = m_swapChain.getCurrentTextureView();
	if (!nextTexture) {
		std::cerr << "Cannot acquire next swap chain texture" << std::endl;
		return;
	}

	CommandEncoderDescriptor commandEncoderDesc{};
	commandEncoderDesc.label = "Command Encoder";
	CommandEncoder encoder = m_device.createCommandEncoder(commandEncoderDesc);

	RenderPassDescriptor renderPassDesc{};

	RenderPassColorAttachment colorAttachment;
	colorAttachment.view = nextTexture;
	colorAttachment.resolveTarget = nullptr;
	colorAttachment.loadOp = LoadOp::Clear;
	colorAttachment.storeOp = StoreOp::Store;
#if defined(WEBGPU_BACKEND_WGPU)
	colorAttachment.clearValue = Color{ 0.05, 0.05, 0.05, 1.0 };
#else
	constexpr auto NaN = std::numeric_limits<double>::quiet_NaN();
	colorAttachment.clearColor = Color{ NaN, NaN, NaN, NaN };
	colorAttachment.clearValue = Color{ 0.256, 0.256, 0.256, 1.0 };
#endif
	renderPassDesc.colorAttachmentCount = 1;
	renderPassDesc.colorAttachments = &colorAttachment;

	RenderPassDepthStencilAttachment depthStencilAttachment;
	depthStencilAttachment.view = m_depthTextureView;
	depthStencilAttachment.depthClearValue = 1.0f;
#if defined(WEBGPU_BACKEND_DAWN)
	constexpr auto NaNf = std::numeric_limits<float>::quiet_NaN();
	depthStencilAttachment.clearDepth = NaNf;
#endif
	depthStencilAttachment.depthLoadOp = LoadOp::Clear;
	depthStencilAttachment.depthStoreOp = StoreOp::Store;
	depthStencilAttachment.depthReadOnly = false;
	depthStencilAttachment.stencilClearValue = 0;
#if defined(WEBGPU_BACKEND_WGPU)
	depthStencilAttachment.stencilLoadOp = LoadOp::Clear;
	depthStencilAttachment.stencilStoreOp = StoreOp::Store;
#else
	depthStencilAttachment.stencilLoadOp = LoadOp::Undefined;
	depthStencilAttachment.stencilStoreOp = StoreOp::Undefined;
#endif
	depthStencilAttachment.stencilReadOnly = true;

	renderPassDesc.depthStencilAttachment = &depthStencilAttachment;
	renderPassDesc.timestampWriteCount = 0;
	renderPassDesc.timestampWrites = nullptr;
	RenderPassEncoder renderPass = encoder.beginRenderPass(renderPassDesc);

	AbstractRenderer::DrawingContext ctx{
		renderPass,
		m_settings.showWireframe
	};
	m_marchingSquaresRenderer->draw(ctx);
	m_marchingCubesRenderer->draw(ctx);
	m_dualContouringRenderer->draw(ctx);

	updateGui(renderPass);

	renderPass.end();

	CommandBuffer command = encoder.finish(CommandBufferDescriptor{});
	queue.submit(command);

#if defined(WEBGPU_BACKEND_WGPU)
	wgpuTextureViewDrop(nextTexture);
#endif
	m_swapChain.present();
}

void Application::onFinish() {
	for (auto texture : m_textures) {
		texture.destroy();
	}
	m_depthTexture.destroy();

	glfwDestroyWindow(m_window);
	glfwTerminate();
}

void Application::onResize() {
	buildSwapChain();
	buildDepthBuffer();

	float ratio = m_swapChainDesc.width / (float)m_swapChainDesc.height;
	m_uniforms.projectionMatrix = glm::perspective(45 * PI / 180, ratio, 0.01f, 100.0f);
	m_device.getQueue().writeBuffer(m_uniformBuffer, offsetof(CameraUniforms, projectionMatrix), &m_uniforms.projectionMatrix, sizeof(CameraUniforms::projectionMatrix));
}

void Application::onMouseMove(double xpos, double ypos) {
	if (m_drag.active) {
		vec2 currentMouse = vec2(-(float)xpos, (float)ypos);
		vec2 delta = (currentMouse - m_drag.startMouse) * m_drag.sensitivity;
		m_cameraState.angles = m_drag.startCameraState.angles + delta;
		m_cameraState.angles.y = glm::clamp(m_cameraState.angles.y, -PI / 2 + 1e-5f, PI / 2 - 1e-5f);
		updateViewMatrix();

		// Inertia
		m_drag.velocity = delta - m_drag.previousDelta;
		m_drag.previousDelta = delta;
	}
}

void Application::onMouseButton(int button, int action, int mods) {
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	if (io.WantCaptureMouse) {
		return;
	}

	(void)mods;
	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		switch(action) {
		case GLFW_PRESS:
			m_drag.active = true;
			double xpos, ypos;
			glfwGetCursorPos(m_window, &xpos, &ypos);
			m_drag.startMouse = vec2(-(float)xpos, (float)ypos);
			m_drag.startCameraState = m_cameraState;
			break;
		case GLFW_RELEASE:
			m_drag.active = false;
			break;
		}
	}
}

void Application::onScroll(double xoffset, double yoffset) {
	(void)xoffset;
	m_cameraState.zoom += m_drag.scrollSensitivity * (float)yoffset;
	m_cameraState.zoom = glm::clamp(m_cameraState.zoom, -2.0f, 2.0f);
	updateViewMatrix();
}

bool Application::isRunning() {
	return !glfwWindowShouldClose(m_window);
}

void Application::updateViewMatrix() {
	float cx = cos(m_cameraState.angles.x);
	float sx = sin(m_cameraState.angles.x);
	float cy = cos(m_cameraState.angles.y);
	float sy = sin(m_cameraState.angles.y);
	vec3 position = vec3(cx * cy, sx * cy, sy) * std::exp(-m_cameraState.zoom);
	m_uniforms.viewMatrix = glm::lookAt(position, vec3(0.0f), vec3(0, 0, 1));
	m_device.getQueue().writeBuffer(m_uniformBuffer, offsetof(CameraUniforms, viewMatrix), &m_uniforms.viewMatrix, sizeof(CameraUniforms::viewMatrix));

	m_uniforms.cameraWorldPosition = position;
	m_device.getQueue().writeBuffer(m_uniformBuffer, offsetof(CameraUniforms, cameraWorldPosition), &m_uniforms.cameraWorldPosition, sizeof(CameraUniforms::cameraWorldPosition));
}

void Application::updateDragInertia() {
	constexpr float eps = 1e-4f;
	if (!m_drag.active) {
		if (std::abs(m_drag.velocity.x) < eps && std::abs(m_drag.velocity.y) < eps) {
			return;
		}
		m_cameraState.angles += m_drag.velocity;
		m_cameraState.angles.y = glm::clamp(m_cameraState.angles.y, -PI / 2 + 1e-5f, PI / 2 - 1e-5f);
		m_drag.velocity *= m_drag.intertia;
		updateViewMatrix();
	}
}

void Application::initGui() {
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOther(m_window, true);
	ImGui_ImplWGPU_Init(m_device, 3, m_swapChainFormat, m_depthTextureFormat);
}

namespace ImGui {
bool DragDirection(const char *label, vec4& direction) {
	vec2 angles = glm::degrees(glm::polar(vec3(direction)));
	bool changed = ImGui::DragFloat2(label, glm::value_ptr(angles));
	direction = vec4(glm::euclidean(glm::radians(angles)), direction.w);
	return changed;
}
} // namespace ImGui

void Application::updateGui(RenderPassEncoder renderPass) {
	// Start the Dear ImGui frame
	ImGui_ImplWGPU_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	/*
	ImGui::Begin("Settings");
	ImGui::Checkbox("Show Wireframe", &m_settings.showWireframe);
	ImGui::End();
	*/
	ImGui::Begin("Profiling");
	ImGui::End();

	ImGui::Render();
	ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), renderPass);
}

bool Application::initTexture(const std::filesystem::path &path) {
	// Create a texture
	TextureView textureView = nullptr;
	Texture texture = ResourceManager::loadTexture(path, m_device, &textureView);
	if (!texture) {
		std::cerr << "Could not load texture!" << std::endl;
		return false;
	}
	m_textures.push_back(texture);

	// Setup binding
	uint32_t bindingIndex = (uint32_t)m_bindingLayoutEntries.size();
	BindGroupLayoutEntry bindingLayout = Default;
	bindingLayout.binding = bindingIndex;
	bindingLayout.visibility = ShaderStage::Fragment;
	bindingLayout.texture.sampleType = TextureSampleType::Float;
	bindingLayout.texture.viewDimension = TextureViewDimension::_2D;
	m_bindingLayoutEntries.push_back(bindingLayout);

	BindGroupEntry binding = Default;
	binding.binding = bindingIndex;
	binding.textureView = textureView;
	m_bindings.push_back(binding);

	return true;
}
