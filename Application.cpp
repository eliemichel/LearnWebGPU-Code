// {Begin block 'file: Application.cpp' (in root '020 - Opening a window - Next')}
// {Begin block 'Includes in Application.cpp' (in root '028 - C++ Wrapper - Next')}
// In Application.cpp
#include "Application.h"
#include "webgpu-utils.h"

#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
// In Application.cpp
#include <glfw3webgpu.h>
using namespace wgpu; // NEW
// {End block 'Includes in Application.cpp' (in root '028 - C++ Wrapper - Next')}

// {Begin block 'Application implementation' (in root '028 - C++ Wrapper - Next')}
bool Application::Initialize() {
	// Move the whole initialization here
	// {Begin block 'Initialize' (in root '028 - C++ Wrapper - Next')}
	// {Begin block 'Open window and get adapter' (in root '028 - C++ Wrapper - Next')}
	// Open window
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // <-- extra info for glfwCreateWindow
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	m_window = glfwCreateWindow(640, 480, "Learn WebGPU", nullptr, nullptr);
	
	// Create instance ('instance' is now declared at the class level)
	m_instance = createInstance(); // NEW
	
	// Get adapter
	std::cout << "Requesting adapter..." << std::endl;
	// {Begin block 'Request adapter' (in root '028 - C++ Wrapper - Next')}
	// {Begin block 'Get the surface' (in root '020 - Opening a window - Next')}
	m_surface = glfwCreateWindowWGPUSurface(m_instance, m_window);
	// {End block 'Get the surface' (in root '020 - Opening a window - Next')}
	RequestAdapterOptions adapterOpts = Default; // NEW
	adapterOpts.compatibleSurface = m_surface;
	Adapter adapter = requestAdapterSync(m_instance, &adapterOpts); // TODO
	// {End block 'Request adapter' (in root '028 - C++ Wrapper - Next')}
	std::cout << "Got adapter: " << adapter << std::endl;
	// {End block 'Open window and get adapter' (in root '028 - C++ Wrapper - Next')}
	
	// {Begin block 'Request device' (in root '028 - C++ Wrapper - Next')}
	std::cout << "Requesting device..." << std::endl;
	DeviceDescriptor deviceDesc = Default; // NEW
	// {Begin block 'Build device descriptor' (in root '028 - C++ Wrapper - Next')}
	deviceDesc.label = StringView("My Device"); // NEW
	
	std::vector<FeatureName> features; // NEW
	// {Begin block 'List required features' (in root '010 - The Device - Next')}
	// No required feature for now
	// {End block 'List required features' (in root '010 - The Device - Next')}
	deviceDesc.requiredFeatureCount = features.size();
	deviceDesc.requiredFeatures = (WGPUFeatureName*)features.data(); // NEW
	
	// Make sure 'features' lives until the call to wgpuAdapterRequestDevice!
	Limits requiredLimits = Default; // NEW
	// {Begin block 'Specify required limits' (in root '010 - The Device - Next')}
	// We leave 'requiredLimits' untouched for now
	// {End block 'Specify required limits' (in root '010 - The Device - Next')}
	deviceDesc.requiredLimits = &requiredLimits;
	
	// Make sure that the 'requiredLimits' variable lives until the call to wgpuAdapterRequestDevice!
	deviceDesc.defaultQueue.label = StringView("The Default Queue"); // NEW
	
	// {Begin block 'Device Lost Callback' (in root '028 - C++ Wrapper - Next')}
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
	// {End block 'Device Lost Callback' (in root '028 - C++ Wrapper - Next')}
	
	// {Begin block 'Device Error Callback' (in root '028 - C++ Wrapper - Next')}
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
	// {End block 'Device Error Callback' (in root '028 - C++ Wrapper - Next')}
	// {End block 'Build device descriptor' (in root '028 - C++ Wrapper - Next')}
	m_device = requestDeviceSync(m_instance, adapter, &deviceDesc); // TODO
	std::cout << "Got device: " << m_device << std::endl;
	// {End block 'Request device' (in root '028 - C++ Wrapper - Next')}
	
	m_queue = m_device.getQueue(); // NEW
	
	// {Begin block 'Surface Configuration' (in root '028 - C++ Wrapper - Next')}
	SurfaceConfiguration config = Default; // NEW
	// {Begin block 'Describe the surface configuration' (in root '028 - C++ Wrapper - Next')}
	// Configuration of the textures created for the underlying swap chain
	config.width = 640;
	config.height = 480;
	config.device = m_device;
	// {Begin block 'Describe surface format' (in root '028 - C++ Wrapper - Next')}
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
	// {End block 'Describe surface format' (in root '028 - C++ Wrapper - Next')}
	config.presentMode = PresentMode::Fifo; // NEW
	config.alphaMode = CompositeAlphaMode::Auto; // NEW
	// {End block 'Describe the surface configuration' (in root '028 - C++ Wrapper - Next')}
	m_surface.configure(config); // NEW
	// {End block 'Surface Configuration' (in root '028 - C++ Wrapper - Next')}
	
	// We no longer need to access the adapter
	adapter.release(); // NEW
	// {End block 'Initialize' (in root '028 - C++ Wrapper - Next')}
	return true;
}

void Application::Terminate() {
	// Move all the release/destroy/terminate calls here
	// {Begin block 'Terminate' (in root '028 - C++ Wrapper - Next')}
	m_surface.unconfigure(); // NEW
	m_queue.release(); // NEW
	// {Begin block 'Destroy surface' (in root '028 - C++ Wrapper - Next')}
	m_surface.release(); // NEW
	// {End block 'Destroy surface' (in root '028 - C++ Wrapper - Next')}
	m_device.release(); // NEW
	glfwDestroyWindow(m_window);
	glfwTerminate();
	// {End block 'Terminate' (in root '028 - C++ Wrapper - Next')}
}

