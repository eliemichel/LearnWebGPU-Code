// {Begin block 'file: Application.cpp' (in root '020 - Opening a window - Next')}
// {Begin block 'Includes in Application.cpp' (in root '037 - Loading from file - Next')}
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
// {End block 'Includes in Application.cpp' (in root '037 - Loading from file - Next')}

// {Begin block 'Application implementation' (in root '039 - A first uniform - Next')}
// {Begin block 'Shader source literal' (in root '037 - Loading from file - Next')}
// {End block 'Shader source literal' (in root '037 - Loading from file - Next')}
// {Begin block 'InitializePipeline method' (in root '030 - Hello Triangle - Next')}
bool Application::InitializePipeline() {
    // {Begin block 'Initialize pipeline' (in root '030 - Hello Triangle - Next')}
    // In Initialize() or in a dedicated InitializePipeline()
    // {Begin block 'Create Shader Module' (in root '037 - Loading from file - Next')}
    std::cout << "Creating shader module..." << std::endl;
    ShaderModule shaderModule = ResourceManager::loadShaderModule(RESOURCE_DIR "/shader.wgsl", m_device);
    std::cout << "Shader module: " << shaderModule << std::endl;
    
    // Check for errors
    if (shaderModule == nullptr) return false;
    // {End block 'Create Shader Module' (in root '037 - Loading from file - Next')}
    // {Begin block 'Create Render Pipeline' (in root '030 - Hello Triangle - Next')}
    RenderPipelineDescriptor pipelineDesc = Default;
    // {Begin block 'Describe render pipeline' (in root '039 - A first uniform - Next')}
    // {Begin block 'Describe vertex buffers' (in root '032 - A first Vertex Attribute - Next')}
    // Vertex fetch
    VertexBufferLayout vertexBufferLayout = Default;
    // {Begin block 'Describe the vertex buffer layout' (in root '033 - Multiple Attributes - Option A - Next')}
    // We now have 2 attributes
    std::vector<VertexAttribute> vertexAttribs(2);
    
    // {Begin block 'Describe the position attribute' (in root '033 - Multiple Attributes - Option A - Next')}
    // Describe the position attribute
    vertexAttribs[0].shaderLocation = 0; // @location(0)
    vertexAttribs[0].format = VertexFormat::Float32x2;
    vertexAttribs[0].offset = 0;
    // {End block 'Describe the position attribute' (in root '033 - Multiple Attributes - Option A - Next')}
    // {Begin block 'Describe the color attribute' (in root '033 - Multiple Attributes - Option A - Next')}
    // Describe the color attribute
    vertexAttribs[1].shaderLocation = 1; // @location(1)
    vertexAttribs[1].format = VertexFormat::Float32x3; // different type!
    vertexAttribs[1].offset = 2 * sizeof(float); // non null offset!
    // {End block 'Describe the color attribute' (in root '033 - Multiple Attributes - Option A - Next')}
    
    vertexBufferLayout.attributeCount = static_cast<uint32_t>(vertexAttribs.size());
    vertexBufferLayout.attributes = vertexAttribs.data();
    
    // {Begin block 'Describe buffer stride and step mode' (in root '033 - Multiple Attributes - Option A - Next')}
    vertexBufferLayout.arrayStride = 5 * sizeof(float);
    //                               ^^^^^^^^^^^^^^^^^ The new stride
    vertexBufferLayout.stepMode = VertexStepMode::Vertex;
    // {End block 'Describe buffer stride and step mode' (in root '033 - Multiple Attributes - Option A - Next')}
    // {End block 'Describe the vertex buffer layout' (in root '033 - Multiple Attributes - Option A - Next')}
    
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
    // {Begin block 'Create pipeline layout' (in root '039 - A first uniform - Next')}
    // {Begin block 'Define bindingLayout' (in root '044 - Dynamic uniforms - Next')}
    // Define binding layout (don't forget to = Default)
    BindGroupLayoutEntry bindingLayout = Default;
    
    // The binding index as used in the @binding attribute in the shader
    bindingLayout.binding = 0;
    
    // The stage that needs to access this resource
    bindingLayout.visibility = ShaderStage::Vertex;
    bindingLayout.buffer.type = BufferBindingType::Uniform;
    bindingLayout.buffer.minBindingSize = 4 * sizeof(float);
    bindingLayout.buffer.minBindingSize = sizeof(MyUniforms);
    bindingLayout.visibility = ShaderStage::Vertex | ShaderStage::Fragment;
    // After declaring the uniform's `bindingLayout` in InitializePipeline():
    
    // Make this binding dynamic so we can offset it between draw calls
    bindingLayout.buffer.hasDynamicOffset = true;
    // {End block 'Define bindingLayout' (in root '044 - Dynamic uniforms - Next')}
    
    // Create a bind group layout
    BindGroupLayoutDescriptor bindGroupLayoutDesc = Default;
    bindGroupLayoutDesc.entryCount = 1;
    bindGroupLayoutDesc.entries = &bindingLayout;
    m_bindGroupLayout = m_device.createBindGroupLayout(bindGroupLayoutDesc);
    
    // Create the pipeline layout
    PipelineLayoutDescriptor layoutDesc = Default;
    layoutDesc.bindGroupLayoutCount = 1;
    layoutDesc.bindGroupLayouts = (const WGPUBindGroupLayout*)&m_bindGroupLayout;
    m_layout = m_device.createPipelineLayout(layoutDesc);
    // {End block 'Create pipeline layout' (in root '039 - A first uniform - Next')}
    
    // Assign the PipelineLayout to the RenderPipelineDescriptor's layout field
    pipelineDesc.layout = m_layout;
    // {End block 'Describe render pipeline' (in root '039 - A first uniform - Next')}
    m_pipeline = m_device.createRenderPipeline(pipelineDesc);
    // {End block 'Create Render Pipeline' (in root '030 - Hello Triangle - Next')}
    shaderModule.release();
    // {End block 'Initialize pipeline' (in root '030 - Hello Triangle - Next')}
    return true;
}
// {End block 'InitializePipeline method' (in root '030 - Hello Triangle - Next')}
bool Application::Initialize() {
	// Move the whole initialization here
	// {Begin block 'Initialize' (in root '039 - A first uniform - Next')}
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
	// At the end of Initialize()
	InitializeBindGroups();
	// {End block 'Initialize' (in root '039 - A first uniform - Next')}
	return true;
}

