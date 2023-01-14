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

#include <glfw3webgpu.h>
#include <GLFW/glfw3.h>

#define WEBGPU_CPP_IMPLEMENTATION
#include <webgpu.hpp>
#include <wgpu.h> // wgpuTextureViewDrop

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#define TINYOBJLOADER_IMPLEMENTATION // add this to exactly 1 of your C++ files
#include "tiny_obj_loader.h"

#include <iostream>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <array>

using namespace wgpu;
namespace fs = std::filesystem;
using glm::mat4;

using tinyobj::attrib_t;

/**
 * The same structure as in the shader, replicated in C++
 */
struct MyUniforms {
	mat4 projectionMatrix;
	mat4 viewMatrix;
	mat4 modelMatrix;
	std::array<float, 4> color;
	float time;
	float _pad[3];
};
static_assert(sizeof(MyUniforms) % 16 == 0);

/**
 * A structure that describes the data layout in the vertex buffer
 * We do not instantiate it but use it in `sizeof` and `offsetof`
 */
struct VertexAttributes {
	std::array<float, 3> position;
	std::array<float, 3> normal;
	std::array<float, 3> color;
};

ShaderModule loadShaderModule(const fs::path& path, Device device);
bool loadGeometryFromObj(const fs::path& path, std::vector<VertexAttributes>& vertexData);