void Application::MainLoop() {
	glfwPollEvents();
	m_instance.processEvents(); // NEW

	// {Begin block 'Main loop content' (in root '025 - First Color - Next')}
	// In Application::MainLoop()
	// {Begin block 'Get the next target texture view' (in root '028 - C++ Wrapper - Next')}
	// Get the next target texture view
	TextureView targetView = GetNextSurfaceView(); // NEW
	if (!targetView) return; // no surface texture, we skip this frame
	// {End block 'Get the next target texture view' (in root '028 - C++ Wrapper - Next')}
	// {Begin block 'Draw things' (in root '025 - First Color - Next')}
	// {Begin block 'Create Command Encoder' (in root '028 - C++ Wrapper - Next')}
	CommandEncoderDescriptor encoderDesc = Default; // NEW
	encoderDesc.label = StringView("My command encoder"); // NEW
	CommandEncoder encoder = m_device.createCommandEncoder(encoderDesc); // NEW
	// {End block 'Create Command Encoder' (in root '028 - C++ Wrapper - Next')}
	// {Begin block 'Encode Render Pass' (in root '028 - C++ Wrapper - Next')}
	RenderPassDescriptor renderPassDesc = Default; // NEW
	// {Begin block 'Describe Render Pass' (in root '028 - C++ Wrapper - Next')}
	RenderPassColorAttachment renderPassColorAttachment = Default; // NEW
	// {Begin block 'Describe the attachment' (in root '028 - C++ Wrapper - Next')}
	renderPassColorAttachment.view = targetView;
	renderPassColorAttachment.loadOp = LoadOp::Clear; // NEW
	renderPassColorAttachment.storeOp = StoreOp::Store; // NEW
	renderPassColorAttachment.clearValue = Color{ 1.0, 0.8, 0.55, 1.0 }; // NEW
	// {End block 'Describe the attachment' (in root '028 - C++ Wrapper - Next')}
	renderPassDesc.colorAttachmentCount = 1;
	renderPassDesc.colorAttachments = &renderPassColorAttachment;
	// {End block 'Describe Render Pass' (in root '028 - C++ Wrapper - Next')}
	
	RenderPassEncoder renderPass = encoder.beginRenderPass(renderPassDesc); // NEW
	// {Begin block 'Use Render Pass' (in root '025 - First Color - Next')}
	// Use the render pass here (we do nothing with the render pass for now)
	// {End block 'Use Render Pass' (in root '025 - First Color - Next')}
	renderPass.end(); // NEW
	renderPass.release(); // NEW
	// {End block 'Encode Render Pass' (in root '028 - C++ Wrapper - Next')}
	// {Begin block 'Finish encoding and submit' (in root '028 - C++ Wrapper - Next')}
	CommandBufferDescriptor cmdBufferDescriptor = Default; // NEW
	cmdBufferDescriptor.label = StringView("Command buffer"); // NEW
	CommandBuffer command = encoder.finish(cmdBufferDescriptor); // NEW
	encoder.release(); // NEW // release encoder after it's finished
	
	// Finally submit the command queue
	std::cout << "Submitting command..." << std::endl;
	m_queue.submit(command); // NEW
	command.release(); // NEW
	std::cout << "Command submitted." << std::endl;
	// {End block 'Finish encoding and submit' (in root '028 - C++ Wrapper - Next')}
	// {End block 'Draw things' (in root '025 - First Color - Next')}
	// {Begin block 'Present the surface onto the window' (in root '028 - C++ Wrapper - Next')}
	targetView.release(); // NEW
	#ifndef __EMSCRIPTEN__
	m_surface.present(); // NEW
	#endif
	// {End block 'Present the surface onto the window' (in root '028 - C++ Wrapper - Next')}
	// {End block 'Main loop content' (in root '025 - First Color - Next')}
}

bool Application::IsRunning() {
	return !glfwWindowShouldClose(m_window);
}

// {Begin block 'GetNextSurfaceView method' (in root '028 - C++ Wrapper - Next')}
TextureView Application::GetNextSurfaceView() { // NEW
	// {Begin block 'Get the next surface texture' (in root '028 - C++ Wrapper - Next')}
	SurfaceTexture surfaceTexture = Default; // NEW
	m_surface.getCurrentTexture(&surfaceTexture); // NEW
	if (
	    surfaceTexture.status != SurfaceGetCurrentTextureStatus::SuccessOptimal && // NEW
	    surfaceTexture.status != SurfaceGetCurrentTextureStatus::SuccessSuboptimal
	) {
	    return nullptr;
	}
	// {End block 'Get the next surface texture' (in root '028 - C++ Wrapper - Next')}
	// {Begin block 'Create surface texture view' (in root '028 - C++ Wrapper - Next')}
	TextureViewDescriptor viewDescriptor = Default; // NEW
	viewDescriptor.label = StringView("Surface texture view"); // NEW
	viewDescriptor.dimension = TextureViewDimension::_2D; // NEW // not to confuse with 2DArray
	TextureView targetView = Texture(surfaceTexture.texture).createView(viewDescriptor); // NEW, TODO
	// {End block 'Create surface texture view' (in root '028 - C++ Wrapper - Next')}
	// {Begin block 'Release the texture' (in root '028 - C++ Wrapper - Next')}
	Texture(surfaceTexture.texture).release(); // NEW, TODO
	// {End block 'Release the texture' (in root '028 - C++ Wrapper - Next')}
	return targetView;
}
// {End block 'GetNextSurfaceView method' (in root '028 - C++ Wrapper - Next')}
// {End block 'Application implementation' (in root '028 - C++ Wrapper - Next')}
// {End block 'file: Application.cpp' (in root '020 - Opening a window - Next')}