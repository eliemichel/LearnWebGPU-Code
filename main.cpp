// Include the C++ wrapper instead of the raw header(s)
#define WEBGPU_CPP_IMPLEMENTATION
#include <webgpu/webgpu.hpp>

#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>

#ifdef __EMSCRIPTEN__
#  include <emscripten.h>
#endif // __EMSCRIPTEN__

#include <iostream>
#include <cassert>
#include <vector>

#include "ResourceManager.h"

using namespace wgpu;

class Application {
public:
	// Initialize everything and return true if it went all right
	bool Initialize();

	// Uninitialize everything that was initialized
	void Terminate();

	// Draw a frame and handle events
	void MainLoop();

	// Return true as long as the main loop should keep on running
	bool IsRunning();

private:
	TextureView GetNextSurfaceTextureView();

	// Substep of Initialize() that creates the render pipeline
	void InitializePipeline();
	RequiredLimits GetRequiredLimits(Adapter adapter) const;
	void InitializeBuffers();
	void InitializeBindGroups();

private:
	// We put here all the variables that are shared between init and main loop
	GLFWwindow *window;
	Device device;
	Queue queue;
	Surface surface;
	std::unique_ptr<ErrorCallback> uncapturedErrorCallbackHandle;
	TextureFormat surfaceFormat = TextureFormat::Undefined;
	RenderPipeline pipeline;
	Buffer pointBuffer;
	Buffer indexBuffer;
	Buffer uniformBuffer;
	uint32_t indexCount;
	BindGroup bindGroup;
	PipelineLayout layout;
	BindGroupLayout bindGroupLayout;
};

int main() {
	Application app;

	if (!app.Initialize()) {
		return 1;
	}

#ifdef __EMSCRIPTEN__
	// Equivalent of the main loop when using Emscripten:
	auto callback = [](void *arg) {
		Application* pApp = reinterpret_cast<Application*>(arg);
		pApp->MainLoop(); // 4. We can use the application object
	};
	emscripten_set_main_loop_arg(callback, &app, 0, true);
#else // __EMSCRIPTEN__
	while (app.IsRunning()) {
		app.MainLoop();
	}
#endif // __EMSCRIPTEN__

	app.Terminate();

	return 0;
}

bool Application::Initialize() {
	// Open window
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	window = glfwCreateWindow(640, 480, "Learn WebGPU", nullptr, nullptr);
	
	Instance instance = wgpuCreateInstance(nullptr);
	
	// Get adapter
	std::cout << "Requesting adapter..." << std::endl;
	surface = glfwGetWGPUSurface(instance, window);
	RequestAdapterOptions adapterOpts = {};
	adapterOpts.compatibleSurface = surface;
	Adapter adapter = instance.requestAdapter(adapterOpts);
	std::cout << "Got adapter: " << adapter << std::endl;
	
	instance.release();
	
	std::cout << "Requesting device..." << std::endl;
	DeviceDescriptor deviceDesc = {};
	deviceDesc.label = "My Device";
	deviceDesc.requiredFeatureCount = 0;
	deviceDesc.requiredLimits = nullptr;
	deviceDesc.defaultQueue.nextInChain = nullptr;
	deviceDesc.defaultQueue.label = "The default queue";
	deviceDesc.deviceLostCallback = [](WGPUDeviceLostReason reason, char const* message, void* /* pUserData */) {
		std::cout << "Device lost: reason " << reason;
		if (message) std::cout << " (" << message << ")";
		std::cout << std::endl;
	};
	// Before adapter.requestDevice(deviceDesc)
	RequiredLimits requiredLimits = GetRequiredLimits(adapter);
	deviceDesc.requiredLimits = &requiredLimits;
	device = adapter.requestDevice(deviceDesc);
	std::cout << "Got device: " << device << std::endl;

	// Device error callback
	uncapturedErrorCallbackHandle = device.setUncapturedErrorCallback([](ErrorType type, char const* message) {
		std::cout << "Uncaptured device error: type " << type;
		if (message) std::cout << " (" << message << ")";
		std::cout << std::endl;
	});
	
	queue = device.getQueue();

	// Configure the surface
	SurfaceConfiguration config = {};
	
	// Configuration of the textures created for the underlying swap chain
	config.width = 640;
	config.height = 480;
	config.usage = TextureUsage::RenderAttachment;
	surfaceFormat = surface.getPreferredFormat(adapter);
	config.format = surfaceFormat;

	// And we do not need any particular view format:
	config.viewFormatCount = 0;
	config.viewFormats = nullptr;
	config.device = device;
	config.presentMode = PresentMode::Fifo;
	config.alphaMode = CompositeAlphaMode::Auto;

	surface.configure(config);

	// Release the adapter only after it has been fully utilized
	adapter.release();

	InitializePipeline();
	InitializeBuffers();
	InitializeBindGroups();
	return true;
}

