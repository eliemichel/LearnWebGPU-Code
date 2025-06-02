// {Begin block 'file: main.cpp' (in root '020 - Opening a window - Next')}
// {Begin block 'Includes' (in root '020 - Opening a window - Next')}
#include "webgpu-utils.h"
#include <vector>
// Includes
#define WEBGPU_CPP_IMPLEMENTATION // NEW
#include <webgpu/webgpu.hpp> // NEW
#include <iostream>
#include <thread>
#include <chrono>
#ifdef __EMSCRIPTEN__
#  include <emscripten.h>
#endif
#include <string_view>
#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>
// {End block 'Includes' (in root '020 - Opening a window - Next')}

using namespace wgpu; // NEW

// {Begin block 'Application class' (in root '025 - First Color - Next')}
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
    TextureView GetNextSurfaceView(); // NEW

private:
	// We put here all the variables that are shared between init and main loop
	// {Begin block 'Application attributes' (in root '020 - Opening a window - Next')}
	GLFWwindow *window;
	wgpu::Instance instance; // NEW
	wgpu::Device device;
	wgpu::Queue queue;
	wgpu::Surface surface;
	// {End block 'Application attributes' (in root '020 - Opening a window - Next')}
};
// {End block 'Application class' (in root '025 - First Color - Next')}

// {Begin block 'Main function' (in root '020 - Opening a window - Next')}
int main() {
	Application app;

	if (!app.Initialize()) {
		return 1;
	}

	// {Begin block 'Main loop' (in root '020 - Opening a window - Next')}
	#ifdef __EMSCRIPTEN__
		// {Begin block 'Emscripten main loop' (in root '020 - Opening a window - Next')}
		// Equivalent of the main loop when using Emscripten:
		auto callback = [](void *arg) {
		    //                   ^^^ 2. We get the address of the app in the callback.
		    Application* pApp = reinterpret_cast<Application*>(arg);
		    //                  ^^^^^^^^^^^^^^^^ 3. We force this address to be interpreted
		    //                                      as a pointer to an Application object.
		    pApp->MainLoop(); // 4. We can use the application object
		};
		emscripten_set_main_loop_arg(callback, &app, 0, true);
		//                                     ^^^^ 1. We pass the address of our application object.
		// {End block 'Emscripten main loop' (in root '020 - Opening a window - Next')}
	#else // __EMSCRIPTEN__
		while (app.IsRunning()) {
			app.MainLoop();
		}
	#endif // __EMSCRIPTEN__
	// {End block 'Main loop' (in root '020 - Opening a window - Next')}

	app.Terminate();

	return 0;
}
// {End block 'Main function' (in root '020 - Opening a window - Next')}

