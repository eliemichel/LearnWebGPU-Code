// In Application.cpp
#include "Application.h"
#include "webgpu-utils.h"

#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
// In Application.cpp
#include <glfw3webgpu.h>
using namespace wgpu; // NEW
// In main.cpp
#include "ResourceManager.h"

bool Application::InitializePipeline() {
    // In Initialize() or in a dedicated InitializePipeline()
    std::cout << "Creating shader module..." << std::endl;
    ShaderModule shaderModule = ResourceManager::loadShaderModule(RESOURCE_DIR "/shader.wgsl", m_device);
    std::cout << "Shader module: " << shaderModule << std::endl;
    
    // Check for errors
    if (shaderModule == nullptr) return false;
    RenderPipelineDescriptor pipelineDesc = Default;
    // Vertex fetch
    VertexBufferLayout vertexBufferLayout = Default;
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
    
    // When describing the render pipeline:
    pipelineDesc.vertex.bufferCount = 1;
    pipelineDesc.vertex.buffers = &vertexBufferLayout;
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
	// At the end of Initialize()
	if (!InitializeBuffers()) return false;
	return true;
}

void Application::Terminate() {
	// Move all the release/destroy/terminate calls here
	m_pointBuffer.release();
	m_indexBuffer.release();
	// At the beginning of Terminate()
	m_vertexBuffer.release();
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
	colorAttachment.clearValue = Color{ 0.25, 0.25, 0.25, 1.0 }; // NEW
	renderPassDesc.colorAttachmentCount = 1;
	renderPassDesc.colorAttachments = &colorAttachment;
	
	RenderPassEncoder renderPass = encoder.beginRenderPass(renderPassDesc); // NEW
	renderPass.setPipeline(m_pipeline);
	
	// Set both vertex and index buffers
	renderPass.setVertexBuffer(0, m_pointBuffer, 0, m_pointBuffer.getSize());
	// The second argument must correspond to the choice of uint16_t or uint32_t
	// we've done when creating the index buffer.
	renderPass.setIndexBuffer(m_indexBuffer, IndexFormat::Uint16, 0, m_indexBuffer.getSize());
	
	// Replace `draw()` with `drawIndexed()` and `m_vertexCount` with `m_indexCount`
	// The extra argument is an offset within the index buffer.
	renderPass.drawIndexed(m_indexCount, 1, 0, 0, 0);
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
bool Application::InitializeBuffers() {
	// 1. Load from disk into CPU-side vectors pointData and indexData
	// Define data vectors, but without filling them in
	std::vector<float> pointData;
	std::vector<uint16_t> indexData;
	// Here we use the new 'loadGeometry' function:
	bool success = ResourceManager::loadGeometry(RESOURCE_DIR  "/webgpu.txt", pointData, indexData);
	if (!success) return false;
	
	m_indexCount = static_cast<uint32_t>(indexData.size());

	// 2. Create GPU buffers and upload data to them
	// Create point buffer
	BufferDescriptor bufferDesc = Default;
	bufferDesc.size = pointData.size() * sizeof(float);
	bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Vertex; // Vertex usage here!
	m_pointBuffer = m_device.createBuffer(bufferDesc);
	
	// Upload geometry data to the buffer
	m_queue.writeBuffer(m_pointBuffer, 0, pointData.data(), bufferDesc.size);
	// It is not easy with the auto-generation of code to remove the previously
	// defined `vertexBuffer` attribute, but at the same time some compilers
	// (rightfully) complain if we do not use it. This is a hack to mark the
	// variable as used and have automated build tests pass.
	(void)m_vertexBuffer;
	(void)m_vertexCount;
	// Create index buffer
	// (we reuse the bufferDesc initialized for the vertexBuffer)
	bufferDesc.size = indexData.size() * sizeof(uint16_t);
	bufferDesc.size = (bufferDesc.size + 3) & ~3; // round up to the next multiple of 4
	indexData.resize((indexData.size() + 1) & ~1); // round up to the next multiple of 2
	bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Index;
	m_indexBuffer = m_device.createBuffer(bufferDesc);
	
	m_queue.writeBuffer(m_indexBuffer, 0, indexData.data(), bufferDesc.size);
	return true;
}