void Application::Terminate() {
	bindGroup.release();
	layout.release();
	bindGroupLayout.release();
	uniformBuffer.release();
	pointBuffer.release();
	indexBuffer.release();
	pipeline.release();
	surface.unconfigure();
	queue.release();
	surface.release();
	device.release();
	glfwDestroyWindow(window);
	glfwTerminate();
}

void Application::MainLoop() {
	glfwPollEvents();

	// Update uniform buffer
	float t = static_cast<float>(glfwGetTime()); // glfwGetTime returns a double
	queue.writeBuffer(uniformBuffer, 0, &t, sizeof(float));

	// Get the next target texture view
	TextureView targetView = GetNextSurfaceTextureView();
	if (!targetView) return;

	// Create a command encoder for the draw call
	CommandEncoderDescriptor encoderDesc = {};
	encoderDesc.label = "My command encoder";
	CommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &encoderDesc);

	// Create the render pass that clears the screen with our color
	RenderPassDescriptor renderPassDesc = {};

	// The attachment part of the render pass descriptor describes the target texture of the pass
	RenderPassColorAttachment renderPassColorAttachment = {};
	renderPassColorAttachment.view = targetView;
	renderPassColorAttachment.resolveTarget = nullptr;
	renderPassColorAttachment.loadOp = LoadOp::Clear;
	renderPassColorAttachment.storeOp = StoreOp::Store;
	renderPassColorAttachment.clearValue = WGPUColor{ 0.05, 0.05, 0.05, 1.0 };
#ifndef WEBGPU_BACKEND_WGPU
	renderPassColorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
#endif // NOT WEBGPU_BACKEND_WGPU

	renderPassDesc.colorAttachmentCount = 1;
	renderPassDesc.colorAttachments = &renderPassColorAttachment;
	renderPassDesc.depthStencilAttachment = nullptr;
	renderPassDesc.timestampWrites = nullptr;

	RenderPassEncoder renderPass = encoder.beginRenderPass(renderPassDesc);

	// Select which render pipeline to use
	renderPass.setPipeline(pipeline);

	// Set vertex buffer while encoding the render pass
	renderPass.setVertexBuffer(0, pointBuffer, 0, pointBuffer.getSize());
	
	// The second argument must correspond to the choice of uint16_t or uint32_t
	// we've done when creating the index buffer.
	renderPass.setIndexBuffer(indexBuffer, IndexFormat::Uint16, 0, indexBuffer.getSize());

	// Set binding group here!
	renderPass.setBindGroup(0, bindGroup, 0, nullptr);

	// Replace `draw()` with `drawIndexed()` and `vertexCount` with `indexCount`
	// The extra argument is an offset within the index buffer.
	renderPass.drawIndexed(indexCount, 1, 0, 0, 0);

	renderPass.end();
	renderPass.release();

	// Finally encode and submit the render pass
	CommandBufferDescriptor cmdBufferDescriptor = {};
	cmdBufferDescriptor.label = "Command buffer";
	CommandBuffer command = encoder.finish(cmdBufferDescriptor);
	encoder.release();

	std::cout << "Submitting command..." << std::endl;
	queue.submit(1, &command);
	command.release();
	std::cout << "Command submitted." << std::endl;

	// At the end of the frame
	targetView.release();
