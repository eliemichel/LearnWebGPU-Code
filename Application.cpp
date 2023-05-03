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

#include "save_texture.h"

#include "stb_image.h"

#include <glfw3webgpu/glfw3webgpu.h>

#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <imgui.h>
#include <backends/imgui_impl_wgpu.h>
#include <backends/imgui_impl_glfw.h>

#include <webgpu/webgpu.hpp>
#include "webgpu-release.h"

#include <iostream>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <array>

constexpr float PI = 3.14159265358979323846f;

namespace fs = std::filesystem;
using namespace wgpu;
using glm::mat4x4;
using glm::vec4;
using glm::vec3;
using glm::vec2;

const fs::path cubemapPaths[] = {
	RESOURCE_DIR "/cubemap-posX.png",
	RESOURCE_DIR "/cubemap-negX.png",
	RESOURCE_DIR "/cubemap-posY.png",
	RESOURCE_DIR "/cubemap-negY.png",
	RESOURCE_DIR "/cubemap-posZ.png",
	RESOURCE_DIR "/cubemap-negZ.png",
};

const fs::path equirectangularPath = RESOURCE_DIR "/equirectangular.jpg";

// == Utils == //

// Equivalent of std::bit_width that is available from C++20 onward
uint32_t bit_width(uint32_t m) {
	if (m == 0) return 0;
	else { uint32_t w = 0; while (m >>= 1) ++w; return w; }
}

uint32_t getMaxMipLevelCount(const Extent3D& textureSize) {
	return bit_width(std::max(textureSize.width, textureSize.height));
}

// == GLFW Callbacks == //

void onWindowResize(GLFWwindow* window, int width, int height) {
	(void)width; (void)height;
	auto pApp = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
	if (pApp != nullptr) pApp->onResize();
}

// == Application == //

bool Application::onInit() {
	if (!initWindow()) return false;
	if (!initDevice()) return false;
	initSwapChain();
	initGui();
	initBindGroupLayout();
	initComputePipeline();
	initBuffers();
	initTextures();
	initTextureViews();
	initSampler();
	initBindGroup();
	return true;
}

void Application::onFinish() {
	terminateBindGroup();
	terminateSampler();
	terminateTextureViews();
	terminateTextures();
	terminateBuffers();
	terminateComputePipeline();
	terminateBindGroupLayout();
	terminateGui();
	terminateSwapChain();
	terminateDevice();
	terminateWindow();
}

bool Application::isRunning() {
	return !glfwWindowShouldClose(m_window);
}

bool Application::shouldCompute() {
	return m_shouldCompute;
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
	requiredLimits.limits.maxBufferSize = 80;
	requiredLimits.limits.maxTextureDimension1D = 4096;
	requiredLimits.limits.maxTextureDimension2D = 4096;
	requiredLimits.limits.maxTextureDimension3D = 4096;
	requiredLimits.limits.maxTextureArrayLayers = 1;
	requiredLimits.limits.maxSampledTexturesPerShaderStage = 3;
	requiredLimits.limits.maxSamplersPerShaderStage = 1;
	requiredLimits.limits.maxVertexBufferArrayStride = 68;
	requiredLimits.limits.maxInterStageShaderComponents = 17;
	requiredLimits.limits.maxStorageBuffersPerShaderStage = 2;
	requiredLimits.limits.maxComputeWorkgroupSizeX = 8;
	requiredLimits.limits.maxComputeWorkgroupSizeY = 8;
	requiredLimits.limits.maxComputeWorkgroupSizeZ = 1;
	requiredLimits.limits.maxComputeInvocationsPerWorkgroup = 64;
	requiredLimits.limits.maxComputeWorkgroupsPerDimension = 2;
	requiredLimits.limits.maxStorageBufferBindingSize = 0;
	requiredLimits.limits.maxStorageTexturesPerShaderStage = 1;

	// Create device
	DeviceDescriptor deviceDesc{};
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
	ImGui_ImplWGPU_Init(m_device, 3, m_swapChainFormat, TextureFormat::Undefined);
}

