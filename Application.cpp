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

// {Begin block 'Application implementation' (in root '032 - A first Vertex Attribute - Next')}
// {Begin block 'Shader source literal' (in root '032 - A first Vertex Attribute - Next')}
const char* shaderSource = R"(
@vertex
// {Begin block 'Vertex shader' (in root '032 - A first Vertex Attribute - Next')}
fn vs_main(@location(0) in_vertex_position: vec2f) -> @builtin(position) vec4f {
	return vec4f(in_vertex_position, 0.0, 1.0);
}
// {End block 'Vertex shader' (in root '032 - A first Vertex Attribute - Next')}

@fragment
// {Begin block 'Fragment shader' (in root '032 - A first Vertex Attribute - Next')}
fn fs_main() -> @location(0) vec4f {
	return vec4f(0.0, 0.4, 1.0, 1.0);
}
// {End block 'Fragment shader' (in root '032 - A first Vertex Attribute - Next')}
)";
// {End block 'Shader source literal' (in root '032 - A first Vertex Attribute - Next')}
// {Begin block 'InitializePipeline method' (in root '030 - Hello Triangle - Next')}
bool Application::InitializePipeline() {
    // {Begin block 'Initialize pipeline' (in root '030 - Hello Triangle - Next')}
    // In Initialize() or in a dedicated InitializePipeline()
    // {Begin block 'Create Shader Module' (in root '030 - Hello Triangle - Next')}
    ShaderSourceWGSL wgslDesc = Default;
    wgslDesc.code = StringView(shaderSource);
    ShaderModuleDescriptor shaderDesc = Default;
    shaderDesc.nextInChain = &wgslDesc.chain; // connect the chained extension
    shaderDesc.label = StringView("Shader source from Application.cpp");
    ShaderModule shaderModule = m_device.createShaderModule(shaderDesc);
    // {End block 'Create Shader Module' (in root '030 - Hello Triangle - Next')}
    // {Begin block 'Create Render Pipeline' (in root '030 - Hello Triangle - Next')}
    RenderPipelineDescriptor pipelineDesc = Default;
    // {Begin block 'Describe render pipeline' (in root '032 - A first Vertex Attribute - Next')}
    // {Begin block 'Describe vertex buffers' (in root '032 - A first Vertex Attribute - Next')}
    // Vertex fetch
    VertexBufferLayout vertexBufferLayout = Default;
    // {Begin block 'Describe the vertex buffer layout' (in root '032 - A first Vertex Attribute - Next')}
    VertexAttribute positionAttrib = Default;
    // {Begin block 'Describe the position attribute' (in root '032 - A first Vertex Attribute - Next')}
    // == For each attribute, describe its layout, i.e., how to interpret the raw data ==
    // Corresponds to @location(...)
    positionAttrib.shaderLocation = 0;
    // Means vec2f in the shader
    positionAttrib.format = VertexFormat::Float32x2;
    // Index of the first element
    positionAttrib.offset = 0;
    // {End block 'Describe the position attribute' (in root '032 - A first Vertex Attribute - Next')}
    
    vertexBufferLayout.attributeCount = 1;
    vertexBufferLayout.attributes = &positionAttrib;
    
    // {Begin block 'Describe buffer stride and step mode' (in root '032 - A first Vertex Attribute - Next')}
    // == Common to attributes from the same buffer ==
    vertexBufferLayout.arrayStride = 2 * sizeof(float);
    vertexBufferLayout.stepMode = VertexStepMode::Vertex;
    // {End block 'Describe buffer stride and step mode' (in root '032 - A first Vertex Attribute - Next')}
    // {End block 'Describe the vertex buffer layout' (in root '032 - A first Vertex Attribute - Next')}
    
    // When describing the render pipeline:
    pipelineDesc.vertex.bufferCount = 1;
    pipelineDesc.vertex.buffers = &vertexBufferLayout;
    // {End block 'Describe vertex buffers' (in root '032 - A first Vertex Attribute - Next')}
    pipelineDesc.vertex.module = shaderModule;
    pipelineDesc.vertex.entryPoint = StringView("vs_main");
    FragmentState fragmentState = Default;
    // {Begin block 'Describe fragment state' (in root '030 - Hello Triangle - Next')}
    fragmentState.module = shaderModule;
    fragmentState.entryPoint = StringView("fs_main");
    ColorTargetState colorTarget = Default;
    // {Begin block 'Describe color target state' (in root '030 - Hello Triangle - Next')}
    colorTarget.format = m_surfaceFormat;
    BlendState blendState = Default;
    colorTarget.blend = &blendState;
    // {End block 'Describe color target state' (in root '030 - Hello Triangle - Next')}
    fragmentState.targetCount = 1;
    fragmentState.targets = &colorTarget;
    // {End block 'Describe fragment state' (in root '030 - Hello Triangle - Next')}
    pipelineDesc.fragment = &fragmentState;
    // {End block 'Describe render pipeline' (in root '032 - A first Vertex Attribute - Next')}
    m_pipeline = m_device.createRenderPipeline(pipelineDesc);
    // {End block 'Create Render Pipeline' (in root '030 - Hello Triangle - Next')}
    shaderModule.release();
    // {End block 'Initialize pipeline' (in root '030 - Hello Triangle - Next')}
    return true;
}
// {End block 'InitializePipeline method' (in root '030 - Hello Triangle - Next')}
bool Application::Initialize() {
	// Move the whole initialization here
	// {Begin block 'Initialize' (in root '032 - A first Vertex Attribute - Next')}
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
	
	// {Begin block 'Surface Configuration' (in root '030 - Hello Triangle - Next')}
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
	m_surfaceFormat = config.format;
	// {End block 'Surface Configuration' (in root '030 - Hello Triangle - Next')}
	
	// We no longer need to access the adapter
	adapter.release(); // NEW
	// At the end of Initialize()
	if (!InitializePipeline()) return false;
	// At the end of Initialize()
	if (!InitializeBuffers()) return false;
	// {End block 'Initialize' (in root '032 - A first Vertex Attribute - Next')}
	return true;
}

