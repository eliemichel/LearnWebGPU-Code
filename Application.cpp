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

#include <GLFW/glfw3.h>
#include "glfw3webgpu.h"

#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <imgui.h>
#include <backends/imgui_impl_wgpu.h>
#include <backends/imgui_impl_glfw.h>

#include <webgpu.hpp>

#include <iostream>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <array>

#if defined(WEBGPU_BACKEND_WGPU)
#include <wgpu.h>
#define wgpuTextureViewRelease wgpuTextureViewDrop
#endif

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
	m_surface = glfwGetWGPUSurface(m_instance, m_window);
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
	requiredLimits.limits.maxUniformBuffersPerShaderStage = 1;
	requiredLimits.limits.maxUniformBufferBindingSize = 16 * 4 * sizeof(float);

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

	Queue queue = m_device.getQueue();

	// Create swapchain
#if defined(WEBGPU_BACKEND_DAWN)
	m_swapChainFormat = WGPUTextureFormat_BGRA8Unorm;
#else
	m_swapChainFormat = m_surface.getPreferredFormat(adapter);
#endif
	buildSwapChain();

	std::cout << "Creating shader module..." << std::endl;
	ShaderModule shaderModule = ResourceManager::loadShaderModule(RESOURCE_DIR "/shader.wsl", m_device);
	std::cout << "Shader module: " << shaderModule << std::endl;

	std::cout << "Creating render pipeline..." << std::endl;
	RenderPipelineDescriptor pipelineDesc{};

	// Vertex fetch
	std::vector<VertexAttribute> vertexAttribs(4);

	// Position attribute
	vertexAttribs[0].shaderLocation = 0;
	vertexAttribs[0].format = VertexFormat::Float32x3;
	vertexAttribs[0].offset = offsetof(VertexAttributes, position);

	// Normal attribute
	vertexAttribs[1].shaderLocation = 1;
	vertexAttribs[1].format = VertexFormat::Float32x3;
	vertexAttribs[1].offset = offsetof(VertexAttributes, normal);

	// Color attribute
	vertexAttribs[2].shaderLocation = 2;
	vertexAttribs[2].format = VertexFormat::Float32x3;
	vertexAttribs[2].offset = offsetof(VertexAttributes, color);

	// UV attribute
	vertexAttribs[3].shaderLocation = 3;
	vertexAttribs[3].format = VertexFormat::Float32x2;
	vertexAttribs[3].offset = offsetof(VertexAttributes, uv);

	VertexBufferLayout vertexBufferLayout;
	vertexBufferLayout.attributeCount = (uint32_t)vertexAttribs.size();
	vertexBufferLayout.attributes = vertexAttribs.data();
	vertexBufferLayout.arrayStride = sizeof(VertexAttributes);
	vertexBufferLayout.stepMode = VertexStepMode::Vertex;

	pipelineDesc.vertex.bufferCount = 1;
	pipelineDesc.vertex.buffers = &vertexBufferLayout;

	pipelineDesc.vertex.module = shaderModule;
	pipelineDesc.vertex.entryPoint = "vs_main";
	pipelineDesc.vertex.constantCount = 0;
	pipelineDesc.vertex.constants = nullptr;

	pipelineDesc.primitive.topology = PrimitiveTopology::TriangleList;
	pipelineDesc.primitive.stripIndexFormat = IndexFormat::Undefined;
	pipelineDesc.primitive.frontFace = FrontFace::CCW;
	pipelineDesc.primitive.cullMode = CullMode::None;

	FragmentState fragmentState{};
	pipelineDesc.fragment = &fragmentState;
	fragmentState.module = shaderModule;
	fragmentState.entryPoint = "fs_main";
	fragmentState.constantCount = 0;
	fragmentState.constants = nullptr;

	BlendState blendState{};
	blendState.color.srcFactor = BlendFactor::SrcAlpha;
	blendState.color.dstFactor = BlendFactor::OneMinusSrcAlpha;
	blendState.color.operation = BlendOperation::Add;
	blendState.alpha.srcFactor = BlendFactor::Zero;
	blendState.alpha.dstFactor = BlendFactor::One;
	blendState.alpha.operation = BlendOperation::Add;

	ColorTargetState colorTarget{};
	colorTarget.format = m_swapChainFormat;
	colorTarget.blend = &blendState;
	colorTarget.writeMask = ColorWriteMask::All;

	fragmentState.targetCount = 1;
	fragmentState.targets = &colorTarget;

	DepthStencilState depthStencilState = Default;
	depthStencilState.depthCompare = CompareFunction::Less;
	depthStencilState.depthWriteEnabled = true;
	depthStencilState.format = m_depthTextureFormat;
	depthStencilState.stencilReadMask = 0;
	depthStencilState.stencilWriteMask = 0;
	pipelineDesc.depthStencil = &depthStencilState;

	pipelineDesc.multisample.count = 1;
	pipelineDesc.multisample.mask = ~0u;
	pipelineDesc.multisample.alphaToCoverageEnabled = false;

	// Create binding layout
	std::vector<BindGroupLayoutEntry> bindingLayoutEntries(3, Default);
	//                                                     ^ This was a 2

	BindGroupLayoutEntry& bindingLayout = bindingLayoutEntries[0];
	bindingLayout.binding = 0;
	bindingLayout.visibility = ShaderStage::Vertex | ShaderStage::Fragment;
	bindingLayout.buffer.type = BufferBindingType::Uniform;
	bindingLayout.buffer.minBindingSize = sizeof(MyUniforms);

	BindGroupLayoutEntry& textureBindingLayout = bindingLayoutEntries[1];
	textureBindingLayout.binding = 1;
	textureBindingLayout.visibility = ShaderStage::Fragment;
	textureBindingLayout.texture.sampleType = TextureSampleType::Float;
	textureBindingLayout.texture.viewDimension = TextureViewDimension::_2D;

	// The new binding layout, for the sampler
	BindGroupLayoutEntry& samplerBindingLayout = bindingLayoutEntries[2];
	samplerBindingLayout.binding = 2;
	samplerBindingLayout.visibility = ShaderStage::Fragment;
	samplerBindingLayout.sampler.type = SamplerBindingType::Filtering;

	// Create a bind group layout
	BindGroupLayoutDescriptor bindGroupLayoutDesc{};
	bindGroupLayoutDesc.entryCount = (uint32_t)bindingLayoutEntries.size();
	bindGroupLayoutDesc.entries = bindingLayoutEntries.data();
	BindGroupLayout bindGroupLayout = m_device.createBindGroupLayout(bindGroupLayoutDesc);

	// Create the pipeline layout
	PipelineLayoutDescriptor layoutDesc{};
	layoutDesc.bindGroupLayoutCount = 1;
	layoutDesc.bindGroupLayouts = &(WGPUBindGroupLayout)bindGroupLayout;
	PipelineLayout layout = m_device.createPipelineLayout(layoutDesc);
	pipelineDesc.layout = layout;

	m_pipeline = m_device.createRenderPipeline(pipelineDesc);
	std::cout << "Render pipeline: " << m_pipeline << std::endl;

	bool success = ResourceManager::loadGeometryFromObj(RESOURCE_DIR "/fourareen.obj", m_vertexData);
	if (!success) {
		std::cerr << "Could not load geometry!" << std::endl;
		return 1;
	}

	// Create vertex buffer
	BufferDescriptor bufferDesc;
	bufferDesc.size = m_vertexData.size() * sizeof(VertexAttributes);
	bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Vertex;
	bufferDesc.mappedAtCreation = false;
	m_vertexBuffer = m_device.createBuffer(bufferDesc);
	queue.writeBuffer(m_vertexBuffer, 0, m_vertexData.data(), bufferDesc.size);

	m_indexCount = static_cast<int>(m_vertexData.size());

	// Create uniform buffer
	bufferDesc.size = sizeof(MyUniforms);
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

	queue.writeBuffer(m_uniformBuffer, 0, &m_uniforms, sizeof(MyUniforms));
	updateViewMatrix();

	buildDepthBuffer();

	// Create a texture
	TextureView textureView = nullptr;
	m_texture = ResourceManager::loadTexture(RESOURCE_DIR "/fourareen2K_albedo.jpg", m_device, &textureView);
	if (!m_texture) {
		std::cerr << "Could not load texture!" << std::endl;
		return 1;
	}

	// Create a sampler
	SamplerDescriptor samplerDesc;
	samplerDesc.addressModeU = AddressMode::Repeat;
	samplerDesc.addressModeV = AddressMode::Repeat;
	samplerDesc.addressModeW = AddressMode::Repeat;
	samplerDesc.magFilter = FilterMode::Linear;
	samplerDesc.minFilter = FilterMode::Linear;