void Application::terminateGui() {
	ImGui_ImplWGPU_Shutdown();
	ImGui_ImplGlfw_Shutdown();
}

void Application::initBuffers() {
	BufferDescriptor desc;
	desc.label = "Uniforms";
	desc.mappedAtCreation = false;
	desc.size = sizeof(Uniforms);
	desc.usage = BufferUsage::CopyDst | BufferUsage::Uniform;
	m_uniformBuffer = m_device.createBuffer(desc);
}

void Application::terminateBuffers() {
	m_uniformBuffer.destroy();
	wgpuBufferRelease(m_uniformBuffer);
}

void Application::initTextures() {
	switch (m_settings.mode) {
	case Mode::EquirectToCubemap:
		initTexturesEquirectToCubemap();
		break;
	case Mode::CubemapToEquirect:
		initTexturesCubemapToEquirect();
		break;
	default:
		assert(false);
	}
}

void Application::initTexturesEquirectToCubemap() {
	// Load image data
	int width, height, channels;
	uint8_t* pixelData = stbi_load(equirectangularPath.string().c_str(), &width, &height, &channels, 4 /* force 4 channels */);
	if (nullptr == pixelData) throw std::runtime_error("Could not load input texture!");
	Extent3D equirectangulareSize = { (uint32_t)width, (uint32_t)height, 1 };

	// Create equirectangular texture
	TextureDescriptor textureDesc;
	textureDesc.dimension = TextureDimension::_2D;
	textureDesc.format = TextureFormat::RGBA8Unorm;
	textureDesc.size = equirectangulareSize;
	textureDesc.sampleCount = 1;
	textureDesc.viewFormatCount = 0;
	textureDesc.viewFormats = nullptr;
	textureDesc.mipLevelCount = 1;

	textureDesc.label = "Equirectangular";
	textureDesc.usage = (
		TextureUsage::TextureBinding | // to bind the texture in a shader
		TextureUsage::CopyDst // to upload the input data
	);
	m_equirectangularTexture = m_device.createTexture(textureDesc);

	// Upload texture data for MIP level 0 to the GPU
	ImageCopyTexture destination;
	destination.texture = m_equirectangularTexture;
	destination.origin = { 0, 0, 0 };
	destination.aspect = TextureAspect::All;
	destination.mipLevel = 0;
	TextureDataLayout source;
	source.offset = 0;
	source.bytesPerRow = 4 * equirectangulareSize.width;
	source.rowsPerImage = equirectangulareSize.height;
	m_queue.writeTexture(destination, pixelData, (size_t)(4 * width * height), source, equirectangulareSize);

	// Free CPU-side data
	stbi_image_free(pixelData);

	// Create cubemap texture
	uint32_t size = (uint32_t)std::pow(2, m_settings.outputSizeLog);
	textureDesc.size = { size, size, 6 };
	textureDesc.label = "Cubemap";
	textureDesc.usage = (
		TextureUsage::TextureBinding | // to bind the texture in a shader
		TextureUsage::StorageBinding | // to write the texture in a shader
		TextureUsage::CopySrc // to save the output data
	);
	m_cubemapTexture = m_device.createTexture(textureDesc);
}