void Application::Terminate() {
	// Move all the release/destroy/terminate calls here
	// {Begin block 'Terminate' (in root '039 - A first uniform - Next')}
	m_bindGroup.release();
	m_layout.release();
	m_bindGroupLayout.release();
	m_uniformBuffer.release();
	m_pointBuffer.release();
	m_indexBuffer.release();
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
	// {End block 'Terminate' (in root '039 - A first uniform - Next')}
}

void Application::MainLoop() {
	glfwPollEvents();
	m_instance.processEvents(); // NEW

	// {Begin block 'Main loop content' (in root '039 - A first uniform - Next')}
	// {Begin block 'Update uniform buffer' (in root '043 - More uniforms - Next')}
	float time = static_cast<float>(glfwGetTime());
	// Upload only the time, whichever its order in the struct
	m_queue.writeBuffer(m_uniformBuffer, offsetof(MyUniforms, time), &time, sizeof(time));
	// {End block 'Update uniform buffer' (in root '043 - More uniforms - Next')}
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
	// {Begin block 'Describe the attachment' (in root '033 - Multiple Attributes - Option A - Next')}
	colorAttachment.view = targetView;
	colorAttachment.loadOp = LoadOp::Clear; // NEW
	colorAttachment.storeOp = StoreOp::Store; // NEW
	colorAttachment.clearValue = Color{ 0.25, 0.25, 0.25, 1.0 }; // NEW
	// {End block 'Describe the attachment' (in root '033 - Multiple Attributes - Option A - Next')}
	renderPassDesc.colorAttachmentCount = 1;
	renderPassDesc.colorAttachments = &colorAttachment;
	// {End block 'Describe Render Pass' (in root '028 - C++ Wrapper - Next')}
	
	RenderPassEncoder renderPass = encoder.beginRenderPass(renderPassDesc); // NEW
	// {Begin block 'Use Render Pass' (in root '044 - Dynamic uniforms - Next')}
	renderPass.setPipeline(m_pipeline);
	renderPass.setVertexBuffer(0, m_pointBuffer, 0, m_pointBuffer.getSize());
	renderPass.setIndexBuffer(m_indexBuffer, IndexFormat::Uint16, 0, m_indexBuffer.getSize());
	
	uint32_t dynamicOffset = 0;
	
	// Set binding group
	dynamicOffset = 0 * m_uniformStride;
	renderPass.setBindGroup(0, m_bindGroup, 1, &dynamicOffset);
	renderPass.drawIndexed(m_indexCount, 1, 0, 0, 0);
	
	// Set binding group with a different uniform offset
	dynamicOffset = 1 * m_uniformStride;
	renderPass.setBindGroup(0, m_bindGroup, 1, &dynamicOffset);
	renderPass.drawIndexed(m_indexCount, 1, 0, 0, 0);
	// {End block 'Use Render Pass' (in root '044 - Dynamic uniforms - Next')}
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
	// {End block 'Main loop content' (in root '039 - A first uniform - Next')}
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
// {Begin block 'InitializeBuffers method' (in root '039 - A first uniform - Next')}
bool Application::InitializeBuffers() {
	// 1. Load from disk into CPU-side vectors pointData and indexData
	// {Begin block 'Load geometry data from file' (in root '037 - Loading from file - Next')}
	// Define data vectors, but without filling them in
	std::vector<float> pointData;
	std::vector<uint16_t> indexData;
	// Here we use the new 'loadGeometry' function:
	bool success = ResourceManager::loadGeometry(RESOURCE_DIR  "/webgpu.txt", pointData, indexData);
	if (!success) return false;
	
	m_indexCount = static_cast<uint32_t>(indexData.size());
	// {End block 'Load geometry data from file' (in root '037 - Loading from file - Next')}

	// 2. Create GPU buffers and upload data to them
	// {Begin block 'Create point buffer' (in root '034 - Index Buffer - Next')}
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
	// {End block 'Create point buffer' (in root '034 - Index Buffer - Next')}
	// {Begin block 'Create index buffer' (in root '034 - Index Buffer - Next')}
	// Create index buffer
	// (we reuse the bufferDesc initialized for the vertexBuffer)
	bufferDesc.size = indexData.size() * sizeof(uint16_t);
	// {Begin block 'Fix buffer size' (in root '034 - Index Buffer - Next')}
	bufferDesc.size = (bufferDesc.size + 3) & ~3; // round up to the next multiple of 4
	indexData.resize((indexData.size() + 1) & ~1); // round up to the next multiple of 2
	// {End block 'Fix buffer size' (in root '034 - Index Buffer - Next')}
	bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Index;
	m_indexBuffer = m_device.createBuffer(bufferDesc);
	
	m_queue.writeBuffer(m_indexBuffer, 0, indexData.data(), bufferDesc.size);
	// {End block 'Create index buffer' (in root '034 - Index Buffer - Next')}

	// 3. Create and fill uniform buffer <-- HERE
	// {Begin block 'Create uniform buffer' (in root '044 - Dynamic uniforms - Next')}
	// {Begin block 'Compute uniform stride' (in root '044 - Dynamic uniforms - Next')}
	Limits deviceLimits = Default;
	m_device.getLimits(&deviceLimits);
	
	// Subtlety
	m_uniformStride = ceilToNextMultiple(
		(uint32_t)sizeof(MyUniforms),
		(uint32_t)deviceLimits.minUniformBufferOffsetAlignment
	);
	// {End block 'Compute uniform stride' (in root '044 - Dynamic uniforms - Next')}
	
	// The buffer now contains 2 values for the uniforms plus the space in between:
	// (NB: stride = sizeof(MyUniforms) + spacing)
	bufferDesc.size = m_uniformStride + sizeof(MyUniforms);
		
	bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Uniform;
	m_uniformBuffer = m_device.createBuffer(bufferDesc);
	// {End block 'Create uniform buffer' (in root '044 - Dynamic uniforms - Next')}
	// {Begin block 'Upload uniform values' (in root '044 - Dynamic uniforms - Next')}
	MyUniforms uniforms;
	
	// Upload first value
	uniforms.time = 1.0f;
	uniforms.color = { 0.0f, 1.0f, 0.4f, 1.0f };
	m_queue.writeBuffer(m_uniformBuffer, 0, &uniforms, sizeof(uniforms));
	
	// Upload second value
	uniforms.time = -1.0f;
	uniforms.color = { 1.0f, 1.0f, 1.0f, 0.7f };
	m_queue.writeBuffer(m_uniformBuffer, m_uniformStride, &uniforms, sizeof(uniforms));
	//                                   ^^^^^^^^^^^^^^^ beware of the non-null offset!
	// {End block 'Upload uniform values' (in root '044 - Dynamic uniforms - Next')}
	return true;
}
// {End block 'InitializeBuffers method' (in root '039 - A first uniform - Next')}
// Add this in the main file
// {Begin block 'InitializeBindGroups method' (in root '039 - A first uniform - Next')}
void Application::InitializeBindGroups() {
	// {Begin block 'Create bind group' (in root '039 - A first uniform - Next')}
	// Create a binding
	BindGroupEntry binding = Default;
	// {Begin block 'Setup binding' (in root '043 - More uniforms - Next')}
	// The index of the binding (the entries in bindGroupDesc can be in any order)
	binding.binding = 0;
	// The buffer it is actually bound to
	binding.buffer = m_uniformBuffer;
	// We can specify an offset within the buffer, so that a single buffer can hold
	// multiple uniform blocks.
	binding.offset = 0;
	// And we specify again the size of the buffer.
	binding.size = 4 * sizeof(float);
	binding.size = sizeof(MyUniforms);
	// {End block 'Setup binding' (in root '043 - More uniforms - Next')}
	
	// A bind group contains one or multiple bindings
	BindGroupDescriptor bindGroupDesc = Default;
	bindGroupDesc.layout = m_bindGroupLayout;
	// There must be as many bindings as declared in the layout!
	bindGroupDesc.entryCount = 1;
	bindGroupDesc.entries = &binding;
	m_bindGroup = m_device.createBindGroup(bindGroupDesc);
	// {End block 'Create bind group' (in root '039 - A first uniform - Next')}
}
// {End block 'InitializeBindGroups method' (in root '039 - A first uniform - Next')}
// {End block 'Application implementation' (in root '039 - A first uniform - Next')}
// {End block 'file: Application.cpp' (in root '020 - Opening a window - Next')}