#ifndef __EMSCRIPTEN__
	surface.present();
#endif

#if defined(WEBGPU_BACKEND_DAWN)
	device.tick();
#elif defined(WEBGPU_BACKEND_WGPU)
	device.poll(false);
#endif
}

bool Application::IsRunning() {
	return !glfwWindowShouldClose(window);
}

TextureView Application::GetNextSurfaceTextureView() {
	// Get the surface texture
	SurfaceTexture surfaceTexture;
	surface.getCurrentTexture(&surfaceTexture);
	if (surfaceTexture.status != SurfaceGetCurrentTextureStatus::Success) {
		return nullptr;
	}
	Texture texture = surfaceTexture.texture;

	// Create a view for this surface texture
	TextureViewDescriptor viewDescriptor;
	viewDescriptor.label = "Surface texture view";
	viewDescriptor.format = texture.getFormat();
	viewDescriptor.dimension = TextureViewDimension::_2D;
	viewDescriptor.baseMipLevel = 0;
	viewDescriptor.mipLevelCount = 1;
	viewDescriptor.baseArrayLayer = 0;
	viewDescriptor.arrayLayerCount = 1;
	viewDescriptor.aspect = TextureAspect::All;
	TextureView targetView = texture.createView(viewDescriptor);

	return targetView;
}