void Application::initTexturesCubemapToEquirect() {
	// Load image data
	Extent3D cubemapSize = { 0, 0, 6 };
	std::array<uint8_t*, 6> pixelData;
	for (uint32_t layer = 0; layer < 6; ++layer) {
		int width, height, channels;
		pixelData[layer] = stbi_load(cubemapPaths[layer].string().c_str(), &width, &height, &channels, 4 /* force 4 channels */);
		if (nullptr == pixelData[layer]) throw std::runtime_error("Could not load input texture!");
		if (layer == 0) {
			cubemapSize.width = (uint32_t)width;
			cubemapSize.height = (uint32_t)height;
		}
		else {
			if (cubemapSize.width != (uint32_t)width || cubemapSize.height != (uint32_t)height)
				throw std::runtime_error("All cubemap faces must have the same size!");
		}
	}

	// Create cubemap texture
	TextureDescriptor textureDesc;
	textureDesc.dimension = TextureDimension::_2D;
	textureDesc.format = TextureFormat::RGBA8Unorm;
	textureDesc.size = cubemapSize;
	textureDesc.sampleCount = 1;
	textureDesc.viewFormatCount = 0;
	textureDesc.viewFormats = nullptr;
	textureDesc.mipLevelCount = 1;

	textureDesc.label = "Cubemap";
	textureDesc.usage = (
		TextureUsage::TextureBinding | // to bind the texture in a shader
		TextureUsage::CopyDst // to upload the input data
		);
	 m_cubemapTexture = m_device.createTexture(textureDesc);

	 // Upload texture data for MIP level 0 to the GPU
	 ImageCopyTexture destination;
	 destination.texture = m_cubemapTexture;
	 destination.aspect = TextureAspect::All;
	 destination.mipLevel = 0;

	 TextureDataLayout source;
	 source.offset = 0;
	 source.bytesPerRow = 4 * cubemapSize.width;
	 source.rowsPerImage = cubemapSize.height;

	 Extent3D cubemapLayerSize = { cubemapSize.width , cubemapSize.height , 1 };
	 for (uint32_t layer = 0; layer < 6; ++layer) {
		 destination.origin = { 0, 0, layer };
		 m_queue.writeTexture(destination, pixelData[layer], (size_t)(4 * cubemapSize.width * cubemapSize.height), source, cubemapLayerSize);

		 // Free CPU-side data
		 stbi_image_free(pixelData[layer]);
	 }

	// Create equirectangular texture
	uint32_t size = (uint32_t)std::pow(2, m_settings.outputSizeLog);
	textureDesc.size = { 2 * size, size, 1 };
	textureDesc.label = "Equirectangular";
	textureDesc.usage = (
		TextureUsage::TextureBinding | // to bind the texture in a shader
		TextureUsage::StorageBinding | // to write the texture in a shader
		TextureUsage::CopySrc // to save the output data
		);
	m_equirectangularTexture = m_device.createTexture(textureDesc);
}

void Application::terminateTextures() {
	m_equirectangularTexture.destroy();
	wgpuTextureRelease(m_equirectangularTexture);

	m_cubemapTexture.destroy();
	wgpuTextureRelease(m_cubemapTexture);
}

void Application::initTextureViews() {
	TextureViewDescriptor textureViewDesc;
	textureViewDesc.aspect = TextureAspect::All;
	textureViewDesc.baseArrayLayer = 0;
	textureViewDesc.arrayLayerCount = 1;
	textureViewDesc.dimension = TextureViewDimension::_2D;
	textureViewDesc.format = TextureFormat::RGBA8Unorm;
	textureViewDesc.mipLevelCount = 1;
	textureViewDesc.baseMipLevel = 0;

	textureViewDesc.label = "Equirectangular";
	m_equirectangularTextureView = m_equirectangularTexture.createView(textureViewDesc);

	const char* outputLabels[] = {
		"CubeMap Positive X",
		"CubeMap Negative X",
		"CubeMap Positive Y",
		"CubeMap Negative Y",
		"CubeMap Positive Z",
		"CubeMap Negative Z",
	};

	for (uint32_t i = 0; i < 6; ++i) {
		textureViewDesc.label = outputLabels[i];
		textureViewDesc.baseArrayLayer = i;
		m_cubemapTextureLayers[i] = m_cubemapTexture.createView(textureViewDesc);
	}

	textureViewDesc.baseArrayLayer = 0;
	textureViewDesc.arrayLayerCount = 6;
	textureViewDesc.dimension = m_settings.mode == Mode::EquirectToCubemap ? TextureViewDimension::_2DArray : TextureViewDimension::Cube;
	m_cubemapTextureView = m_cubemapTexture.createView(textureViewDesc);
}