#if defined(WEBGPU_BACKEND_WGPU)
	samplerDesc.mipmapFilter = MipmapFilterMode::Linear;
#else
	samplerDesc.mipmapFilter = FilterMode::Linear;
#endif
	samplerDesc.lodMinClamp = 0.0f;
	samplerDesc.lodMaxClamp = 32.0f;
	samplerDesc.compare = CompareFunction::Undefined;
	samplerDesc.maxAnisotropy = 1;
	Sampler sampler = m_device.createSampler(samplerDesc);

	// Create a binding
	std::vector<BindGroupEntry> bindings(3);
	//                                   ^ This was a 2

	bindings[0].binding = 0;
	bindings[0].buffer = m_uniformBuffer;
	bindings[0].offset = 0;
	bindings[0].size = sizeof(MyUniforms);

	bindings[1].binding = 1;
	bindings[1].textureView = textureView;

	// A new binding for the sampler
	bindings[2].binding = 2;
	bindings[2].sampler = sampler;

	// Create bind group
	BindGroupDescriptor bindGroupDesc{};
	bindGroupDesc.layout = bindGroupLayout;
	bindGroupDesc.entryCount = (uint32_t)bindings.size();
	bindGroupDesc.entries = bindings.data();
	m_bindGroup = m_device.createBindGroup(bindGroupDesc);

	initGui();
	computeStuff();

	return true;
}