int main(int, char**) {
	Instance instance = createInstance(InstanceDescriptor{});
	if (!instance) {
		std::cerr << "Could not initialize WebGPU!" << std::endl;
		return 1;
	}

	if (!glfwInit()) {
		std::cerr << "Could not initialize GLFW!" << std::endl;
		return 1;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	GLFWwindow* window = glfwCreateWindow(640, 480, "Learn WebGPU", NULL, NULL);
	if (!window) {
		std::cerr << "Could not open window!" << std::endl;
		return 1;
	}

	std::cout << "Requesting adapter..." << std::endl;
	Surface surface = glfwGetWGPUSurface(instance, window);
	RequestAdapterOptions adapterOpts{};
	adapterOpts.compatibleSurface = surface;
	Adapter adapter = instance.requestAdapter(adapterOpts);
	std::cout << "Got adapter: " << adapter << std::endl;

	std::cout << "Requesting device..." << std::endl;
	RequiredLimits requiredLimits = Default;
	requiredLimits.limits.maxVertexAttributes = 3;
	requiredLimits.limits.maxVertexBuffers = 1;
	requiredLimits.limits.maxBindGroups = 1;
	requiredLimits.limits.maxUniformBuffersPerShaderStage = 1;
	requiredLimits.limits.maxUniformBufferBindingSize = 16 * 4 * sizeof(float);

	DeviceDescriptor deviceDesc{};
	deviceDesc.label = "My Device";
	deviceDesc.requiredFeaturesCount = 0;
	deviceDesc.requiredLimits = &requiredLimits;
	deviceDesc.defaultQueue.label = "The default queue";
	Device device = adapter.requestDevice(deviceDesc);
	std::cout << "Got device: " << device << std::endl;

	// Add an error callback for more debug info
	// (TODO: fix the callback in the webgpu.hpp wrapper)
	auto myCallback = [](ErrorType type, char const* message) {
		std::cout << "Device error: type " << type;
		if (message) std::cout << " (message: " << message << ")";
		std::cout << std::endl;
	};
	struct Context {
		decltype(myCallback) theCallback;
	};
	Context ctx = { myCallback };
	static auto cCallback = [](WGPUErrorType type, char const* message, void* userdata) -> void {
		Context& ctx = *reinterpret_cast<Context*>(userdata);
		ctx.theCallback(static_cast<ErrorType>(type), message);
	};
	wgpuDeviceSetUncapturedErrorCallback(device, cCallback, reinterpret_cast<void*>(&ctx));

	Queue queue = device.getQueue();

	std::cout << "Creating swapchain..." << std::endl;
	TextureFormat swapChainFormat = surface.getPreferredFormat(adapter);
	SwapChainDescriptor swapChainDesc = {};
	swapChainDesc.width = 640;
	swapChainDesc.height = 480;
	swapChainDesc.usage = TextureUsage::RenderAttachment;
	swapChainDesc.format = swapChainFormat;
	swapChainDesc.presentMode = PresentMode::Fifo;
	SwapChain swapChain = device.createSwapChain(surface, swapChainDesc);
	std::cout << "Swapchain: " << swapChain << std::endl;

	std::cout << "Creating shader module..." << std::endl;
	ShaderModule shaderModule = loadShaderModule(RESOURCE_DIR "/shader.wsl", device);
	std::cout << "Shader module: " << shaderModule << std::endl;

	std::cout << "Creating render pipeline..." << std::endl;
	RenderPipelineDescriptor pipelineDesc{};

	// Vertex fetch
	std::vector<VertexAttribute> vertexAttribs(3);

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
	colorTarget.format = swapChainFormat;
	colorTarget.blend = &blendState;
	colorTarget.writeMask = ColorWriteMask::All;

	fragmentState.targetCount = 1;
	fragmentState.targets = &colorTarget;

	// Setup the Z-Buffer algorithm options
	DepthStencilState depthStencilState = Default;
	depthStencilState.depthCompare = CompareFunction::Less;
	depthStencilState.depthWriteEnabled = true;
	TextureFormat depthTextureFormat = TextureFormat::Depth24Plus;
	depthStencilState.format = depthTextureFormat;
	depthStencilState.stencilReadMask = 0;
	depthStencilState.stencilWriteMask = 0;
	pipelineDesc.depthStencil = &depthStencilState;

	pipelineDesc.multisample.count = 1;
	pipelineDesc.multisample.mask = ~0u;
	pipelineDesc.multisample.alphaToCoverageEnabled = false;

	// Create binding layout
	BindGroupLayoutEntry bindingLayout = Default;
	bindingLayout.binding = 0;
	bindingLayout.visibility = ShaderStage::Vertex | ShaderStage::Fragment;
	bindingLayout.buffer.type = BufferBindingType::Uniform;
	bindingLayout.buffer.minBindingSize = sizeof(MyUniforms);

	// Create a bind group layout
	BindGroupLayoutDescriptor bindGroupLayoutDesc{};
	bindGroupLayoutDesc.entryCount = 1;
	bindGroupLayoutDesc.entries = &bindingLayout;
	BindGroupLayout bindGroupLayout = device.createBindGroupLayout(bindGroupLayoutDesc);

	// Create the pipeline layout
	PipelineLayoutDescriptor layoutDesc{};
	layoutDesc.bindGroupLayoutCount = 1;
	layoutDesc.bindGroupLayouts = &(WGPUBindGroupLayout)bindGroupLayout;
	PipelineLayout layout = device.createPipelineLayout(layoutDesc);
	pipelineDesc.layout = layout;

	RenderPipeline pipeline = device.createRenderPipeline(pipelineDesc);
	std::cout << "Render pipeline: " << pipeline << std::endl;

	std::vector<VertexAttributes> vertexData;

	bool success = loadGeometryFromObj(RESOURCE_DIR "/pyramid.obj", vertexData);
	if (!success) {
		std::cerr << "Could not load geometry!" << std::endl;
		return 1;
	}

	// Create vertex buffer
	BufferDescriptor bufferDesc;
	bufferDesc.size = vertexData.size() * sizeof(VertexAttributes);
	bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Vertex;
	bufferDesc.mappedAtCreation = false;
	Buffer vertexBuffer = device.createBuffer(bufferDesc);
	queue.writeBuffer(vertexBuffer, 0, vertexData.data(), bufferDesc.size);

	int indexCount = static_cast<int>(vertexData.size());

	// Create uniform buffer
	bufferDesc.size = sizeof(MyUniforms);
	bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Uniform;
	bufferDesc.mappedAtCreation = false;
	Buffer uniformBuffer = device.createBuffer(bufferDesc);

	// Upload the initial value of the uniforms
	MyUniforms uniforms;
	uniforms.time = 1.0f;
	uniforms.color = { 0.0f, 1.0f, 0.4f, 1.0f };
	uniforms.projectionMatrix = glm::perspective(45.0f, 640.0f / 480.0f, 0.001f, 100.0f);
	uniforms.viewMatrix = mat4(1.0);
	uniforms.modelMatrix = mat4(1.0);
	queue.writeBuffer(uniformBuffer, 0, &uniforms, sizeof(MyUniforms));

	// Create a binding
	BindGroupEntry binding{};
	binding.binding = 0;
	binding.buffer = uniformBuffer;
	binding.offset = 0;
	binding.size = sizeof(MyUniforms);

	BindGroupDescriptor bindGroupDesc{};
	bindGroupDesc.layout = bindGroupLayout;
	bindGroupDesc.entryCount = bindGroupLayoutDesc.entryCount;
	bindGroupDesc.entries = &binding;
	BindGroup bindGroup = device.createBindGroup(bindGroupDesc);

	// Create the depth texture
	TextureDescriptor depthTextureDesc;
	depthTextureDesc.dimension = TextureDimension::_2D;
	depthTextureDesc.format = depthTextureFormat;
	depthTextureDesc.mipLevelCount = 1;
	depthTextureDesc.sampleCount = 1;
	depthTextureDesc.size = { 640, 480, 1 };
	depthTextureDesc.usage = TextureUsage::RenderAttachment;
	depthTextureDesc.viewFormatCount = 1;
	depthTextureDesc.viewFormats = (WGPUTextureFormat*)&depthTextureFormat;
	Texture depthTexture = device.createTexture(depthTextureDesc);

	// Create the view of the depth texture manipulated by the rasterizer
	TextureViewDescriptor depthTextureViewDesc;
	depthTextureViewDesc.aspect = TextureAspect::DepthOnly;
	depthTextureViewDesc.baseArrayLayer = 0;
	depthTextureViewDesc.arrayLayerCount = 1;
	depthTextureViewDesc.baseMipLevel = 0;
	depthTextureViewDesc.mipLevelCount = 1;
	depthTextureViewDesc.dimension = TextureViewDimension::_2D;
	depthTextureViewDesc.format = depthTextureFormat;
	TextureView depthTextureView = depthTexture.createView(depthTextureViewDesc);

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		// Update uniform buffer
		uniforms.time = static_cast<float>(glfwGetTime());
		queue.writeBuffer(uniformBuffer, offsetof(MyUniforms, time), &uniforms.time, sizeof(MyUniforms::time));

		TextureView nextTexture = swapChain.getCurrentTextureView();
		if (!nextTexture) {
			std::cerr << "Cannot acquire next swap chain texture" << std::endl;
			return 1;
		}

		CommandEncoderDescriptor commandEncoderDesc{};
		commandEncoderDesc.label = "Command Encoder";
		CommandEncoder encoder = device.createCommandEncoder(commandEncoderDesc);

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
		depthStencilAttachment.view = depthTextureView;
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

		renderPass.setPipeline(pipeline);

		renderPass.setVertexBuffer(0, vertexBuffer, 0, vertexData.size() * sizeof(VertexAttributes));
		renderPass.setBindGroup(0, bindGroup, 0, nullptr);

		renderPass.draw(indexCount, 1, 0, 0);

		renderPass.end();

		wgpuTextureViewDrop(nextTexture);

		CommandBufferDescriptor cmdBufferDescriptor{};
		cmdBufferDescriptor.label = "Command buffer";
		CommandBuffer command = encoder.finish(cmdBufferDescriptor);
		queue.submit(command);

		swapChain.present();
	}

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}