void Application::terminateTextureViews() {
	wgpuTextureViewRelease(m_equirectangularTextureView);
	wgpuTextureViewRelease(m_cubemapTextureView);
	for (TextureView v : m_cubemapTextureLayers) {
		wgpuTextureViewRelease(v);
	}
}

void Application::initSampler() {
	SamplerDescriptor desc;
	desc.magFilter = FilterMode::Linear;
	desc.minFilter = FilterMode::Linear;
#ifdef WEBGPU_BACKEND_WGPU
	desc.mipmapFilter = MipmapFilterMode::Linear;
#else
	desc.mipmapFilter = FilterMode::Linear;
#endif
	desc.maxAnisotropy = 16;
	m_sampler = m_device.createSampler(desc);
}

void Application::terminateSampler() {
	wgpuSamplerRelease(m_sampler);
}

void Application::initBindGroup() {
	// Create compute bind group
	std::vector<BindGroupEntry> entries(4, Default);

	// Equirectangular texture
	entries[0].binding = m_settings.mode == Mode::EquirectToCubemap ? 0 : 2;
	entries[0].textureView = m_equirectangularTextureView;

	// CubeMap texture
	entries[1].binding = m_settings.mode == Mode::EquirectToCubemap ? 3 : 1;
	entries[1].textureView = m_cubemapTextureView;

	// Uniforms
	entries[2].binding = 4;
	entries[2].buffer = m_uniformBuffer;
	entries[2].offset = 0;
	entries[2].size = sizeof(Uniforms);

	// Uniforms
	entries[3].binding = 5;
	entries[3].sampler = m_sampler;

	BindGroupDescriptor bindGroupDesc;
	bindGroupDesc.layout = m_bindGroupLayout;
	bindGroupDesc.entryCount = (uint32_t)entries.size();
	bindGroupDesc.entries = (WGPUBindGroupEntry*)entries.data();
	m_bindGroup = m_device.createBindGroup(bindGroupDesc);
}

void Application::terminateBindGroup() {
	wgpuBindGroupRelease(m_bindGroup);
}

void Application::initBindGroupLayout() {
	// Create bind group layout
	std::vector<BindGroupLayoutEntry> bindings(4, Default);

	switch (m_settings.mode) {
	case Mode::EquirectToCubemap:
		// Equidirectional map as input
		bindings[0].binding = 0;
		bindings[0].texture.sampleType = TextureSampleType::Float;
		bindings[0].texture.viewDimension = TextureViewDimension::_2D;
		bindings[0].visibility = ShaderStage::Compute;

		// CubeMap as output
		bindings[1].binding = 3;
		bindings[1].storageTexture.access = StorageTextureAccess::WriteOnly;
		bindings[1].storageTexture.format = TextureFormat::RGBA8Unorm;
		bindings[1].storageTexture.viewDimension = TextureViewDimension::_2DArray;
		bindings[1].visibility = ShaderStage::Compute;
		break;
	case Mode::CubemapToEquirect:
		// Equidirectional as output
		bindings[0].binding = 2;
		bindings[0].storageTexture.access = StorageTextureAccess::WriteOnly;
		bindings[0].storageTexture.format = TextureFormat::RGBA8Unorm;
		bindings[0].storageTexture.viewDimension = TextureViewDimension::_2D;
		bindings[0].visibility = ShaderStage::Compute;

		// CubeMap as input
		bindings[1].binding = 1;
		bindings[1].texture.sampleType = TextureSampleType::Float;
		bindings[1].texture.viewDimension = TextureViewDimension::Cube;
		bindings[1].visibility = ShaderStage::Compute;
		break;
	default:
		assert(false);
	}

	// Uniforms
	bindings[2].binding = 4;
	bindings[2].buffer.type = BufferBindingType::Uniform;
	bindings[2].buffer.minBindingSize = sizeof(Uniforms);
	bindings[2].visibility = ShaderStage::Compute;

	// Uniforms
	bindings[3].binding = 5;
	bindings[3].sampler.type = SamplerBindingType::Filtering;
	bindings[3].visibility = ShaderStage::Compute;

	BindGroupLayoutDescriptor bindGroupLayoutDesc;
	bindGroupLayoutDesc.entryCount = (uint32_t)bindings.size();
	bindGroupLayoutDesc.entries = bindings.data();
	m_bindGroupLayout = m_device.createBindGroupLayout(bindGroupLayoutDesc);
}