void Application::Terminate() {
	// Move all the release/destroy/terminate calls here
	// {Begin block 'Terminate' (in root '032 - A first Vertex Attribute - Next')}
	// At the beginning of Terminate()
	m_vertexBuffer.release();
	m_pipeline.release();
	m_surface.unconfigure(); // NEW
	m_queue.release(); // NEW
	// {Begin block 'Destroy surface' (in root '028 - C++ Wrapper - Next')}
	m_surface.release(); // NEW
	// {End block 'Destroy surface' (in root '028 - C++ Wrapper - Next')}
	m_device.release(); // NEW
	glfwDestroyWindow(m_window);
	glfwTerminate();
	// {End block 'Terminate' (in root '032 - A first Vertex Attribute - Next')}
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
	RenderPassColorAttachment colorAttachment = Default; // NEW
	// {Begin block 'Describe the attachment' (in root '028 - C++ Wrapper - Next')}
	colorAttachment.view = targetView;
	colorAttachment.loadOp = LoadOp::Clear; // NEW
	colorAttachment.storeOp = StoreOp::Store; // NEW
	colorAttachment.clearValue = Color{ 1.0, 0.8, 0.55, 1.0 }; // NEW
	// {End block 'Describe the attachment' (in root '028 - C++ Wrapper - Next')}
	renderPassDesc.colorAttachmentCount = 1;
	renderPassDesc.colorAttachments = &colorAttachment;
	// {End block 'Describe Render Pass' (in root '028 - C++ Wrapper - Next')}
	
	RenderPassEncoder renderPass = encoder.beginRenderPass(renderPassDesc); // NEW
	// {Begin block 'Use Render Pass' (in root '030 - Hello Triangle - Next')}
	// {Begin block 'Draw a triangle' (in root '032 - A first Vertex Attribute - Next')}
	renderPass.setPipeline(m_pipeline);
	
	// Set vertex buffer while encoding the render pass
	renderPass.setVertexBuffer(0, m_vertexBuffer, 0, m_vertexBuffer.getSize());
	
	// We use the `m_vertexCount` variable instead of hard-coding the vertex count
	renderPass.draw(m_vertexCount, 1, 0, 0);
	// {End block 'Draw a triangle' (in root '032 - A first Vertex Attribute - Next')}
	// {End block 'Use Render Pass' (in root '030 - Hello Triangle - Next')}
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
// {Begin block 'InitializeBuffers method' (in root '032 - A first Vertex Attribute - Next')}
bool Application::InitializeBuffers() {
	// {Begin block 'Define vertex data' (in root '032 - A first Vertex Attribute - Next')}
	// Vertex buffer data
	// There are 2 floats per vertex, one for x and one for y.
	std::vector<float> vertexData = {
		// Define a first triangle:
		-0.45f, 0.5f,
		0.45f, 0.5f,
		0.0f, -0.5f,
	
		// Add a second triangle:
		0.47f, 0.47f,
		0.25f, 0.0f,
		0.69f, 0.0f
	};
	
	m_vertexCount = static_cast<uint32_t>(vertexData.size() / 2);
	// {End block 'Define vertex data' (in root '032 - A first Vertex Attribute - Next')}
	// {Begin block 'Create vertex buffer' (in root '032 - A first Vertex Attribute - Next')}
	// Create vertex buffer
	BufferDescriptor bufferDesc = Default;
	bufferDesc.size = vertexData.size() * sizeof(float);
	bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Vertex; // Vertex usage here!
	m_vertexBuffer = m_device.createBuffer(bufferDesc);
	
	// Upload geometry data to the buffer
	m_queue.writeBuffer(m_vertexBuffer, 0, vertexData.data(), bufferDesc.size);
	// {End block 'Create vertex buffer' (in root '032 - A first Vertex Attribute - Next')}
	return true;
}
// {End block 'InitializeBuffers method' (in root '032 - A first Vertex Attribute - Next')}
// {End block 'Application implementation' (in root '032 - A first Vertex Attribute - Next')}
// {End block 'file: Application.cpp' (in root '020 - Opening a window - Next')}