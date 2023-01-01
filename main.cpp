/**
 * This file is part of the "Learn WebGPU for C++" book.
 *   https://github.com/eliemichel/LearnWebGPU
 * 
 * MIT License
 * Copyright (c) 2022 Elie Michel
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

#include "glfw3webgpu.h"

#include <GLFW/glfw3.h>

#define WEBGPU_CPP_IMPLEMENTATION
#include <webgpu.hpp>

#include <wgpu.h> // wgpuTextureViewDrop

#include <iostream>
#include <cassert>
#include <array>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

using namespace wgpu;
namespace fs = std::filesystem;

/**
 * A structure holding the value of our uniforms
 */
struct MyUniforms {
	std::array<float, 4> color;
	std::array<float, 2> resolution;
	float time;
	float _pad[1];
};
// Have the compiler check byte alignment
static_assert(sizeof(MyUniforms) % 16 == 0);

bool loadGeometry(const fs::path& path, std::vector<float>& pointData, std::vector<uint16_t>& indexData);

int main (int, char**) {
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
	// Don't forget to = Default
	RequiredLimits requiredLimits = Default;
	// We use at most 1 bind group for now
	requiredLimits.limits.maxBindGroups = 1;
	requiredLimits.limits.maxUniformBuffersPerShaderStage = 1;
	requiredLimits.limits.maxDynamicUniformBuffersPerPipelineLayout = 0;
	requiredLimits.limits.maxUniformBufferBindingSize = 16 * 4;

	requiredLimits.limits.maxVertexAttributes = 2;
	requiredLimits.limits.maxVertexBuffers = 1;

	DeviceDescriptor deviceDesc{};
	deviceDesc.label = "My Device";
	deviceDesc.requiredFeaturesCount = 0;
	// We specify required limits here
	deviceDesc.requiredLimits = &requiredLimits;
	deviceDesc.defaultQueue.label = "The default queue";
	Device device = adapter.requestDevice(deviceDesc);
	std::cout << "Got device: " << device << std::endl;

	// Display supported limits
	SupportedLimits supportedLimits;
	adapter.getLimits(&supportedLimits);
	std::cout << "adapter.maxBindGroups: " << supportedLimits.limits.maxBindGroups << std::endl;
	std::cout << "adapter.minUniformBufferOffsetAlignment: " << supportedLimits.limits.minUniformBufferOffsetAlignment << std::endl;
	device.getLimits(&supportedLimits);
	std::cout << "device.maxBindGroups: " << supportedLimits.limits.maxBindGroups << std::endl;
	std::cout << "device.minUniformBufferOffsetAlignment: " << supportedLimits.limits.minUniformBufferOffsetAlignment << std::endl;
	Limits deviceLimits = supportedLimits.limits;

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
	const char* shaderSource = R"(
/**
 * A structure holding the value of our uniforms
 */
struct MyUniforms {
    color: vec4<f32>,
	resolution: vec2<f32>,
	time: f32,
};

struct VertexInput {
	@location(0) position: vec2<f32>,
	@location(1) color: vec3<f32>,
};

struct VertexOutput {
	@builtin(position) position: vec4<f32>,
	@location(0) color: vec3<f32>,
};

@group(0) @binding(0) var<uniform> uMyUniforms: MyUniforms;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
	var out: VertexOutput;
	var p = (in.position - vec2<f32>(0.6875, 0.463)) * vec2<f32>(1.0, uMyUniforms.resolution.x / uMyUniforms.resolution.y);
	out.position = vec4<f32>(p, 0.0, 1.0);
	out.color = in.color;
	return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4<f32> {
	return vec4<f32>(pow(in.color, vec3<f32>(2.2)), 1.0);
}
)";

	ShaderModuleWGSLDescriptor shaderCodeDesc{};
	shaderCodeDesc.chain.next = nullptr;
	shaderCodeDesc.chain.sType = SType::ShaderModuleWGSLDescriptor;
	shaderCodeDesc.code = shaderSource;

	ShaderModuleDescriptor shaderDesc{};
	shaderDesc.nextInChain = &shaderCodeDesc.chain;
	shaderDesc.hintCount = 0;
	shaderDesc.hints = nullptr;

	ShaderModule shaderModule = device.createShaderModule(shaderDesc);
	std::cout << "Shader module: " << shaderModule << std::endl;

	std::cout << "Creating render pipeline..." << std::endl;
	RenderPipelineDescriptor pipelineDesc{};

	std::vector<VertexAttribute> vertexAttribs(2);

	// Position attribute
	vertexAttribs[0].shaderLocation = 0;
	vertexAttribs[0].format = VertexFormat::Float32x2;
	vertexAttribs[0].offset = 0;

	// Color attribute
	vertexAttribs[1].shaderLocation = 1;
	vertexAttribs[1].format = VertexFormat::Float32x3;
	vertexAttribs[1].offset = 2 * sizeof(float);

	VertexBufferLayout vertexBufferLayout;
	vertexBufferLayout.arrayStride = 5 * sizeof(float);
	vertexBufferLayout.attributeCount = static_cast<uint32_t>(vertexAttribs.size());
	vertexBufferLayout.attributes = vertexAttribs.data();
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
	
	pipelineDesc.depthStencil = nullptr;

	pipelineDesc.multisample.count = 1;
	pipelineDesc.multisample.mask = ~0u;
	pipelineDesc.multisample.alphaToCoverageEnabled = false;

	// Create uniform buffer
	BufferDescriptor bufferDesc{};
	bufferDesc.size = sizeof(MyUniforms);
	bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Uniform;
	bufferDesc.mappedAtCreation = false;
	Buffer uniformBuffer = device.createBuffer(bufferDesc);

	MyUniforms uniforms;
	uniforms.time = 1.0f;
	uniforms.color = { 0.0f, 1.0f, 0.4f, 1.0f };
	uniforms.resolution = { 640.0f, 480.0f };
	queue.writeBuffer(uniformBuffer, 0, &uniforms, sizeof(MyUniforms));

	// Define geometry
	// The de-duplicated list of point positions
	std::vector<float> pointData = {
		// x, y, r, g, b
		-0.5, -0.5, 1.0, 0.0, 0.0,
		+0.5, -0.5, 0.0, 1.0, 0.0,
		+0.5, +0.5, 0.0, 0.0, 1.0,
		-0.5, +0.5, 1.0, 1.0, 0.0
	};

	// This is a list of indices referencing positions in the pointData
	std::vector<uint16_t> indexData = {
		0, 1, 2, // Triangle #0
		0, 2, 3  // Triangle #1
	};

	bool success = loadGeometry("../data/webgpu.txt", pointData, indexData);
	if (!success) {
		std::cerr << "Could not load geometry!" << std::endl;
		return 1;
	}

	int indexCount = static_cast<int>(indexData.size());
	// Align
	indexData.resize((size_t)ceil(indexData.size() / (float)4) * 4);

	// Create vertex buffer
	bufferDesc.size = pointData.size() * sizeof(float);
	bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Vertex;
	bufferDesc.mappedAtCreation = false;
	Buffer vertexBuffer = device.createBuffer(bufferDesc);

	// Upload geometry data to the buffer
	queue.writeBuffer(vertexBuffer, 0, pointData.data(), bufferDesc.size);

	// Create index buffer
	bufferDesc.size = indexData.size() * sizeof(uint16_t);
	bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Index;
	bufferDesc.mappedAtCreation = false;
	Buffer indexBuffer = device.createBuffer(bufferDesc);

	// Upload geometry data to the buffer
	queue.writeBuffer(indexBuffer, 0, indexData.data(), bufferDesc.size);

	// Create binding layouts
	BindGroupLayoutEntry bindingLayout = Default;
	// The binding index as used in the @binding attribute in the shader
	bindingLayout.binding = 0;
	// The stage that needs to access this resource
	bindingLayout.visibility = ShaderStage::Vertex | ShaderStage::Fragment;
	// The bindin is for a uniform buffer
	bindingLayout.buffer.type = BufferBindingType::Uniform;
	// The uniform buffer has the length of MyUniforms
	bindingLayout.buffer.minBindingSize = sizeof(MyUniforms);
	// Make this binding dynamic so we can offset it between draw calls
	bindingLayout.buffer.hasDynamicOffset = false;

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

	// Create bindings
	BindGroupEntry binding;

	// The index of the binding (the entries in bindGroupDesc can be in any order)
	binding.binding = 0;
	// The buffer it is actually bound to
	binding.buffer = uniformBuffer;
	// We can specify an offset within the buffer, so that a single buffer can hold
	// multiple uniform blocks.
	// NB: The dynamic offset is added to this value when setBindGroup() is called
	binding.offset = 0;
	// And we specify the size of 1 uniform struct.
	binding.size = sizeof(MyUniforms);

	// A bind group contains one or multiple bindings
	BindGroupDescriptor bindGroupDesc{};
	bindGroupDesc.layout = bindGroupLayout;
	// There must be as many bindings as declared in the layout!
	bindGroupDesc.entryCount = bindGroupLayoutDesc.entryCount;
	bindGroupDesc.entries = &binding;
	BindGroup bindGroup = device.createBindGroup(bindGroupDesc);

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		// Update the time uniform
		uniforms.time = static_cast<float>(glfwGetTime());
		queue.writeBuffer(uniformBuffer, offsetof(MyUniforms, time), &uniforms.time, sizeof(MyUniforms::time));

		// Update the color uniform
		uniforms.color = { 1.0f, 0.5f, 0.0f, 1.0f };
		queue.writeBuffer(uniformBuffer, offsetof(MyUniforms, color), &uniforms.color, sizeof(MyUniforms::color));

		TextureView nextTexture = swapChain.getCurrentTextureView();
		if (!nextTexture) {
			std::cerr << "Cannot acquire next swap chain texture" << std::endl;
			return 1;
		}

		CommandEncoderDescriptor commandEncoderDesc{};
		commandEncoderDesc.label = "Command Encoder";
		CommandEncoder encoder = device.createCommandEncoder(commandEncoderDesc);
		
		RenderPassDescriptor renderPassDesc{};

		WGPURenderPassColorAttachment renderPassColorAttachment = {};
		renderPassColorAttachment.view = nextTexture;
		renderPassColorAttachment.resolveTarget = nullptr;
		renderPassColorAttachment.loadOp = LoadOp::Clear;
		renderPassColorAttachment.storeOp = StoreOp::Store;
		renderPassColorAttachment.clearValue = Color{ 0.025, 0.025, 0.025, 1.0 };
		renderPassDesc.colorAttachmentCount = 1;
		renderPassDesc.colorAttachments = &renderPassColorAttachment;

		renderPassDesc.depthStencilAttachment = nullptr;
		renderPassDesc.timestampWriteCount = 0;
		renderPassDesc.timestampWrites = nullptr;

		RenderPassEncoder renderPass = encoder.beginRenderPass(renderPassDesc);

		renderPass.setPipeline(pipeline);

		// Set vertex buffer while encoding the render pass
		renderPass.setVertexBuffer(0, vertexBuffer, 0, pointData.size() * sizeof(float));
		renderPass.setIndexBuffer(indexBuffer, IndexFormat::Uint16, 0, indexData.size() * sizeof(uint16_t));

		// Set binding group
		renderPass.setBindGroup(0, bindGroup, 0, nullptr);
		renderPass.drawIndexed(indexCount, 1, 0, 0, 0);

		renderPass.end();

		CommandBufferDescriptor cmdBufferDescriptor{};
		cmdBufferDescriptor.label = "Command buffer";
		CommandBuffer command = encoder.finish(cmdBufferDescriptor);
		queue.submit(command);

		wgpuTextureViewDrop(nextTexture);
		swapChain.present();
	}

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}

bool loadGeometry(const fs::path& path, std::vector<float>& pointData, std::vector<uint16_t>& indexData) {
	std::ifstream file(path);
	if (!file.is_open()) {
		return false;
	}

	pointData.clear();
	indexData.clear();

	enum class Section {
		None,
		Points,
		Indices,
	};
	Section currentSection = Section::None;

	float value;
	uint16_t index;
	std::string line;
	while (!file.eof()) {
		getline(file, line);
		if (line == "[points]") {
			currentSection = Section::Points;
		}
		else if (line == "[indices]") {
			currentSection = Section::Indices;
		}
		else if (line[0] == '#' || line.empty()) {
			// Do nothing, this is a comment
		}
		else if (currentSection == Section::Points) {
			std::istringstream iss(line);
			// Get x, y, r, g, b
			for (int i = 0; i < 5; ++i) {
				iss >> value;
				pointData.push_back(value);
			}
		}
		else if (currentSection == Section::Indices) {
			std::istringstream iss(line);
			// Get corners #0 #1 and #2
			for (int i = 0; i < 3; ++i) {
				iss >> index;
				indexData.push_back(index);
			}
		}
	}
	return true;
}