void Application::terminateBindGroupLayout() {
	wgpuBindGroupLayoutRelease(m_bindGroupLayout);
}

void Application::initComputePipeline() {
	// Load compute shader
	ShaderModule computeShaderModule = ResourceManager::loadShaderModule(RESOURCE_DIR "/compute-shader.wgsl", m_device);

	// Create compute pipeline layout
	PipelineLayoutDescriptor pipelineLayoutDesc;
	pipelineLayoutDesc.bindGroupLayoutCount = 1;
	pipelineLayoutDesc.bindGroupLayouts = (WGPUBindGroupLayout*)&m_bindGroupLayout;
	m_pipelineLayout = m_device.createPipelineLayout(pipelineLayoutDesc);

	// Create compute pipeline
	ComputePipelineDescriptor computePipelineDesc;
	computePipelineDesc.compute.constantCount = 0;
	computePipelineDesc.compute.constants = nullptr;
	computePipelineDesc.compute.module = computeShaderModule;
	computePipelineDesc.layout = m_pipelineLayout;
	switch (m_settings.mode) {
	case Mode::EquirectToCubemap:
		computePipelineDesc.compute.entryPoint = "computeCubeMap";
		break;
	case Mode::CubemapToEquirect:
		computePipelineDesc.compute.entryPoint = "computeEquirectangular";
		break;
	default:
		assert(false);
	}
	m_pipeline = m_device.createComputePipeline(computePipelineDesc);
}

void Application::terminateComputePipeline() {
	wgpuComputePipelineRelease(m_pipeline);
	wgpuPipelineLayoutRelease(m_pipelineLayout);
}

void Application::onFrame() {
	glfwPollEvents();

	TextureView nextTexture = m_swapChain.getCurrentTextureView();
	if (!nextTexture) {
		std::cerr << "Cannot acquire next swap chain texture" << std::endl;
		return;
	}

	RenderPassDescriptor renderPassDesc = Default;
	WGPURenderPassColorAttachment renderPassColorAttachment{};
	renderPassColorAttachment.view = nextTexture;
	renderPassColorAttachment.resolveTarget = nullptr;
	renderPassColorAttachment.loadOp = WGPULoadOp_Clear;
	renderPassColorAttachment.storeOp = WGPUStoreOp_Store;
	renderPassColorAttachment.clearValue = WGPUColor{ 0.0, 0.0, 0.0, 1.0 };
	renderPassDesc.colorAttachmentCount = 1;
	renderPassDesc.colorAttachments = &renderPassColorAttachment;

	CommandEncoder encoder = m_device.createCommandEncoder(Default);
	RenderPassEncoder renderPass = encoder.beginRenderPass(renderPassDesc);
	onGui(renderPass);
	renderPass.end();

	CommandBuffer command = encoder.finish(CommandBufferDescriptor{});
	m_queue.submit(command);

	m_swapChain.present();
#if !defined(WEBGPU_BACKEND_WGPU)
	wgpuCommandBufferRelease(command);
	wgpuCommandEncoderRelease(encoder);
	wgpuRenderPassEncoderRelease(renderPass);
#endif

	wgpuTextureViewRelease(nextTexture);
#ifdef WEBGPU_BACKEND_WGPU
	wgpuQueueSubmit(m_queue, 0, nullptr);
#else
	wgpuDeviceTick(m_device);
#endif
}