// {Begin block 'Application implementation' (in root '025 - First Color - Next')}
bool Application::Initialize() {
	// Move the whole initialization here
	// {Begin block 'Initialize' (in root '025 - First Color - Next')}
	// {Begin block 'Open window and get adapter' (in root '020 - Opening a window - Next')}
	// Open window
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // <-- extra info for glfwCreateWindow
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	window = glfwCreateWindow(640, 480, "Learn WebGPU", nullptr, nullptr);
	
	// Create instance ('instance' is now declared at the class level)
	instance = createInstance(); // NEW
	
	// Get adapter
	std::cout << "Requesting adapter..." << std::endl;
	// {Begin block 'Request adapter' (in root '020 - Opening a window - Next')}
	// {Begin block 'Get the surface' (in root '020 - Opening a window - Next')}
	surface = glfwCreateWindowWGPUSurface(instance, window);
	// {End block 'Get the surface' (in root '020 - Opening a window - Next')}
	
	RequestAdapterOptions adapterOpts = Default; // NEW
	adapterOpts.compatibleSurface = surface;
	//                              ^^^^^^^ Use the surface here
	
	Adapter adapter = requestAdapterSync(instance, &adapterOpts); // TODO
	// {End block 'Request adapter' (in root '020 - Opening a window - Next')}
	std::cout << "Got adapter: " << adapter << std::endl;
	// {End block 'Open window and get adapter' (in root '020 - Opening a window - Next')}
	
	// {Begin block 'Request device' (in root '020 - Opening a window - Next')}
	std::cout << "Requesting device..." << std::endl;
	DeviceDescriptor deviceDesc = Default; // NEW
	// {Begin block 'Build device descriptor' (in root '010 - The Device - Next')}
	// Any name works here, that's your call
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
	// {Begin block 'Device Lost Callback' (in root '010 - The Device - Next')}
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
	// {End block 'Device Lost Callback' (in root '010 - The Device - Next')}
	deviceDesc.deviceLostCallbackInfo.callback = onDeviceLost;
	deviceDesc.deviceLostCallbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;
	// {Begin block 'Device Error Callback' (in root '010 - The Device - Next')}
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
	// {End block 'Device Error Callback' (in root '010 - The Device - Next')}
	deviceDesc.uncapturedErrorCallbackInfo.callback = onDeviceError;
	// {End block 'Build device descriptor' (in root '010 - The Device - Next')}
	// NB: 'device' is now declared at the class level
	device = requestDeviceSync(instance, adapter, &deviceDesc); // TODO
	std::cout << "Got device: " << device << std::endl;
	// {End block 'Request device' (in root '020 - Opening a window - Next')}
	
	queue = device.getQueue(); // NEW
	
	// {Begin block 'Surface Configuration' (in root '025 - First Color - Next')}
	SurfaceConfiguration config = Default; // NEW
	
	// {Begin block 'Describe the surface configuration' (in root '025 - First Color - Next')}
	// Configuration of the textures created for the underlying swap chain
	config.width = 640;
	config.height = 480;
	config.device = device;
	// {Begin block 'Describe surface format' (in root '025 - First Color - Next')}
	// We initialize an empty capability struct:
	SurfaceCapabilities capabilities = Default; // NEW
	
	// We get the capabilities for a pair of (surface, adapter).
	// If it works, this populates the `capabilities` structure
	Status status = surface.getCapabilities(adapter, &capabilities); // NEW
	if (status != Status::Success) { // NEW
	    return false;
	}
	
	// From the capabilities, we get the preferred format: it is always the first one!
	// (NB: There is always at least 1 format if the GetCapabilities was successful)
	config.format = capabilities.formats[0];
	
	// We no longer need to access the capabilities, so we release their memory.
	capabilities.freeMembers(); // NEW
	// {End block 'Describe surface format' (in root '025 - First Color - Next')}
	config.presentMode = PresentMode::Fifo; // NEW
	config.alphaMode = CompositeAlphaMode::Auto; // NEW
	// {End block 'Describe the surface configuration' (in root '025 - First Color - Next')}
	
	surface.configure(config); // NEW
	// {End block 'Surface Configuration' (in root '025 - First Color - Next')}
	
	// We no longer need to access the adapter
	adapter.release(); // NEW
	// {End block 'Initialize' (in root '025 - First Color - Next')}
	return true;
}

void Application::Terminate() {
	// Move all the release/destroy/terminate calls here
	// {Begin block 'Terminate' (in root '025 - First Color - Next')}
	surface.unconfigure(); // NEW
	queue.release();
	// {Begin block 'Destroy surface' (in root '020 - Opening a window - Next')}
	surface.release();
	// {End block 'Destroy surface' (in root '020 - Opening a window - Next')}
	device.release();
	glfwDestroyWindow(window);
	glfwTerminate();
	// {End block 'Terminate' (in root '025 - First Color - Next')}
}

