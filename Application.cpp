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
#include "webgpu-release.h"

#include <GLFW/glfw3.h>
#include "glfw3webgpu.h"

#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/polar_coordinates.hpp>

#include <imgui.h>
#include <backends/imgui_impl_wgpu.h>
#include <backends/imgui_impl_glfw.h>

#include <webgpu/webgpu.hpp>

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
	if (!initWindow()) return false;
	if (!initDevice()) return false;
	initSwapChain();
	initDepthBuffer();
	initShaders();
	initGui();
	initBindGroupLayouts();
	initPipelines();
	if (!initTextures()) return false;
	if (!initBuffers()) return false;
	initLighting();
	initSamplers();
	initBindGroups();

#ifdef WEBGPU_BACKEND_DAWN
	// Flush error queue
	m_device.tick();
#endif

	return true;
}

void Application::onFinish() {
	terminateBindGroups();
	terminateSamplers();
	terminateLighting();
	terminateBuffers();
	terminateTextures();
	terminatePipelines();
	terminateBindGroupLayouts();
	terminateGui();
	terminateShaders();
	terminateDepthBuffer();
	terminateSwapChain();
	terminateDevice();
	terminateWindow();
}

bool Application::initDevice() {
	// Create instance
	m_instance = createInstance(InstanceDescriptor{});
	if (!m_instance) {
		std::cerr << "Could not initialize WebGPU!" << std::endl;
		return false;
	}

	// Create surface and adapter
	std::cout << "Requesting adapter..." << std::endl;
	m_surface = glfwGetWGPUSurface(m_instance, m_window);
	RequestAdapterOptions adapterOpts{};
	adapterOpts.compatibleSurface = nullptr;
	adapterOpts.compatibleSurface = m_surface;
	m_adapter = m_instance.requestAdapter(adapterOpts);
	std::cout << "Got adapter: " << m_adapter << std::endl;

	std::cout << "Requesting device..." << std::endl;
	SupportedLimits supportedLimits;
	m_adapter.getLimits(&supportedLimits);
	RequiredLimits requiredLimits = Default;
	requiredLimits.limits.maxVertexAttributes = 6;
	requiredLimits.limits.maxVertexBuffers = 1;
	requiredLimits.limits.maxBindGroups = 2;
	requiredLimits.limits.maxUniformBuffersPerShaderStage = 2;
	requiredLimits.limits.maxUniformBufferBindingSize = 16 * 4 * sizeof(float);
	requiredLimits.limits.minStorageBufferOffsetAlignment = supportedLimits.limits.minStorageBufferOffsetAlignment;
	requiredLimits.limits.maxBufferSize = 4 * 1024 * 1024 * sizeof(float);
	requiredLimits.limits.maxTextureDimension1D = 4096;
	requiredLimits.limits.maxTextureDimension2D = 4096;
	requiredLimits.limits.maxTextureDimension3D = 4096;
	requiredLimits.limits.maxTextureArrayLayers = 1;
	requiredLimits.limits.maxSampledTexturesPerShaderStage = 3;
	requiredLimits.limits.maxSamplersPerShaderStage = 1;
	requiredLimits.limits.maxVertexBufferArrayStride = 68;
	requiredLimits.limits.maxInterStageShaderComponents = 18;

	// Create device
	DeviceDescriptor deviceDesc;
	deviceDesc.requiredFeaturesCount = 0;
	deviceDesc.requiredLimits = &requiredLimits;
	m_device = m_adapter.requestDevice(deviceDesc);
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

	m_queue = m_device.getQueue();

#ifdef WEBGPU_BACKEND_WGPU
	m_queue.submit(0, nullptr);
#else
	m_instance.processEvents();
#endif

	return true;
}

void Application::terminateDevice() {
#ifndef WEBGPU_BACKEND_WGPU
	wgpuQueueRelease(m_queue);
#endif WEBGPU_BACKEND_WGPU
	wgpuDeviceRelease(m_device);
	wgpuInstanceRelease(m_instance);
}

bool Application::initWindow() {
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
	return true;
}

void Application::terminateWindow() {
	glfwDestroyWindow(m_window);
	glfwTerminate();
}