void Application::onGui(RenderPassEncoder renderPass) {
	ImGui_ImplWGPU_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	// Display images
	{
		ImDrawList* drawList = ImGui::GetBackgroundDrawList();
		float offset = 0.0f;
		float s = m_cubemapTexture.getWidth() * m_settings.scale;

		// Equirectangular image
		drawList->AddImage((ImTextureID)m_equirectangularTextureView, { 0, 0 }, {
			m_equirectangularTexture.getWidth()* m_settings.scale,
			m_equirectangularTexture.getHeight() * m_settings.scale
		});
		offset += m_equirectangularTexture.getWidth() * m_settings.scale;

		// CubeMap faces
		ImTextureID view;

		// Same as drawList->AddImage but with U and V flipped
		auto AddFlippedImage = [drawList](ImTextureID user_texture_id, const ImVec2& p_min, const ImVec2& p_max, const ImVec2& uv_min, const ImVec2& uv_max) {
			drawList->AddImageQuad(user_texture_id, p_min, { p_max.x, p_min.y }, { p_max.x, p_max.y }, { p_min.x, p_max.y }, { uv_min.y, uv_min.x }, { uv_min.y, uv_max.x }, { uv_max.y, uv_max.x }, { uv_max.y, uv_min.x });
		};

		view = (ImTextureID)m_cubemapTextureLayers[(int)CubeFace::NegativeY];
		drawList->AddImage(view, { offset + 0 * s, s }, { offset + 1 * s, 2 * s }, { 0, 0 }, {1, 1});

		view = (ImTextureID)m_cubemapTextureLayers[(int)CubeFace::PositiveX];
		AddFlippedImage(view, { offset + 1 * s, s }, { offset + 2 * s, 2 * s }, { 1, 0 }, { 0, 1 });

		view = (ImTextureID)m_cubemapTextureLayers[(int)CubeFace::PositiveY];
		drawList->AddImage(view, { offset + 2 * s, s }, { offset + 3 * s, 2 * s }, { 1, 1 }, { 0, 0 });

		view = (ImTextureID)m_cubemapTextureLayers[(int)CubeFace::NegativeX];
		AddFlippedImage(view, { offset + 3 * s, s }, { offset + 4 * s, 2 * s }, { 0, 1 }, { 1, 0 });

		view = (ImTextureID)m_cubemapTextureLayers[(int)CubeFace::PositiveZ];
		AddFlippedImage(view, { offset + 1 * s, 0 * s }, { offset + 2 * s, 1 * s }, { 1, 0 }, { 0, 1 });

		view = (ImTextureID)m_cubemapTextureLayers[(int)CubeFace::NegativeZ];
		AddFlippedImage(view, { offset + 1 * s, 2 * s }, { offset + 2 * s,  3 * s }, { 1, 0 }, { 0, 1 });
	}

	bool changed = false;
	ImGui::Begin("Parameters");
	float minimum = m_parameters.normalize  ? 0.0f : -2.0f;
	float maximum = m_parameters.normalize ? 4.0f : 2.0f;
	changed = ImGui::Combo("Filter Type", (int*)&m_parameters.filterType, "Sum\0Maximum\0Minimum\0") || changed;
	changed = ImGui::SliderFloat3("Kernel X", glm::value_ptr(m_parameters.kernel[0]), minimum, maximum) || changed;
	changed = ImGui::SliderFloat3("Kernel Y", glm::value_ptr(m_parameters.kernel[1]), minimum, maximum) || changed;
	changed = ImGui::SliderFloat3("Kernel Z", glm::value_ptr(m_parameters.kernel[2]), minimum, maximum) || changed;
	changed = ImGui::Checkbox("Normalize", &m_parameters.normalize) || changed;
	ImGui::End();

	if (changed) {
		float sum = dot(vec4(1.0, 1.0, 1.0, 0.0), m_parameters.kernel * vec3(1.0));
		m_uniforms.kernel = m_parameters.normalize && std::abs(sum) > 1e-6
			? m_parameters.kernel / sum
			: m_parameters.kernel;
		m_uniforms.filterType = (uint32_t)m_parameters.filterType;
	}
	m_shouldCompute = m_shouldCompute || changed;

	ImGui::Begin("Settings");
	if (ImGui::Combo("Mode", (int*)&m_settings.mode, "EquirectToCubemap\0CubemapToEquirect\0")) {
		m_shouldRebuildPipeline = true;
		m_shouldCompute = true;
	}
	ImGui::SliderFloat("Scale", &m_settings.scale, 0.0f, 2.0f);
	if (ImGui::SliderInt("Output Size (log)", &m_settings.outputSizeLog, 2, 11)) {
		m_shouldReallocateTextures = true;
		m_shouldCompute = true;
	}
	if (ImGui::Button("Save Output")) {
		switch (m_settings.mode) {
		case Mode::EquirectToCubemap:
			for (uint32_t layer = 0; layer < 6; ++layer) {
				saveTexture(cubemapPaths[layer], m_device, m_cubemapTexture, 0, layer);
			}
			break;
		case Mode::CubemapToEquirect:
			saveTexture(equirectangularPath, m_device, m_equirectangularTexture, 0);
			break;
		default:
			assert(false);
		}
	}
	ImGui::End();

	ImGui::Render();
	ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), renderPass);
}