void Application::MainLoop() {
	glfwPollEvents();
	instance.processEvents(); // NEW

	// {Begin block 'Main loop content' (in root '025 - First Color - Next')}
	// In Application::MainLoop()
	// {Begin block 'Get the next target texture view' (in root '025 - First Color - Next')}
	// Get the next target texture view
	TextureView targetView = GetNextSurfaceView(); // NEW
	if (!targetView) return; // no surface texture, we skip this frame
	// {End block 'Get the next target texture view' (in root '025 - First Color - Next')}
	// {Begin block 'Draw things' (in root '025 - First Color - Next')}
	// {Begin block 'Create Command Encoder' (in root '015 - The Command Queue - Next')}
	CommandEncoderDescriptor encoderDesc = Default; // NEW
	encoderDesc.label = StringView("My command encoder"); // NEW
	CommandEncoder encoder = device.createCommandEncoder(encoderDesc); // NEW
	// {End block 'Create Command Encoder' (in root '015 - The Command Queue - Next')}
	// {Begin block 'Encode Render Pass' (in root '025 - First Color - Next')}
	RenderPassDescriptor renderPassDesc = Default; // NEW
	// {Begin block 'Describe Render Pass' (in root '025 - First Color - Next')}
	RenderPassColorAttachment renderPassColorAttachment = Default; // NEW
	
	// {Begin block 'Describe the attachment' (in root '025 - First Color - Next')}
	renderPassColorAttachment.view = targetView;
	renderPassColorAttachment.loadOp = LoadOp::Clear; // NEW
	renderPassColorAttachment.storeOp = StoreOp::Store; // NEW
	renderPassColorAttachment.clearValue = Color{ 1.0, 0.8, 0.55, 1.0 }; // NEW
	// {End block 'Describe the attachment' (in root '025 - First Color - Next')}
	
	renderPassDesc.colorAttachmentCount = 1;
	renderPassDesc.colorAttachments = &renderPassColorAttachment;
	// {End block 'Describe Render Pass' (in root '025 - First Color - Next')}
	
	RenderPassEncoder renderPass = encoder.beginRenderPass(renderPassDesc); // NEW
	// {Begin block 'Use Render Pass' (in root '025 - First Color - Next')}
	// Use the render pass here (we do nothing with the render pass for now)
	// {End block 'Use Render Pass' (in root '025 - First Color - Next')}
	renderPass.end(); // NEW
	renderPass.release(); // NEW
	// {End block 'Encode Render Pass' (in root '025 - First Color - Next')}
	// {Begin block 'Finish encoding and submit' (in root '015 - The Command Queue - Next')}
	CommandBufferDescriptor cmdBufferDescriptor = Default; // NEW
	cmdBufferDescriptor.label = StringView("Command buffer"); // NEW
	CommandBuffer command = encoder.finish(cmdBufferDescriptor); // NEW
	encoder.release(); // NEW // release encoder after it's finished
	
	// Finally submit the command queue
	std::cout << "Submitting command..." << std::endl;
	queue.submit(command); // NEW
	command.release(); // NEW
	std::cout << "Command submitted." << std::endl;
	// {End block 'Finish encoding and submit' (in root '015 - The Command Queue - Next')}
	// {End block 'Draw things' (in root '025 - First Color - Next')}
	// {Begin block 'Present the surface onto the window' (in root '025 - First Color - Next')}
	// At the end of the frame
	targetView.release(); // NEW
	#ifndef __EMSCRIPTEN__
	surface.present(); // NEW
	#endif
	// {End block 'Present the surface onto the window' (in root '025 - First Color - Next')}
	// {End block 'Main loop content' (in root '025 - First Color - Next')}
}

bool Application::IsRunning() {
	return !glfwWindowShouldClose(window);
}
// {Begin block 'GetNextSurfaceView method' (in root '025 - First Color - Next')}
TextureView Application::GetNextSurfaceView() { // NEW
    // {Begin block 'Get the next surface texture' (in root '025 - First Color - Next')}
    SurfaceTexture surfaceTexture = Default; // NEW
    surface.getCurrentTexture(&surfaceTexture); // NEW
    if (
        surfaceTexture.status != SurfaceGetCurrentTextureStatus::SuccessOptimal && // NEW
        surfaceTexture.status != SurfaceGetCurrentTextureStatus::SuccessSuboptimal
    ) {
        return nullptr;
    }
    // {End block 'Get the next surface texture' (in root '025 - First Color - Next')}
    // {Begin block 'Create surface texture view' (in root '025 - First Color - Next')}
    TextureViewDescriptor viewDescriptor = Default; // NEW
    viewDescriptor.label = StringView("Surface texture view"); // NEW
    viewDescriptor.dimension = TextureViewDimension::_2D; // NEW // not to confuse with 2DArray
    TextureView targetView = Texture(surfaceTexture.texture).createView(viewDescriptor); // NEW, TODO
    // {End block 'Create surface texture view' (in root '025 - First Color - Next')}
    // {Begin block 'Release the texture' (in root '025 - First Color - Next')}
    // We no longer need the texture, only its view,
    // so we release it at the end of GetNextSurfaceViewData
    Texture(surfaceTexture.texture).release(); // NEW
    // {End block 'Release the texture' (in root '025 - First Color - Next')}
    return targetView;
}
// {End block 'GetNextSurfaceView method' (in root '025 - First Color - Next')}
// {End block 'Application implementation' (in root '025 - First Color - Next')}
// {End block 'file: main.cpp' (in root '020 - Opening a window - Next')}