void Application::InitializePipeline() {
	std::cout << "Creating shader module..." << std::endl;
	ShaderModule shaderModule = ResourceManager::loadShaderModule(RESOURCE_DIR "/shader.wgsl", device);
	std::cout << "Shader module: " << shaderModule << std::endl;

	// Check for errors
	if (shaderModule == nullptr) {
		std::cerr << "Could not load shader!" << std::endl;
		exit(1);
	}

	// Create the render pipeline
	RenderPipelineDescriptor pipelineDesc;

	// Configure the vertex pipeline
	// We use one vertex buffer
	VertexBufferLayout vertexBufferLayout;
	// We now have 2 attributes
	std::vector<VertexAttribute> vertexAttribs(2);
	
	// Describe the position attribute
	vertexAttribs[0].shaderLocation = 0; // @location(0)
	vertexAttribs[0].format = VertexFormat::Float32x2;
	vertexAttribs[0].offset = 0;

	// Describe the color attribute
	vertexAttribs[1].shaderLocation = 1; // @location(1)
	vertexAttribs[1].format = VertexFormat::Float32x3; // different type!
	vertexAttribs[1].offset = 2 * sizeof(float); // non null offset!
	
	vertexBufferLayout.attributeCount = static_cast<uint32_t>(vertexAttribs.size());
	vertexBufferLayout.attributes = vertexAttribs.data();
	
	vertexBufferLayout.arrayStride = 5 * sizeof(float);
	//                               ^^^^^^^^^^^^^^^^^ The new stride
	vertexBufferLayout.stepMode = VertexStepMode::Vertex;
	
	pipelineDesc.vertex.bufferCount = 1;
	pipelineDesc.vertex.buffers = &vertexBufferLayout;

	// NB: We define the 'shaderModule' in the second part of this chapter.
	// Here we tell that the programmable vertex shader stage is described
	// by the function called 'vs_main' in that module.
	pipelineDesc.vertex.module = shaderModule;
	pipelineDesc.vertex.entryPoint = "vs_main";
	pipelineDesc.vertex.constantCount = 0;
	pipelineDesc.vertex.constants = nullptr;

	// Each sequence of 3 vertices is considered as a triangle
	pipelineDesc.primitive.topology = PrimitiveTopology::TriangleList;
	
	// We'll see later how to specify the order in which vertices should be
	// connected. When not specified, vertices are considered sequentially.
	pipelineDesc.primitive.stripIndexFormat = IndexFormat::Undefined;
	
	// The face orientation is defined by assuming that when looking
	// from the front of the face, its corner vertices are enumerated
	// in the counter-clockwise (CCW) order.
	pipelineDesc.primitive.frontFace = FrontFace::CCW;
	
	// But the face orientation does not matter much because we do not
	// cull (i.e. "hide") the faces pointing away from us (which is often
	// used for optimization).
	pipelineDesc.primitive.cullMode = CullMode::None;

	// We tell that the programmable fragment shader stage is described
	// by the function called 'fs_main' in the shader module.
	FragmentState fragmentState;
	fragmentState.module = shaderModule;
	fragmentState.entryPoint = "fs_main";
	fragmentState.constantCount = 0;
	fragmentState.constants = nullptr;

	BlendState blendState;
	blendState.color.srcFactor = BlendFactor::SrcAlpha;
	blendState.color.dstFactor = BlendFactor::OneMinusSrcAlpha;
	blendState.color.operation = BlendOperation::Add;
	blendState.alpha.srcFactor = BlendFactor::Zero;
	blendState.alpha.dstFactor = BlendFactor::One;
	blendState.alpha.operation = BlendOperation::Add;
	
	ColorTargetState colorTarget;
	colorTarget.format = surfaceFormat;
	colorTarget.blend = &blendState;
	colorTarget.writeMask = ColorWriteMask::All; // We could write to only some of the color channels.
	
	// We have only one target because our render pass has only one output color
	// attachment.
	fragmentState.targetCount = 1;
	fragmentState.targets = &colorTarget;
	pipelineDesc.fragment = &fragmentState;

	// We do not use stencil/depth testing for now
	pipelineDesc.depthStencil = nullptr;

	// Samples per pixel
	pipelineDesc.multisample.count = 1;

	// Default value for the mask, meaning "all bits on"
	pipelineDesc.multisample.mask = ~0u;

	// Default value as well (irrelevant for count = 1 anyways)
	pipelineDesc.multisample.alphaToCoverageEnabled = false;

	// Define binding layout (don't forget to = Default)
	BindGroupLayoutEntry bindingLayout = Default;
	// The binding index as used in the @binding attribute in the shader
	bindingLayout.binding = 0;
	// The stage that needs to access this resource
	bindingLayout.visibility = ShaderStage::Vertex;
	bindingLayout.buffer.type = BufferBindingType::Uniform;
	bindingLayout.buffer.minBindingSize = 4 * sizeof(float);

	// Create a bind group layout
	BindGroupLayoutDescriptor bindGroupLayoutDesc{};
	bindGroupLayoutDesc.entryCount = 1;
	bindGroupLayoutDesc.entries = &bindingLayout;
	bindGroupLayout = device.createBindGroupLayout(bindGroupLayoutDesc);

	// Create the pipeline layout
	PipelineLayoutDescriptor layoutDesc{};
	layoutDesc.bindGroupLayoutCount = 1;
	layoutDesc.bindGroupLayouts = (WGPUBindGroupLayout*)&bindGroupLayout;
	layout = device.createPipelineLayout(layoutDesc);

	pipelineDesc.layout = layout;
	
	pipeline = device.createRenderPipeline(pipelineDesc);

	// We no longer need to access the shader module
	shaderModule.release();
}