void Application::initSwapChain() {
#ifdef WEBGPU_BACKEND_DAWN
	m_swapChainFormat = TextureFormat::BGRA8Unorm;
#else
	m_swapChainFormat = m_surface.getPreferredFormat(m_adapter);
#endif

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

void Application::terminateSwapChain() {
	wgpuSwapChainRelease(m_swapChain);
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

void Application::terminateGui() {
	ImGui_ImplWGPU_Shutdown();
	ImGui_ImplGlfw_Shutdown();
}

void Application::initShaders() {
	std::cout << "Creating shader module..." << std::endl;
	m_shaderModule = ResourceManager::loadShaderModule(RESOURCE_DIR "/shader.wgsl", m_device);
	std::cout << "Shader module: " << m_shaderModule << std::endl;
}

void Application::terminateShaders() {
	wgpuShaderModuleRelease(m_shaderModule);
}

void Application::initSamplers() {
	SamplerDescriptor samplerDesc;
	samplerDesc.addressModeU = AddressMode::Repeat;
	samplerDesc.addressModeV = AddressMode::Repeat;
	samplerDesc.addressModeW = AddressMode::Repeat;
	samplerDesc.magFilter = FilterMode::Linear;
	samplerDesc.minFilter = FilterMode::Linear;
#ifdef WEBGPU_BACKEND_DAWN
	samplerDesc.mipmapFilter = FilterMode::Linear;
#else
	samplerDesc.mipmapFilter = MipmapFilterMode::Linear;
#endif
	samplerDesc.lodMinClamp = 0.0f;
	samplerDesc.lodMaxClamp = 32.0f;
	samplerDesc.compare = CompareFunction::Undefined;
	samplerDesc.maxAnisotropy = 1;
	m_repeatSampler = m_device.createSampler(samplerDesc);

	samplerDesc.addressModeU = AddressMode::ClampToEdge;
	samplerDesc.addressModeV = AddressMode::ClampToEdge;
	samplerDesc.addressModeW = AddressMode::ClampToEdge;
	m_clampSampler = m_device.createSampler(samplerDesc);
}

void Application::terminateSamplers() {
	wgpuSamplerRelease(m_repeatSampler);
	wgpuSamplerRelease(m_clampSampler);
}

bool Application::initBuffers() {
	bool success = ResourceManager::loadGeometryFromObj(RESOURCE_DIR "/suzanne.obj", m_vertexData);
	//bool success = ResourceManager::loadGeometryFromObj(RESOURCE_DIR "/fourareen.obj", m_vertexData);
	if (!success) {
		std::cerr << "Could not load geometry!" << std::endl;
		return false;
	}

	// Create vertex buffer
	BufferDescriptor bufferDesc;
	bufferDesc.size = m_vertexData.size() * sizeof(VertexAttributes);
	bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Vertex;
	bufferDesc.mappedAtCreation = false;
	m_vertexBuffer = m_device.createBuffer(bufferDesc);
	m_queue.writeBuffer(m_vertexBuffer, 0, m_vertexData.data(), bufferDesc.size);

	m_indexCount = static_cast<int>(m_vertexData.size());

	// Create uniform buffer
	bufferDesc.size = sizeof(Uniforms);
	bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Uniform;
	bufferDesc.mappedAtCreation = false;
	m_uniformBuffer = m_device.createBuffer(bufferDesc);

	// Upload the initial value of the uniforms
	m_uniforms.time = 1.0f;
	m_uniforms.color = { 0.0f, 1.0f, 0.4f, 1.0f };

	// Matrices
	m_uniforms.modelMatrix = mat4x4(1.0);
	m_uniforms.viewMatrix = glm::lookAt(vec3(-2.0f, -3.0f, 2.0f), vec3(0.0f), vec3(0, 0, 1));
	m_uniforms.projectionMatrix = glm::perspective(20 * PI / 180, 640.0f / 480.0f, 0.01f, 100.0f);

	switch (m_swapChainFormat)
	{
	case WGPUTextureFormat_ASTC10x10UnormSrgb:
	case WGPUTextureFormat_ASTC10x5UnormSrgb:
	case WGPUTextureFormat_ASTC10x6UnormSrgb:
	case WGPUTextureFormat_ASTC10x8UnormSrgb:
	case WGPUTextureFormat_ASTC12x10UnormSrgb:
	case WGPUTextureFormat_ASTC12x12UnormSrgb:
	case WGPUTextureFormat_ASTC4x4UnormSrgb:
	case WGPUTextureFormat_ASTC5x5UnormSrgb:
	case WGPUTextureFormat_ASTC6x5UnormSrgb:
	case WGPUTextureFormat_ASTC6x6UnormSrgb:
	case WGPUTextureFormat_ASTC8x5UnormSrgb:
	case WGPUTextureFormat_ASTC8x6UnormSrgb:
	case WGPUTextureFormat_ASTC8x8UnormSrgb:
	case WGPUTextureFormat_BC1RGBAUnormSrgb:
	case WGPUTextureFormat_BC2RGBAUnormSrgb:
	case WGPUTextureFormat_BC3RGBAUnormSrgb:
	case WGPUTextureFormat_BC7RGBAUnormSrgb:
	case WGPUTextureFormat_BGRA8UnormSrgb:
	case WGPUTextureFormat_ETC2RGB8A1UnormSrgb:
	case WGPUTextureFormat_ETC2RGB8UnormSrgb:
	case WGPUTextureFormat_ETC2RGBA8UnormSrgb:
	case WGPUTextureFormat_RGBA8UnormSrgb:
		m_uniforms.gamma = 2.2f;
		break;
	default:
		m_uniforms.gamma = 1.0f;
	}

	m_queue.writeBuffer(m_uniformBuffer, 0, &m_uniforms, sizeof(Uniforms));
	updateViewMatrix();

	return true;
}

void Application::terminateBuffers() {
	wgpuShaderModuleRelease(m_shaderModule);
}

bool Application::initTextures() {
	m_textures.clear();
	m_textureViews.clear();

	// Create a texture and its view, whichever texture type (loader) is used
	auto load = [&](const std::filesystem::path& path, auto loader) {
		TextureView textureView = nullptr;
		Texture texture = loader(path, m_device, &textureView);
		if (!texture) {
			std::cerr << "Could not load prefiltered cubemap!" << std::endl;
			return false;
		}
		m_textures.push_back(texture);
		m_textureViews.push_back(textureView);
		return true;
	};

	if (!(
		//load(RESOURCE_DIR "/fourareen2K_albedo.jpg", ResourceManager::loadTexture) &&
		load(RESOURCE_DIR "/red.png", ResourceManager::loadTexture) &&
		load(RESOURCE_DIR "/fourareen2K_normals.png", ResourceManager::loadTexture) &&
		load(RESOURCE_DIR "/autumn_park", ResourceManager::loadPrefilteredCubemap) &&
		load(RESOURCE_DIR "/DFG.bin", ResourceManager::loadDFGTexture)
	)) return false;

	return true;
}

void Application::terminateTextures() {
	for (TextureView tv : m_textureViews) {
		wgpuTextureViewRelease(tv);
	}

	for (Texture t : m_textures) {
		t.destroy();
		wgpuTextureRelease(t);
	}
}

void Application::initBindGroupLayouts() {
	// Create binding layout
	std::vector<wgpu::BindGroupLayoutEntry> entries(8, Default);

	BindGroupLayoutEntry& uniformBindingLayout = entries[0];
	uniformBindingLayout.binding = 0;
	uniformBindingLayout.visibility = ShaderStage::Vertex | ShaderStage::Fragment;
	uniformBindingLayout.buffer.type = BufferBindingType::Uniform;
	uniformBindingLayout.buffer.minBindingSize = sizeof(Uniforms);

	BindGroupLayoutEntry& repeatSamplerBindingLayout = entries[1];
	repeatSamplerBindingLayout.binding = 1;
	repeatSamplerBindingLayout.visibility = ShaderStage::Fragment;
	repeatSamplerBindingLayout.sampler.type = SamplerBindingType::Filtering;

	BindGroupLayoutEntry& clampSamplerBindingLayout = entries[2];
	clampSamplerBindingLayout.binding = 2;
	clampSamplerBindingLayout.visibility = ShaderStage::Fragment;
	clampSamplerBindingLayout.sampler.type = SamplerBindingType::Filtering;

	BindGroupLayoutEntry& lightingBindingLayout = entries[3];
	lightingBindingLayout.binding = 3;
	lightingBindingLayout.visibility = ShaderStage::Fragment | ShaderStage::Vertex;
	lightingBindingLayout.buffer.type = BufferBindingType::Uniform;
	lightingBindingLayout.buffer.minBindingSize = sizeof(LightingUniforms);

	BindGroupLayoutEntry& baseColorTextureLayout = entries[4];
	baseColorTextureLayout.binding = 4;
	baseColorTextureLayout.visibility = ShaderStage::Fragment;
	baseColorTextureLayout.texture.sampleType = TextureSampleType::Float;
	baseColorTextureLayout.texture.viewDimension = TextureViewDimension::_2D;

	BindGroupLayoutEntry& normalTextureLayout = entries[5];
	normalTextureLayout.binding = 5;
	normalTextureLayout.visibility = ShaderStage::Fragment;
	normalTextureLayout.texture.sampleType = TextureSampleType::Float;
	normalTextureLayout.texture.viewDimension = TextureViewDimension::_2D;

	BindGroupLayoutEntry& environmentTextureLayout = entries[6];
	environmentTextureLayout.binding = 6;
	environmentTextureLayout.visibility = ShaderStage::Fragment;
	environmentTextureLayout.texture.sampleType = TextureSampleType::Float;
	environmentTextureLayout.texture.viewDimension = TextureViewDimension::Cube;

	BindGroupLayoutEntry& dfgLutLayout = entries[7];
	dfgLutLayout.binding = 7;
	dfgLutLayout.visibility = ShaderStage::Fragment;
	dfgLutLayout.texture.sampleType = TextureSampleType::Float;
	dfgLutLayout.texture.viewDimension = TextureViewDimension::_2D;

	// Create a bind group layout
	BindGroupLayoutDescriptor bindGroupLayoutDesc;
	bindGroupLayoutDesc.entryCount = (uint32_t)entries.size();
	bindGroupLayoutDesc.entries = entries.data();
	m_bindGroupLayout = m_device.createBindGroupLayout(bindGroupLayoutDesc);
}

void Application::terminateBindGroupLayouts() {
	wgpuBindGroupLayoutRelease(m_bindGroupLayout);
}

void Application::initBindGroups() {
	// Create bindings
	std::vector<wgpu::BindGroupEntry> entries(8, Default);

	uint32_t b = 0;
	entries[b].binding = b;
	entries[b].buffer = m_uniformBuffer;
	entries[b].offset = 0;
	entries[b].size = sizeof(Uniforms);

	++b; // 1
	entries[b].binding = b;
	entries[b].sampler = m_repeatSampler;

	++b; // 2
	entries[b].binding = b;
	entries[b].sampler = m_clampSampler;

	++b; // 3
	entries[b].binding = b;
	entries[b].buffer = m_lightingUniformBuffer;
	entries[b].offset = 0;
	entries[b].size = sizeof(LightingUniforms);

	++b; // 4
	entries[b].binding = b;
	entries[b].textureView = m_textureViews[0];

	++b; // 5
	entries[b].binding = b;
	entries[b].textureView = m_textureViews[1];

	++b; // 6
	entries[b].binding = b;
	entries[b].textureView = m_textureViews[2];

	++b; // 7
	entries[b].binding = b;
	entries[b].textureView = m_textureViews[3];

	// Create bind group
	BindGroupDescriptor bindGroupDesc;
	bindGroupDesc.layout = m_bindGroupLayout;
	bindGroupDesc.entryCount = (uint32_t)entries.size();
	bindGroupDesc.entries = entries.data();
	m_bindGroup = m_device.createBindGroup(bindGroupDesc);
}

void Application::terminateBindGroups() {
	wgpuBindGroupRelease(m_bindGroup);
}

void Application::initPipelines() {
	std::cout << "Creating render pipeline..." << std::endl;
	RenderPipelineDescriptor pipelineDesc{};

	// Vertex fetch
	std::vector<VertexAttribute> vertexAttribs(6);
	//                                         ^ This was a 4

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

	// Tangent attribute
	vertexAttribs[4].shaderLocation = 4;
	vertexAttribs[4].format = VertexFormat::Float32x3;
	vertexAttribs[4].offset = offsetof(VertexAttributes, tangent);

	// Bitangent attribute
	vertexAttribs[5].shaderLocation = 5;
	vertexAttribs[5].format = VertexFormat::Float32x3;
	vertexAttribs[5].offset = offsetof(VertexAttributes, bitangent);

	VertexBufferLayout vertexBufferLayout;
	vertexBufferLayout.attributeCount = (uint32_t)vertexAttribs.size();
	vertexBufferLayout.attributes = vertexAttribs.data();
	vertexBufferLayout.arrayStride = sizeof(VertexAttributes);
	vertexBufferLayout.stepMode = VertexStepMode::Vertex;

	pipelineDesc.vertex.bufferCount = 1;
	pipelineDesc.vertex.buffers = &vertexBufferLayout;

	pipelineDesc.vertex.module = m_shaderModule;
	pipelineDesc.vertex.entryPoint = "vs_main";
	pipelineDesc.vertex.constantCount = 0;
	pipelineDesc.vertex.constants = nullptr;

	pipelineDesc.primitive.topology = PrimitiveTopology::TriangleList;
	pipelineDesc.primitive.stripIndexFormat = IndexFormat::Undefined;
	pipelineDesc.primitive.frontFace = FrontFace::CCW;
	pipelineDesc.primitive.cullMode = CullMode::None;

	FragmentState fragmentState{};
	pipelineDesc.fragment = &fragmentState;
	fragmentState.module = m_shaderModule;
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

	// Create the pipeline layout
	PipelineLayoutDescriptor layoutDesc;
	layoutDesc.bindGroupLayoutCount = 1;
	layoutDesc.bindGroupLayouts = (WGPUBindGroupLayout*)&m_bindGroupLayout;
	m_pipelineLayout = m_device.createPipelineLayout(layoutDesc);
	pipelineDesc.layout = m_pipelineLayout;

	m_pipeline = m_device.createRenderPipeline(pipelineDesc);
	std::cout << "Render pipeline: " << m_pipeline << std::endl;
}

void Application::terminatePipelines() {
	wgpuRenderPipelineRelease(m_pipeline);
	wgpuPipelineLayoutRelease(m_pipelineLayout);
}

void Application::initDepthBuffer() {
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

void Application::terminateDepthBuffer() {
	wgpuTextureViewRelease(m_depthTextureView);
	m_depthTexture.destroy();
	wgpuTextureRelease(m_depthTexture);
}

void Application::onFrame() {
	glfwPollEvents();
	TextureView nextTexture = m_swapChain.getCurrentTextureView();
	if (!nextTexture) {
		std::cerr << "Cannot acquire next swap chain texture" << std::endl;
		return;
	}
	
	updateLighting();
	updateDragInertia();

	// Update uniform buffer
	m_uniforms.time = static_cast<float>(glfwGetTime());
	m_queue.writeBuffer(m_uniformBuffer, offsetof(Uniforms, time), &m_uniforms.time, sizeof(Uniforms::time));

	CommandEncoderDescriptor commandEncoderDesc{};
	commandEncoderDesc.label = "Command Encoder";
	CommandEncoder encoder = m_device.createCommandEncoder(commandEncoderDesc);

	RenderPassDescriptor renderPassDesc{};

	RenderPassColorAttachment colorAttachment;
	colorAttachment.view = nextTexture;
	colorAttachment.resolveTarget = nullptr;
	colorAttachment.loadOp = LoadOp::Clear;
	colorAttachment.storeOp = StoreOp::Store;
	double grey = std::pow(0.256, m_uniforms.gamma);
	colorAttachment.clearValue = Color{ grey, grey, grey, 1.0 };
	renderPassDesc.colorAttachmentCount = 1;
	renderPassDesc.colorAttachments = &colorAttachment;

	RenderPassDepthStencilAttachment depthStencilAttachment;
	depthStencilAttachment.view = m_depthTextureView;
	depthStencilAttachment.depthClearValue = 1.0f;
	depthStencilAttachment.depthLoadOp = LoadOp::Clear;
	depthStencilAttachment.depthStoreOp = StoreOp::Store;
	depthStencilAttachment.depthReadOnly = false;
	depthStencilAttachment.stencilClearValue = 0;
#ifdef WEBGPU_BACKEND_WGPU
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
	
	renderPass.setPipeline(m_pipeline);

	renderPass.setVertexBuffer(0, m_vertexBuffer, 0, m_vertexData.size() * sizeof(VertexAttributes));
	renderPass.setBindGroup(0, m_bindGroup, 0, nullptr);

	renderPass.draw(m_indexCount, 8, 0, 0);

	updateGui(renderPass);
	
	renderPass.end();
	renderPass.release();

	CommandBuffer command = encoder.finish(CommandBufferDescriptor{});
	encoder.release();
	m_queue.submit(command);
	command.release();
	
	nextTexture.release();
	
	m_swapChain.present();
}

void Application::onResize() {
	terminateDepthBuffer();
	terminateSwapChain();
	initSwapChain();
	initDepthBuffer();

	float ratio = m_swapChainDesc.width / (float)m_swapChainDesc.height;
	m_uniforms.projectionMatrix = glm::perspective(45 * PI / 180, ratio, 0.01f, 100.0f);
	m_device.getQueue().writeBuffer(m_uniformBuffer, offsetof(Uniforms, projectionMatrix), &m_uniforms.projectionMatrix, sizeof(Uniforms::projectionMatrix));
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
	m_cameraState.zoom = glm::clamp(m_cameraState.zoom, -5.0f, 5.0f);
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
	m_device.getQueue().writeBuffer(m_uniformBuffer, offsetof(Uniforms, viewMatrix), &m_uniforms.viewMatrix, sizeof(Uniforms::viewMatrix));

	m_uniforms.cameraWorldPosition = position;
	m_device.getQueue().writeBuffer(m_uniformBuffer, offsetof(Uniforms, cameraWorldPosition), &m_uniforms.cameraWorldPosition, sizeof(Uniforms::cameraWorldPosition));
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

	if (!glfwInit()) {
		std::cerr << "Could not initialize GLFW!" << std::endl;
		return false;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	//                             ^^^^^^^^^ This was GLFW_FALSE
	m_window = glfwCreateWindow(640, 480, "Learn WebGPU", NULL, NULL);
	if (!m_window) {
		std::cerr << "Could not open window!" << std::endl;
		return false;
	}

	std::cout << "Requesting adapter..." << std::endl;
	m_surface = glfwGetWGPUSurface(m_instance, m_window);
	RequestAdapterOptions adapterOpts{};
	adapterOpts.compatibleSurface = m_surface;
	Adapter adapter = m_instance.requestAdapter(adapterOpts);
	std::cout << "Got adapter: " << adapter << std::endl;

	SupportedLimits supportedLimits;
	adapter.getLimits(&supportedLimits);

	std::cout << "Requesting device..." << std::endl;
	RequiredLimits requiredLimits = Default;
	requiredLimits.limits.maxVertexAttributes = 4;
	requiredLimits.limits.maxVertexBuffers = 1;
	requiredLimits.limits.maxBufferSize = 150000 * sizeof(VertexAttributes);
	requiredLimits.limits.maxVertexBufferArrayStride = sizeof(VertexAttributes);
	requiredLimits.limits.minStorageBufferOffsetAlignment = supportedLimits.limits.minStorageBufferOffsetAlignment;
	requiredLimits.limits.minUniformBufferOffsetAlignment = supportedLimits.limits.minUniformBufferOffsetAlignment;
	requiredLimits.limits.maxInterStageShaderComponents = 8;
	requiredLimits.limits.maxBindGroups = 1;
	requiredLimits.limits.maxUniformBuffersPerShaderStage = 1;
	requiredLimits.limits.maxUniformBufferBindingSize = 16 * 4 * sizeof(float);
	// Allow textures up to 2K
	requiredLimits.limits.maxTextureDimension1D = 2048;
	requiredLimits.limits.maxTextureDimension2D = 2048;
	requiredLimits.limits.maxTextureArrayLayers = 1;
	requiredLimits.limits.maxSampledTexturesPerShaderStage = 1;
	requiredLimits.limits.maxSamplersPerShaderStage = 1;

	DeviceDescriptor deviceDesc;
	deviceDesc.label = "My Device";
	deviceDesc.requiredFeaturesCount = 0;
	deviceDesc.requiredLimits = &requiredLimits;
	deviceDesc.defaultQueue.label = "The default queue";
	m_device = adapter.requestDevice(deviceDesc);
	std::cout << "Got device: " << m_device << std::endl;

	// Add an error callback for more debug info
	m_errorCallbackHandle = m_device.setUncapturedErrorCallback([](ErrorType type, char const* message) {
		std::cout << "Device error: type " << type;
		if (message) std::cout << " (message: " << message << ")";
		std::cout << std::endl;
	});

	m_queue = m_device.getQueue();

#ifdef WEBGPU_BACKEND_WGPU
	m_swapChainFormat = m_surface.getPreferredFormat(adapter);
#else
	m_swapChainFormat = TextureFormat::BGRA8Unorm;
#endif

	// Set the user pointer to be "this"
	glfwSetWindowUserPointer(m_window, this);
	// Use a non-capturing lambda as resize callback
	glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow* window, int, int) {
		auto that = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
		if (that != nullptr) that->onResize();
	});

	adapter.release();
	return m_device != nullptr;
>>>>>>> 008c331 (Add missing releases)
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

	bool changed = false;
	ImGui::Begin("Lighting");
	changed = ImGui::ColorEdit3("Color #0", glm::value_ptr(m_lightingUniforms.colors[0])) || changed;
	changed = ImGui::DragDirection("Direction #0", m_lightingUniforms.directions[0]) || changed;
	changed = ImGui::ColorEdit3("Color #1", glm::value_ptr(m_lightingUniforms.colors[1])) || changed;
	changed = ImGui::DragDirection("Direction #1", m_lightingUniforms.directions[1]) || changed;
	changed = ImGui::SliderFloat("Roughness", &m_lightingUniforms.roughness, 0.0f, 1.0f) || changed;
	changed = ImGui::SliderFloat("Metallic", &m_lightingUniforms.metallic, 0.0f, 1.0f) || changed;
	changed = ImGui::SliderFloat("Reflectance", &m_lightingUniforms.reflectance, 0.0f, 1.0f) || changed;
	changed = ImGui::SliderFloat("Normal Map Strength", &m_lightingUniforms.normalMapStrength, 0.0f, 1.0f) || changed;
	bool highQuality = m_lightingUniforms.highQuality != 0;
	changed = ImGui::Checkbox("High Quality", &highQuality) || changed;
	m_lightingUniforms.highQuality = highQuality ? 1 : 0;
	changed = ImGui::SliderFloat("Roughness 2", &m_lightingUniforms.roughness2, 0.0f, 1.0f) || changed;
	changed = ImGui::SliderFloat("Metallic 2", &m_lightingUniforms.metallic2, 0.0f, 1.0f) || changed;
	changed = ImGui::SliderFloat("Reflectance 2", &m_lightingUniforms.reflectance2, 0.0f, 1.0f) || changed;
	changed = ImGui::SliderFloat("Instance Spacing", &m_lightingUniforms.instanceSpacing, 0.0f, 10.0f) || changed;
	ImGui::End();
	m_lightingUniformsChanged = changed;

	ImGui::Render();
	ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), renderPass);
}