ShaderModule loadShaderModule(const fs::path& path, Device device) {
	std::ifstream file(path);
	if (!file.is_open()) {
		return nullptr;
	}
	file.seekg(0, std::ios::end);
	size_t size = file.tellg();
	std::string shaderSource(size, ' ');
	file.seekg(0);
	file.read(shaderSource.data(), size);

	ShaderModuleWGSLDescriptor shaderCodeDesc{};
	shaderCodeDesc.chain.next = nullptr;
	shaderCodeDesc.chain.sType = SType::ShaderModuleWGSLDescriptor;
	shaderCodeDesc.code = shaderSource.c_str();
	ShaderModuleDescriptor shaderDesc{};
	shaderDesc.hintCount = 0;
	shaderDesc.hints = nullptr;
	shaderDesc.nextInChain = &shaderCodeDesc.chain;
	return device.createShaderModule(shaderDesc);
}

bool loadGeometryFromObj(const fs::path& path, std::vector<VertexAttributes>& vertexData) {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string warn;
	std::string err;

	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.string().c_str());

	if (!warn.empty()) {
		std::cout << warn << std::endl;
	}

	if (!err.empty()) {
		std::cerr << err << std::endl;
	}

	if (!ret) {
		return false;
	}

	if (shapes.empty()) {
		return false;
	}

	vertexData.clear();
	// for (const auto& shape : shapes) {
	const auto& shape = shapes[0];

	vertexData.resize(shape.mesh.indices.size());
	for (int i = 0; i < vertexData.size(); ++i) {
		const tinyobj::index_t& idx = shape.mesh.indices[i];

		vertexData[i].position = {
			attrib.vertices[3 * idx.vertex_index + 0],
			-attrib.vertices[3 * idx.vertex_index + 2],
			attrib.vertices[3 * idx.vertex_index + 1]
		};

		vertexData[i].normal = {
			attrib.normals[3 * idx.normal_index + 0],
			-attrib.normals[3 * idx.normal_index + 2],
			attrib.normals[3 * idx.normal_index + 1]
		};

		vertexData[i].color = {
			attrib.colors[3 * idx.vertex_index + 0],
			attrib.colors[3 * idx.vertex_index + 1],
			attrib.colors[3 * idx.vertex_index + 2]
		};
	}

	return true;
}