RequiredLimits Application::GetRequiredLimits(Adapter adapter) const {
	// Get adapter supported limits, in case we need them
	SupportedLimits supportedLimits;
	adapter.getLimits(&supportedLimits);

	// Don't forget to = Default
	RequiredLimits requiredLimits = Default;

	// We use at most 2 vertex attributes
	requiredLimits.limits.maxVertexAttributes = 2;
	// We should also tell that we use 1 vertex buffers
	requiredLimits.limits.maxVertexBuffers = 1;
	// Maximum size of a buffer is 15 vertices of 5 float each
	requiredLimits.limits.maxBufferSize = 15 * 5 * sizeof(float);
	// Maximum stride between 2 consecutive vertices in the vertex buffer
	requiredLimits.limits.maxVertexBufferArrayStride = 5 * sizeof(float);

	// There is a maximum of 3 float forwarded from vertex to fragment shader
	requiredLimits.limits.maxInterStageShaderComponents = 3;

	// We use at most 1 bind group for now
	requiredLimits.limits.maxBindGroups = 1;
	// We use at most 1 uniform buffer per stage
	requiredLimits.limits.maxUniformBuffersPerShaderStage = 1;
	// Uniform structs have a size of maximum 16 float (more than what we need)
	requiredLimits.limits.maxUniformBufferBindingSize = 16 * 4;

	// These two limits are different because they are "minimum" limits,
	// they are the only ones we are may forward from the adapter's supported
	// limits.
	requiredLimits.limits.minUniformBufferOffsetAlignment = supportedLimits.limits.minUniformBufferOffsetAlignment;
	requiredLimits.limits.minStorageBufferOffsetAlignment = supportedLimits.limits.minStorageBufferOffsetAlignment;
	return requiredLimits;
}

void Application::InitializeBuffers() {
	// Define data vectors, but without filling them in
	std::vector<float> pointData;
	std::vector<uint16_t> indexData;

	// Here we use the new 'loadGeometry' function:
	bool success = ResourceManager::loadGeometry(RESOURCE_DIR "/webgpu.txt", pointData, indexData);

	// Check for errors
	if (!success) {
		std::cerr << "Could not load geometry!" << std::endl;
		exit(1);
	}

	// We now store the index count rather than the vertex count
	indexCount = static_cast<uint32_t>(indexData.size());
	
	// Create vertex buffer
	BufferDescriptor bufferDesc;
	bufferDesc.size = pointData.size() * sizeof(float);
	bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Vertex; // Vertex usage here!
	bufferDesc.mappedAtCreation = false;
	pointBuffer = device.createBuffer(bufferDesc);
	
	// Upload geometry data to the buffer
	queue.writeBuffer(pointBuffer, 0, pointData.data(), bufferDesc.size);

	// Create index buffer
	// (we reuse the bufferDesc initialized for the pointBuffer)
	bufferDesc.size = indexData.size() * sizeof(uint16_t);
	bufferDesc.size = (bufferDesc.size + 3) & ~3; // round up to the next multiple of 4
	bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Index;
	indexBuffer = device.createBuffer(bufferDesc);

	queue.writeBuffer(indexBuffer, 0, indexData.data(), bufferDesc.size);

	// Create uniform buffer (reusing bufferDesc from other buffer creations)
	// The buffer will only contain 1 float with the value of uTime
	// then 3 floats left empty but needed by alignment constraints
	bufferDesc.size = 4 * sizeof(float);

	// Make sure to flag the buffer as BufferUsage::Uniform
	bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Uniform;

	bufferDesc.mappedAtCreation = false;
	uniformBuffer = device.createBuffer(bufferDesc);

	// Upload uniform data
	float currentTime = 1.0f;
	queue.writeBuffer(uniformBuffer, 0, &currentTime, sizeof(float));
}

void Application::InitializeBindGroups() {
	// Create a binding
	BindGroupEntry binding{};
	// The index of the binding (the entries in bindGroupDesc can be in any order)
	binding.binding = 0;
	// The buffer it is actually bound to
	binding.buffer = uniformBuffer;
	// We can specify an offset within the buffer, so that a single buffer can hold
	// multiple uniform blocks.
	binding.offset = 0;
	// And we specify again the size of the buffer.
	binding.size = 4 * sizeof(float);

	// A bind group contains one or multiple bindings
	BindGroupDescriptor bindGroupDesc{};
	bindGroupDesc.layout = bindGroupLayout;
	// There must be as many bindings as declared in the layout!
	bindGroupDesc.entryCount = 1;
	bindGroupDesc.entries = &binding;
	bindGroup = device.createBindGroup(bindGroupDesc);
}
