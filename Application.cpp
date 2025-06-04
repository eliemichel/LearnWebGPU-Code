// In Application.cpp
#include "Application.h"
#include "webgpu-utils.h"

#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
// In Application.cpp
#include <glfw3webgpu.h>
using namespace wgpu; // NEW

const char* shaderSource = R"(
@vertex
fn vs_main(@builtin(vertex_index) in_vertex_index: u32) -> @builtin(position) vec4f {
	if (in_vertex_index == 0u) {
		return vec4f(-0.45, 0.5, 0.0, 1.0);
	} else if (in_vertex_index == 1u) {
		return vec4f(0.45, 0.5, 0.0, 1.0);
	} else {
		return vec4f(0.0, -0.5, 0.0, 1.0);
	}
}
// Add this in the same shaderSource literal than the vertex entry point
@fragment
fn fs_main() -> @location(0) vec4f {
	return vec4f(0.0, 0.4, 0.7, 1.0);
}
)";
bool Application::InitializePipeline() {
    // In Initialize() or in a dedicated InitializePipeline()
    ShaderSourceWGSL wgslDesc = Default;
    wgslDesc.code = StringView(shaderSource);
    ShaderModuleDescriptor shaderDesc = Default;
    shaderDesc.nextInChain = &wgslDesc.chain; // connect the chained extension
    shaderDesc.label = StringView("Shader source from Application.cpp");
    ShaderModule shaderModule = m_device.createShaderModule(shaderDesc);
    RenderPipelineDescriptor pipelineDesc = Default;
    pipelineDesc.vertex.module = shaderModule;
    pipelineDesc.vertex.entryPoint = StringView("vs_main");
    FragmentState fragmentState = Default;
    fragmentState.module = shaderModule;
    fragmentState.entryPoint = StringView("fs_main");
    ColorTargetState colorTarget = Default;
    colorTarget.format = m_surfaceFormat;
    BlendState blendState = Default;
    colorTarget.blend = &blendState;
    fragmentState.targetCount = 1;
    fragmentState.targets = &colorTarget;
    pipelineDesc.fragment = &fragmentState;
    m_pipeline = m_device.createRenderPipeline(pipelineDesc);
    shaderModule.release();
    return true;
}
bool Application::Initialize() {
	// Move the whole initialization here
	// Open window
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // <-- extra info for glfwCreateWindow
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	m_window = glfwCreateWindow(640, 480, "Learn WebGPU", nullptr, nullptr);
	
	// Create instance ('instance' is now declared at the class level)
	m_instance = createInstance(); // NEW
	
	// Get adapter
	std::cout << "Requesting adapter..." << std::endl;
	m_surface = glfwCreateWindowWGPUSurface(m_instance, m_window);
	RequestAdapterOptions adapterOpts = Default; // NEW
	adapterOpts.compatibleSurface = m_surface;
	Adapter adapter = requestAdapterSync(m_instance, &adapterOpts); // TODO
	std::cout << "Got adapter: " << adapter << std::endl;
	
	std::cout << "Requesting device..." << std::endl;
	DeviceDescriptor deviceDesc = Default; // NEW
	deviceDesc.label = StringView("My Device"); // NEW
	
	std::vector<FeatureName> features; // NEW
	// No required feature for now
	deviceDesc.requiredFeatureCount = features.size();
	deviceDesc.requiredFeatures = (WGPUFeatureName*)features.data(); // NEW
	
	// Make sure 'features' lives until the call to wgpuAdapterRequestDevice!
	Limits requiredLimits = Default; // NEW
	// We leave 'requiredLimits' untouched for now
	deviceDesc.requiredLimits = &requiredLimits;
	
	// Make sure that the 'requiredLimits' variable lives until the call to wgpuAdapterRequestDevice!
	deviceDesc.defaultQueue.label = StringView("The Default Queue"); // NEW
	
	auto onDeviceLost = []( // TODO: setter
		WGPUDevice const * device,
		WGPUDeviceLostReason reason,
		struct WGPUStringView message,
		void* /* userdata1 */,
		void* /* userdata2 */
	) {
		// All we do is display a message when the device is lost
	    std::cout
	    	<< "Device " << device << " was lost: reason " << reason
	    	<< " (" << StringView(message) << ")" // NEW
	    	<< std::endl;
	};
	deviceDesc.deviceLostCallbackInfo.callback = onDeviceLost;
	deviceDesc.deviceLostCallbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;
	
	auto onDeviceError = []( // TODO: setter
		WGPUDevice const * device,
		WGPUErrorType type,
		struct WGPUStringView message,
		void* /* userdata1 */,
		void* /* userdata2 */
	) {
	    std::cout
	    	<< "Uncaptured error in device " << device << ": type " << type
	    	<< " (" << StringView(message) << ")" // NEW
	    	<< std::endl;
	};
	deviceDesc.uncapturedErrorCallbackInfo.callback = onDeviceError;
	m_device = requestDeviceSync(m_instance, adapter, &deviceDesc); // TODO
	std::cout << "Got device: " << m_device << std::endl;
	
	m_queue = m_device.getQueue(); // NEW
	
	SurfaceConfiguration config = Default; // NEW
	// Configuration of the textures created for the underlying swap chain
	config.width = 640;
	config.height = 480;
	config.device = m_device;
	SurfaceCapabilities capabilities = Default; // NEW
	
	// We get the capabilities for a pair of (surface, adapter).
	// If it works, this populates the `capabilities` structure
	Status status = m_surface.getCapabilities(adapter, &capabilities); // NEW
	if (status != Status::Success) { // NEW
	    return false;
	}
	
	// From the capabilities, we get the preferred format: it is always the first one!
	// (NB: There is always at least 1 format if the GetCapabilities was successful)
	config.format = capabilities.formats[0];
	
	// We no longer need to access the capabilities, so we release their memory.
	capabilities.freeMembers(); // NEW
	config.presentMode = PresentMode::Fifo; // NEW
	config.alphaMode = CompositeAlphaMode::Auto; // NEW
	m_surface.configure(config); // NEW
	m_surfaceFormat = config.format;
	
	// We no longer need to access the adapter
	adapter.release(); // NEW
	// At the end of Initialize()
	if (!InitializePipeline()) return false;
	return true;
}