void Application::onCompute() {
	std::cout << "Computing..." << std::endl;

	if (m_shouldRebuildPipeline) {
		terminateBindGroup();
		terminateTextureViews();
		terminateTextures();
		terminateComputePipeline();
		terminateBindGroupLayout();
		initBindGroupLayout();
		initComputePipeline();
		initTextures();
		initTextureViews();
		initBindGroup();
		m_shouldRebuildPipeline = false;
		m_shouldReallocateTextures = false;
	}

	if (m_shouldReallocateTextures) {
		terminateBindGroup();
		terminateTextureViews();
		terminateTextures();
		initTextures();
		initTextureViews();
		initBindGroup();
		m_shouldReallocateTextures = false;
	}

	// Update uniforms
	m_queue.writeBuffer(m_uniformBuffer, 0, &m_uniforms, sizeof(Uniforms));

	// Initialize a command encoder
	CommandEncoderDescriptor encoderDesc = Default;
	CommandEncoder encoder = m_device.createCommandEncoder(encoderDesc);

	// Create compute pass
	ComputePassDescriptor computePassDesc;
	computePassDesc.timestampWriteCount = 0;
	computePassDesc.timestampWrites = nullptr;
	ComputePassEncoder computePass = encoder.beginComputePass(computePassDesc);

	computePass.setPipeline(m_pipeline);

	computePass.setBindGroup(0, m_bindGroup, 0, nullptr);
	uint32_t invocationCountX = 0, invocationCountY = 0, workgroupSizePerDim = 0;
	switch (m_settings.mode) {
	case Mode::EquirectToCubemap:
		invocationCountX = m_cubemapTexture.getWidth();
		invocationCountY = m_cubemapTexture.getWidth();
		workgroupSizePerDim = 4;
		break;
	case Mode::CubemapToEquirect:
		invocationCountX = m_equirectangularTexture.getWidth();
		invocationCountY = m_equirectangularTexture.getWidth();
		workgroupSizePerDim = 8;
		break;
	default:
		assert(false);
	}
	// This ceils invocationCountX / workgroupSizePerDim
	uint32_t workgroupCountX = (invocationCountX + workgroupSizePerDim - 1) / workgroupSizePerDim;
	uint32_t workgroupCountY = (invocationCountY + workgroupSizePerDim - 1) / workgroupSizePerDim;
	computePass.dispatchWorkgroups(workgroupCountX, workgroupCountY, 1);

	// Finalize compute pass
	computePass.end();

	// Encode and submit the GPU commands
	CommandBuffer commands = encoder.finish(CommandBufferDescriptor{});
	m_queue.submit(commands);

#if !defined(WEBGPU_BACKEND_WGPU)
	wgpuCommandBufferRelease(commands);
	wgpuCommandEncoderRelease(encoder);
	wgpuComputePassEncoderRelease(computePass);
#endif

	m_shouldCompute = false;
}

void Application::onResize() {
	terminateSwapChain();
	initSwapChain();
}