void Application::buildSwapChain() {
	int width, height;
	glfwGetFramebufferSize(m_window, &width, &height);

	std::cout << "Creating swapchain..." << std::endl;
	m_swapChainDesc = {};
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

void Application::onFrame() {
	computeStuff();

	glfwPollEvents();
	Queue queue = m_device.getQueue();

	updateDragInertia();

	// Update uniform buffer
	m_uniforms.time = static_cast<float>(glfwGetTime());
	queue.writeBuffer(m_uniformBuffer, offsetof(MyUniforms, time), &m_uniforms.time, sizeof(MyUniforms::time));

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
	colorAttachment.clearValue = Color{ 0.05, 0.05, 0.05, 1.0 };
	renderPassDesc.colorAttachmentCount = 1;
	renderPassDesc.colorAttachments = &colorAttachment;

	RenderPassDepthStencilAttachment depthStencilAttachment;
	depthStencilAttachment.view = m_depthTextureView;
	depthStencilAttachment.depthClearValue = 100.0f;
	depthStencilAttachment.depthLoadOp = LoadOp::Clear;
	depthStencilAttachment.depthStoreOp = StoreOp::Store;
	depthStencilAttachment.depthReadOnly = false;
	depthStencilAttachment.stencilClearValue = 0;
	depthStencilAttachment.stencilLoadOp = LoadOp::Clear;
	depthStencilAttachment.stencilStoreOp = StoreOp::Store;
	depthStencilAttachment.stencilReadOnly = true;

	renderPassDesc.depthStencilAttachment = &depthStencilAttachment;
	renderPassDesc.timestampWriteCount = 0;
	renderPassDesc.timestampWrites = nullptr;
	RenderPassEncoder renderPass = encoder.beginRenderPass(renderPassDesc);

	renderPass.setPipeline(m_pipeline);

	renderPass.setVertexBuffer(0, m_vertexBuffer, 0, m_vertexData.size() * sizeof(VertexAttributes));
	renderPass.setBindGroup(0, m_bindGroup, 0, nullptr);

	renderPass.draw(m_indexCount, 1, 0, 0);

	updateGui(renderPass);

	renderPass.end();

	CommandBuffer command = encoder.finish(CommandBufferDescriptor{});
	queue.submit(command);

	wgpuTextureViewRelease(nextTexture);
	m_swapChain.present();
}

void Application::onFinish() {
	m_texture.destroy();
	m_depthTexture.destroy();

	glfwDestroyWindow(m_window);
	glfwTerminate();
}

void Application::onResize() {
	buildSwapChain();
	buildDepthBuffer();

	float ratio = m_swapChainDesc.width / (float)m_swapChainDesc.height;
	m_uniforms.projectionMatrix = glm::perspective(45 * PI / 180, ratio, 0.01f, 100.0f);
	m_device.getQueue().writeBuffer(m_uniformBuffer, offsetof(MyUniforms, projectionMatrix), &m_uniforms.projectionMatrix, sizeof(MyUniforms::projectionMatrix));
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
	m_device.getQueue().writeBuffer(m_uniformBuffer, offsetof(MyUniforms, viewMatrix), &m_uniforms.viewMatrix, sizeof(MyUniforms::viewMatrix));
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

void Application::updateGui(RenderPassEncoder renderPass) {
	// Start the Dear ImGui frame
	ImGui_ImplWGPU_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	{
		static float f = 0.0f;
		static int counter = 0;
		static bool show_demo_window = true;
		static bool show_another_window = false;
		static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

		ImGui::Begin("Hello, world!");                                // Create a window called "Hello, world!" and append into it.

		ImGui::Text("This is some useful text.");                     // Display some text (you can use a format strings too)
		ImGui::Checkbox("Demo Window", &show_demo_window);            // Edit bools storing our window open/close state
		ImGui::Checkbox("Another Window", &show_another_window);

		ImGui::SliderFloat("float", &f, 0.0f, 1.0f);                  // Edit 1 float using a slider from 0.0f to 1.0f
		ImGui::ColorEdit3("clear color", (float*)&clear_color);       // Edit 3 floats representing a color

		if (ImGui::Button("Button"))                                  // Buttons return true when clicked (most widgets return true when edited/activated)
			counter++;
		ImGui::SameLine();
		ImGui::Text("counter = %d", counter);

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();
	}

	ImGui::Render();
	ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), renderPass);
}