void Application::initLighting() {
	Queue queue = m_device.getQueue();

	// Create uniform buffer
	BufferDescriptor bufferDesc;
	bufferDesc.size = sizeof(LightingUniforms);
	bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Uniform;
	bufferDesc.mappedAtCreation = false;
	m_lightingUniformBuffer = m_device.createBuffer(bufferDesc);

	// Load SH
	std::ifstream ifs(RESOURCE_DIR "/SH.txt");
	assert(ifs.is_open());
	for (auto& sh : m_lightingUniforms.sphericalHarmonics) {
		ifs >> sh.x;
		ifs >> sh.y;
		ifs >> sh.z;
	}

	// Upload the initial value of the uniforms
	m_lightingUniforms.directions = {
		vec4{0.5, -0.9, 0.1, 0.0},
		vec4{0.2, 0.4, 0.3, 0.0}
	};
	m_lightingUniforms.colors = {
		vec4{1.0, 0.9, 0.6, 1.0},
		vec4{0.6, 0.9, 1.0, 1.0}
	};
	m_lightingUniforms.roughness = 1.0f;
	m_lightingUniforms.metallic = 0.0f;
	m_lightingUniforms.reflectance = 0.5f;
	m_lightingUniforms.normalMapStrength = 0.5f;
	m_lightingUniforms.highQuality = true;
	m_lightingUniforms.roughness2 = 0.0f;
	m_lightingUniforms.metallic2 = 1.0f;
	m_lightingUniforms.reflectance2 = 0.5f;
	m_lightingUniforms.instanceSpacing = 2.0f;

	queue.writeBuffer(m_lightingUniformBuffer, 0, &m_lightingUniforms, sizeof(LightingUniforms));
}

void Application::updateLighting() {
	if (m_lightingUniformsChanged) {
		Queue queue = m_device.getQueue();
		queue.writeBuffer(m_lightingUniformBuffer, 0, &m_lightingUniforms, sizeof(LightingUniforms));
	}
}

void Application::terminateLighting() {
	m_lightingUniformBuffer.destroy();
	wgpuBufferDestroy(m_lightingUniformBuffer);
}
