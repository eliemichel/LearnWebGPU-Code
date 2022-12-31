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

using namespace wgpu;

/**
 * A structure holding the value of our uniforms
 */
struct MyUniforms {
	std::array<float, 4> color;
	float time;
	//float _pad[3];
};

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
	requiredLimits.limits.minUniformBufferOffsetAlignment = 64;

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
	time: f32,
};

// Instead of the simple uTime variable, our uniform variable is a struct
@group(0) @binding(0) var<uniform> uMyUniforms: MyUniforms;

@vertex
fn vs_main(@builtin(vertex_index) in_vertex_index: u32) -> @builtin(position) vec4<f32> {
	var p = vec2<f32>(0.0, 0.0);
	if (in_vertex_index == 0u) {
		p = vec2<f32>(-0.5, -0.5);
	} else if (in_vertex_index == 1u) {
		p = vec2<f32>(0.5, -0.5);
	} else {
		p = vec2<f32>(0.0, 0.5);
	}

	// We move the object depending on the time
	p += 0.3 * vec2<f32>(cos(uMyUniforms.time), sin(uMyUniforms.time));

	return vec4<f32>(p, 0.0, 1.0);
}

@fragment
fn fs_main() -> @location(0) vec4<f32> {
    return uMyUniforms.color;
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

	pipelineDesc.vertex.bufferCount = 0;
	pipelineDesc.vertex.buffers = nullptr;
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

	// The buffer will only contain 1 float with the value of uTime
	bufferDesc.size = sizeof(MyUniforms);

	// Make sure to flag the buffer as BufferUsage::Uniform
	bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Uniform;

	bufferDesc.mappedAtCreation = false;
	Buffer uniformBuffer = device.createBuffer(bufferDesc);

	// Upload the initial value of the uniform
	MyUniforms uniforms;
	uniforms.time = 1.0f;
	uniforms.color = { 0.0f, 1.0f, 0.4f, 1.0f };
	queue.writeBuffer(uniformBuffer, 0, &uniforms, sizeof(MyUniforms));

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
	binding.offset = 0;
	// And we specify again the size of the buffer.
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

		WGPURenderPassColorAttachment renderPassColorAttachment = {};
		renderPassColorAttachment.view = nextTexture;
		renderPassColorAttachment.resolveTarget = nullptr;
		renderPassColorAttachment.loadOp = LoadOp::Clear;
		renderPassColorAttachment.storeOp = StoreOp::Store;
		renderPassColorAttachment.clearValue = Color{ 0.9, 0.1, 0.2, 1.0 };
		renderPassDesc.colorAttachmentCount = 1;
		renderPassDesc.colorAttachments = &renderPassColorAttachment;

		renderPassDesc.depthStencilAttachment = nullptr;
		renderPassDesc.timestampWriteCount = 0;
		renderPassDesc.timestampWrites = nullptr;

		RenderPassEncoder renderPass = encoder.beginRenderPass(renderPassDesc);

		renderPass.setPipeline(pipeline);

		// Set binding group
		renderPass.setBindGroup(0, bindGroup, 0, nullptr);

		// First value of the uniforms
		uniforms.color = { 0.0f, 0.4f, 1.0f, 1.0f };
		queue.writeBuffer(uniformBuffer, offsetof(MyUniforms, color), &uniforms.color, sizeof(MyUniforms::color));

		renderPass.draw(3, 1, 0, 0);

		// Second value of the uniforms
		uniforms.time -= 1.0f;
		uniforms.color = { 1.0f, 0.5f, 0.0f, 1.0f };
		queue.writeBuffer(uniformBuffer, 0, &uniforms, sizeof(MyUniforms));

		renderPass.draw(3, 1, 0, 0);

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