void Application::computeStuff() {
	// Initialize a command encoder
	Queue queue = m_device.getQueue();
	CommandEncoderDescriptor encoderDesc = Default;
	CommandEncoder encoder = m_device.createCommandEncoder(encoderDesc);

	// Create input/output buffers
	BufferDescriptor bufferDesc;
	bufferDesc.mappedAtCreation = false;
	bufferDesc.size = 32 * sizeof(float);
	bufferDesc.usage = BufferUsage::Storage | BufferUsage::CopyDst;
	Buffer inputBuffer = m_device.createBuffer(bufferDesc);
	bufferDesc.usage = BufferUsage::Storage | BufferUsage::CopySrc;
	Buffer outputBuffer = m_device.createBuffer(bufferDesc);
	bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::MapRead;
	Buffer mapBuffer = m_device.createBuffer(bufferDesc);

	// Fill in input buffer
	std::vector<float> input(32);
	for (int i = 0; i < input.size(); ++i) {
		input[i] = 0.1f * i;
	}
	queue.writeBuffer(inputBuffer, 0, input.data(), input.size() * sizeof(float));

	// Create bind group layout
	std::vector<BindGroupLayoutEntry> bindings(2, Default);
	// Input buffer
	bindings[0].binding = 0;
	bindings[0].buffer.type = BufferBindingType::ReadOnlyStorage;
	bindings[0].visibility = ShaderStage::Compute;
	// Output buffer
	bindings[1].binding = 1;
	bindings[1].buffer.type = BufferBindingType::Storage;
	bindings[1].visibility = ShaderStage::Compute;

	BindGroupLayoutDescriptor bindGroupLayoutDesc;
	bindGroupLayoutDesc.entryCount = (uint32_t)bindings.size();
	bindGroupLayoutDesc.entries = bindings.data();
	BindGroupLayout bindGroupLayout = m_device.createBindGroupLayout(bindGroupLayoutDesc);

	// Create compute pipeline layout
	PipelineLayoutDescriptor pipelineLayoutDesc;
	pipelineLayoutDesc.bindGroupLayoutCount = 1;
	pipelineLayoutDesc.bindGroupLayouts = (WGPUBindGroupLayout*)&bindGroupLayout;
	PipelineLayout pipelineLayout = m_device.createPipelineLayout(pipelineLayoutDesc);

	// Load compute shader
	ShaderModule computeShaderModule = ResourceManager::loadShaderModule(RESOURCE_DIR "/compute-shader.wsl", m_device);

	// Create compute pipeline
	ComputePipelineDescriptor computePipelineDesc;
	computePipelineDesc.compute.constantCount = 0;
	computePipelineDesc.compute.constants = nullptr;
	computePipelineDesc.compute.entryPoint = "computeStuff";
	computePipelineDesc.compute.module = computeShaderModule;
	computePipelineDesc.layout = pipelineLayout;
	ComputePipeline computePipeline = m_device.createComputePipeline(computePipelineDesc);

	// Create compute bind group
	std::vector<BindGroupEntry> entries(2, Default);
	// Input buffer
	entries[0].binding = 0;
	entries[0].buffer = inputBuffer;
	entries[0].offset = 0;
	entries[0].size = bufferDesc.size;
	// Output buffer
	entries[1].binding = 1;
	entries[1].buffer = outputBuffer;
	entries[1].offset = 0;
	entries[1].size = bufferDesc.size;

	BindGroupDescriptor bindGroupDesc;
	bindGroupDesc.layout = bindGroupLayout;
	bindGroupDesc.entryCount = (uint32_t)entries.size();
	bindGroupDesc.entries = (WGPUBindGroupEntry*)entries.data();
	BindGroup bindGroup = m_device.createBindGroup(bindGroupDesc);

	// Create compute pass
	ComputePassDescriptor computePassDesc;
	computePassDesc.timestampWriteCount = 0;
	computePassDesc.timestampWrites = nullptr;
	ComputePassEncoder computePass = encoder.beginComputePass(computePassDesc);

	// Use compute pass
	computePass.setPipeline(computePipeline);
	computePass.setBindGroup(0, bindGroup, 0, nullptr);
	computePass.dispatchWorkgroups(1, 1, 1);

	// Finalize compute pass
	computePass.end();

	encoder.copyBufferToBuffer(outputBuffer, 0, mapBuffer, 0, bufferDesc.size);

	// Encode and submit the GPU commands
	CommandBuffer commands = encoder.finish(CommandBufferDescriptor{});
	queue.submit(commands);

	// Print output
	bool done = false;
	auto handle = mapBuffer.mapAsync(MapMode::Read, 0, bufferDesc.size, [&](BufferMapAsyncStatus status) {
		if (status == BufferMapAsyncStatus::Success) {
#ifdef WEBGPU_BACKEND_WGPU
			const float* output = (const float*)mapBuffer.getMappedRange(0, bufferDesc.size);
#else
			const float* output = (const float*)wgpuBufferGetConstMappedRange(mapBuffer, 0, bufferDesc.size);
#endif
			for (int i = 0; i < input.size(); ++i) {
				std::cout << "input " << input[i] << " became " << output[i] << std::endl;
			}
			mapBuffer.unmap();
		}
		done = true;
	});

	while (!done) {
		// Do nothing, this checks for ongoing asynchronous operations and call their callbacks if needed
		wgpuQueueSubmit(queue, 0, nullptr);
	}
}