void Application::Terminate() {
	// Move all the release/destroy/terminate calls here
	m_pipeline.release();
	m_surface.unconfigure(); // NEW
	m_queue.release(); // NEW
	m_surface.release(); // NEW
	m_device.release(); // NEW
	glfwDestroyWindow(m_window);
	glfwTerminate();
}

void Application::MainLoop() {
	glfwPollEvents();
	m_instance.processEvents(); // NEW

	// In Application::MainLoop()
	// Get the next target texture view
	TextureView targetView = GetNextSurfaceView(); // NEW
	if (!targetView) return; // no surface texture, we skip this frame
	CommandEncoderDescriptor encoderDesc = Default; // NEW
	encoderDesc.label = StringView("My command encoder"); // NEW
	CommandEncoder encoder = m_device.createCommandEncoder(encoderDesc); // NEW
	RenderPassDescriptor renderPassDesc = Default; // NEW
	RenderPassColorAttachment colorAttachment = Default; // NEW
	colorAttachment.view = targetView;
	colorAttachment.loadOp = LoadOp::Clear; // NEW
	colorAttachment.storeOp = StoreOp::Store; // NEW
	colorAttachment.clearValue = Color{ 1.0, 0.8, 0.55, 1.0 }; // NEW
	renderPassDesc.colorAttachmentCount = 1;
	renderPassDesc.colorAttachments = &colorAttachment;
	
	RenderPassEncoder renderPass = encoder.beginRenderPass(renderPassDesc); // NEW
	// [...] Begin render pass
	// Select which render pipeline to use
	renderPass.setPipeline(m_pipeline);
	// Draw 1 instance of a 3-vertices shape
	renderPass.draw(3, 1, 0, 0);
	// [...] End render pass
	renderPass.end(); // NEW
	renderPass.release(); // NEW
	CommandBufferDescriptor cmdBufferDescriptor = Default; // NEW
	cmdBufferDescriptor.label = StringView("Command buffer"); // NEW
	CommandBuffer command = encoder.finish(cmdBufferDescriptor); // NEW
	encoder.release(); // NEW // release encoder after it's finished
	
	// Finally submit the command queue
	std::cout << "Submitting command..." << std::endl;
	m_queue.submit(command); // NEW
	command.release(); // NEW
	std::cout << "Command submitted." << std::endl;
	targetView.release(); // NEW
	#ifndef __EMSCRIPTEN__
	m_surface.present(); // NEW
	#endif
}

bool Application::IsRunning() {
	return !glfwWindowShouldClose(m_window);
}

TextureView Application::GetNextSurfaceView() { // NEW
	SurfaceTexture surfaceTexture = Default; // NEW
	m_surface.getCurrentTexture(&surfaceTexture); // NEW
	if (
	    surfaceTexture.status != SurfaceGetCurrentTextureStatus::SuccessOptimal && // NEW
	    surfaceTexture.status != SurfaceGetCurrentTextureStatus::SuccessSuboptimal
	) {
	    return nullptr;
	}
	TextureViewDescriptor viewDescriptor = Default; // NEW
	viewDescriptor.label = StringView("Surface texture view"); // NEW
	viewDescriptor.dimension = TextureViewDimension::_2D; // NEW // not to confuse with 2DArray
	TextureView targetView = Texture(surfaceTexture.texture).createView(viewDescriptor); // NEW, TODO
	Texture(surfaceTexture.texture).release(); // NEW, TODO
	return targetView;
}