// {Begin block 'file: Application.cpp' (in root '020 - Opening a window - Next')}
// {Begin block 'Includes in Application.cpp' (in root '037 - Loading from file - Next - vanilla')}
// In Application.cpp
#include "Application.h"
#include "webgpu-utils.h"

#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
// In Application.cpp
#include <glfw3webgpu.h>
// In main.cpp
#include "ResourceManager.h"
// {End block 'Includes in Application.cpp' (in root '037 - Loading from file - Next - vanilla')}

// {Begin block 'Application implementation' (in root '039 - A first uniform - Next - vanilla')}
// {Begin block 'Shader source literal' (in root '037 - Loading from file - Next - vanilla')}
// {End block 'Shader source literal' (in root '037 - Loading from file - Next - vanilla')}
// {Begin block 'InitializePipeline method' (in root '030 - Hello Triangle - Next - vanilla')}
bool Application::InitializePipeline() {
    // {Begin block 'Initialize pipeline' (in root '030 - Hello Triangle - Next - vanilla')}
    // In Initialize() or in a dedicated InitializePipeline()
    // {Begin block 'Create Shader Module' (in root '037 - Loading from file - Next - vanilla')}
    std::cout << "Creating shader module..." << std::endl;
    WGPUShaderModule shaderModule = ResourceManager::loadShaderModule(RESOURCE_DIR "/shader.wgsl", m_device);
    std::cout << "Shader module: " << shaderModule << std::endl;
    
    // Check for errors
    if (shaderModule == nullptr) return false;
    // {End block 'Create Shader Module' (in root '037 - Loading from file - Next - vanilla')}
    // {Begin block 'Create Render Pipeline' (in root '030 - Hello Triangle - Next - vanilla')}
    WGPURenderPipelineDescriptor pipelineDesc = WGPU_RENDER_PIPELINE_DESCRIPTOR_INIT;
    // {Begin block 'Describe render pipeline' (in root '052 - Depth buffer - Next - vanilla')}
    // {Begin block 'Describe vertex buffers' (in root '032 - A first Vertex Attribute - Next - vanilla')}
    // Vertex fetch
    WGPUVertexBufferLayout vertexBufferLayout = WGPU_VERTEX_BUFFER_LAYOUT_INIT;
    // {Begin block 'Describe the vertex buffer layout' (in root '033 - Multiple Attributes - Option A - Next - vanilla')}
    // We now have 2 attributes
    std::vector<WGPUVertexAttribute> vertexAttribs(2);
    
    // {Begin block 'Describe the position attribute' (in root '050 - A simple example - Next - vanilla')}
    // Position attribute
    vertexAttribs[0].shaderLocation = 0; // @location(0)
    vertexAttribs[0].format = WGPUVertexFormat_Float32x3;
    //                                                 ^ This was a 2
    vertexAttribs[0].offset = 0;
    // {End block 'Describe the position attribute' (in root '050 - A simple example - Next - vanilla')}
    // {Begin block 'Describe the color attribute' (in root '050 - A simple example - Next - vanilla')}
    // Color attribute
    vertexAttribs[1].shaderLocation = 1; // @location(1)
    vertexAttribs[1].format = WGPUVertexFormat_Float32x3;
    vertexAttribs[1].offset = 3 * sizeof(float);
    //                        ^ This was a 2
    // {End block 'Describe the color attribute' (in root '050 - A simple example - Next - vanilla')}
    
    vertexBufferLayout.attributeCount = static_cast<uint32_t>(vertexAttribs.size());
    vertexBufferLayout.attributes = vertexAttribs.data();
    
    // {Begin block 'Describe buffer stride and step mode' (in root '050 - A simple example - Next - vanilla')}
    // The buffer stride
    vertexBufferLayout.arrayStride = 6 * sizeof(float);
    //                               ^ This was a 5
    vertexBufferLayout.stepMode = WGPUVertexStepMode_Vertex;
    // {End block 'Describe buffer stride and step mode' (in root '050 - A simple example - Next - vanilla')}
    // {End block 'Describe the vertex buffer layout' (in root '033 - Multiple Attributes - Option A - Next - vanilla')}
    
    // When describing the render pipeline:
    pipelineDesc.vertex.bufferCount = 1;
    pipelineDesc.vertex.buffers = &vertexBufferLayout;
    // {End block 'Describe vertex buffers' (in root '032 - A first Vertex Attribute - Next - vanilla')}
    pipelineDesc.vertex.module = shaderModule;
    pipelineDesc.vertex.entryPoint = toWgpuStringView("vs_main");
    WGPUFragmentState fragmentState = WGPU_FRAGMENT_STATE_INIT;
    // {Begin block 'Describe fragment state' (in root '030 - Hello Triangle - Next - vanilla')}
    fragmentState.module = shaderModule;
    fragmentState.entryPoint = toWgpuStringView("fs_main");
    WGPUColorTargetState colorTarget = WGPU_COLOR_TARGET_STATE_INIT;
    // {Begin block 'Describe color target state' (in root '030 - Hello Triangle - Next - vanilla')}
    colorTarget.format = m_surfaceFormat;
    WGPUBlendState blendState = WGPU_BLEND_STATE_INIT;
    colorTarget.blend = &blendState;
    // {End block 'Describe color target state' (in root '030 - Hello Triangle - Next - vanilla')}
    fragmentState.targetCount = 1;
    fragmentState.targets = &colorTarget;
    // {End block 'Describe fragment state' (in root '030 - Hello Triangle - Next - vanilla')}
    pipelineDesc.fragment = &fragmentState;
    // {Begin block 'Create pipeline layout' (in root '039 - A first uniform - Next - vanilla')}
    // {Begin block 'Define bindingLayout' (in root '043 - More uniforms - Next - vanilla')}
    // Define binding layout
    WGPUBindGroupLayoutEntry bindingLayout = WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT;
    
    // The binding index as used in the @binding attribute in the shader
    bindingLayout.binding = 0;
    
    // The stage that needs to access this resource
    bindingLayout.visibility = WGPUShaderStage_Vertex;
    bindingLayout.buffer.type = WGPUBufferBindingType_Uniform;
    bindingLayout.buffer.minBindingSize = 4 * sizeof(float);
    bindingLayout.buffer.minBindingSize = sizeof(MyUniforms);
    bindingLayout.visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
    // {End block 'Define bindingLayout' (in root '043 - More uniforms - Next - vanilla')}
    
    // Create a bind group layout
    WGPUBindGroupLayoutDescriptor bindGroupLayoutDesc = WGPU_BIND_GROUP_LAYOUT_DESCRIPTOR_INIT;
    bindGroupLayoutDesc.nextInChain = nullptr;
    bindGroupLayoutDesc.entryCount = 1;
    bindGroupLayoutDesc.entries = &bindingLayout;
    m_bindGroupLayout = wgpuDeviceCreateBindGroupLayout(m_device, &bindGroupLayoutDesc);
    
    // Create the pipeline layout
    WGPUPipelineLayoutDescriptor layoutDesc = WGPU_PIPELINE_LAYOUT_DESCRIPTOR_INIT;
    layoutDesc.nextInChain = nullptr;
    layoutDesc.bindGroupLayoutCount = 1;
    layoutDesc.bindGroupLayouts = &m_bindGroupLayout;
    m_layout = wgpuDeviceCreatePipelineLayout(m_device, &layoutDesc);
    // {End block 'Create pipeline layout' (in root '039 - A first uniform - Next - vanilla')}
    
    // Assign the PipelineLayout to the RenderPipelineDescriptor's layout field
    pipelineDesc.layout = m_layout;
    WGPUDepthStencilState depthStencilState = WGPU_DEPTH_STENCIL_STATE_INIT;
    // {Begin block 'Describe depth/stencil state' (in root '052 - Depth buffer - Next - vanilla')}
    depthStencilState.depthCompare = WGPUCompareFunction_Less;
    depthStencilState.depthWriteEnabled = WGPUOptionalBool_True;
    depthStencilState.format = m_depthTextureFormat;
    // {End block 'Describe depth/stencil state' (in root '052 - Depth buffer - Next - vanilla')}
    pipelineDesc.depthStencil = &depthStencilState;
    // {End block 'Describe render pipeline' (in root '052 - Depth buffer - Next - vanilla')}
    m_pipeline = wgpuDeviceCreateRenderPipeline(m_device, &pipelineDesc);
    // {End block 'Create Render Pipeline' (in root '030 - Hello Triangle - Next - vanilla')}
    wgpuShaderModuleRelease(shaderModule);
    // {End block 'Initialize pipeline' (in root '030 - Hello Triangle - Next - vanilla')}
    return true;
}
// {End block 'InitializePipeline method' (in root '030 - Hello Triangle - Next - vanilla')}
bool Application::Initialize() {
	// Move the whole initialization here
	// {Begin block 'Initialize' (in root '052 - Depth buffer - Next - vanilla')}
	// {Begin block 'Open window and get adapter' (in root '020 - Opening a window - Next')}
	// Open window
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // <-- extra info for glfwCreateWindow
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	m_window = glfwCreateWindow(640, 480, "Learn WebGPU", nullptr, nullptr);
	
	// Create instance ('instance' is now declared at the class level)
	m_instance = wgpuCreateInstance(nullptr);
	
	// Get adapter
	std::cout << "Requesting adapter..." << std::endl;
	// {Begin block 'Request adapter' (in root '020 - Opening a window - Next')}
	// {Begin block 'Get the surface' (in root '020 - Opening a window - Next')}
	m_surface = glfwCreateWindowWGPUSurface(m_instance, m_window);
	// {End block 'Get the surface' (in root '020 - Opening a window - Next')}
	
	WGPURequestAdapterOptions adapterOpts = WGPU_REQUEST_ADAPTER_OPTIONS_INIT;
	adapterOpts.compatibleSurface = m_surface;
	//                              ^^^^^^^^^ Use the surface here
	
	WGPUAdapter adapter = requestAdapterSync(m_instance, &adapterOpts);
	// {End block 'Request adapter' (in root '020 - Opening a window - Next')}
	std::cout << "Got adapter: " << adapter << std::endl;
	// {End block 'Open window and get adapter' (in root '020 - Opening a window - Next')}
	
	// {Begin block 'Request device' (in root '020 - Opening a window - Next')}
	std::cout << "Requesting device..." << std::endl;
	WGPUDeviceDescriptor deviceDesc = WGPU_DEVICE_DESCRIPTOR_INIT;
	// {Begin block 'Build device descriptor' (in root '010 - The Device - Next')}
	// Any name works here, that's your call
	deviceDesc.label = toWgpuStringView("My Device");
	std::vector<WGPUFeatureName> features;
	// {Begin block 'List required features' (in root '010 - The Device - Next')}
	// No required feature for now
	// {End block 'List required features' (in root '010 - The Device - Next')}
	deviceDesc.requiredFeatureCount = features.size();
	deviceDesc.requiredFeatures = features.data();
	// Make sure 'features' lives until the call to wgpuAdapterRequestDevice!
	WGPULimits requiredLimits = WGPU_LIMITS_INIT;
	// {Begin block 'Specify required limits' (in root '010 - The Device - Next')}
	// We leave 'requiredLimits' untouched for now
	// {End block 'Specify required limits' (in root '010 - The Device - Next')}
	deviceDesc.requiredLimits = &requiredLimits;
	// Make sure that the 'requiredLimits' variable lives until the call to wgpuAdapterRequestDevice!
	deviceDesc.defaultQueue.label = toWgpuStringView("The Default Queue");
	// {Begin block 'Device Lost Callback' (in root '010 - The Device - Next')}
	auto onDeviceLost = [](
		WGPUDevice const * device,
		WGPUDeviceLostReason reason,
		struct WGPUStringView message,
		void* /* userdata1 */,
		void* /* userdata2 */
	) {
		// All we do is display a message when the device is lost
	    std::cout
	    	<< "Device " << device << " was lost: reason " << reason
	    	<< " (" << toStdStringView(message) << ")"
	    	<< std::endl;
	};
	// {End block 'Device Lost Callback' (in root '010 - The Device - Next')}
	deviceDesc.deviceLostCallbackInfo.callback = onDeviceLost;
	deviceDesc.deviceLostCallbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;
	// {Begin block 'Device Error Callback' (in root '010 - The Device - Next')}
	auto onDeviceError = [](
		WGPUDevice const * device,
		WGPUErrorType type,
		struct WGPUStringView message,
		void* /* userdata1 */,
		void* /* userdata2 */
	) {
	    std::cout
	    	<< "Uncaptured error in device " << device << ": type " << type
	    	<< " (" << toStdStringView(message) << ")"
	    	<< std::endl;
	};
	// {End block 'Device Error Callback' (in root '010 - The Device - Next')}
	deviceDesc.uncapturedErrorCallbackInfo.callback = onDeviceError;
	// {End block 'Build device descriptor' (in root '010 - The Device - Next')}
	// NB: 'device' is now declared at the class level
	m_device = requestDeviceSync(m_instance, adapter, &deviceDesc);
	std::cout << "Got device: " << m_device << std::endl;
	// {End block 'Request device' (in root '020 - Opening a window - Next')}
	
	m_queue = wgpuDeviceGetQueue(m_device);
	
	// {Begin block 'Surface Configuration' (in root '030 - Hello Triangle - Next - vanilla')}
	WGPUSurfaceConfiguration config = WGPU_SURFACE_CONFIGURATION_INIT;
	
	// {Begin block 'Describe the surface configuration' (in root '025 - First Color - Next')}
	// Configuration of the textures created for the underlying swap chain
	config.width = 640;
	config.height = 480;
	config.device = m_device;
	// {Begin block 'Describe surface format' (in root '025 - First Color - Next')}
	// We initialize an empty capability struct:
	WGPUSurfaceCapabilities capabilities = WGPU_SURFACE_CAPABILITIES_INIT;
	
	// We get the capabilities for a pair of (surface, adapter).
	// If it works, this populates the `capabilities` structure
	WGPUStatus status = wgpuSurfaceGetCapabilities(m_surface, adapter, &capabilities);
	if (status != WGPUStatus_Success) {
	    return false;
	}
	
	// From the capabilities, we get the preferred format: it is always the first one!
	// (NB: There is always at least 1 format if the GetCapabilities was successful)
	config.format = capabilities.formats[0];
	
	// We no longer need to access the capabilities, so we release their memory.
	wgpuSurfaceCapabilitiesFreeMembers(capabilities);
	// {End block 'Describe surface format' (in root '025 - First Color - Next')}
	config.presentMode = WGPUPresentMode_Fifo;
	config.alphaMode = WGPUCompositeAlphaMode_Auto;
	// {End block 'Describe the surface configuration' (in root '025 - First Color - Next')}
	
	wgpuSurfaceConfigure(m_surface, &config);
	m_surfaceFormat = config.format;
	// {End block 'Surface Configuration' (in root '030 - Hello Triangle - Next - vanilla')}
	// Create the depth texture after surface configuration 
	// Create the depth texture
	WGPUTextureDescriptor depthTextureDesc = WGPU_TEXTURE_DESCRIPTOR_INIT;
	depthTextureDesc.label = toWgpuStringView("Z Buffer");
	depthTextureDesc.usage = WGPUTextureUsage_RenderAttachment;
	depthTextureDesc.size = { 640, 480, 1 };
	depthTextureDesc.format = m_depthTextureFormat;
	WGPUTexture depthTexture = wgpuDeviceCreateTexture(m_device, &depthTextureDesc);
	// Create the view of the depth texture manipulated by the rasterizer
	m_depthTextureView = wgpuTextureCreateView(depthTexture, nullptr);
	
	// We can now release the texture and only hold to the view
	wgpuTextureRelease(depthTexture);
	
	// We no longer need to access the adapter
	wgpuAdapterRelease(adapter);
	// At the end of Initialize()
	if (!InitializePipeline()) return false;
	// At the end of Initialize()
	if (!InitializeBuffers()) return false;
	// At the end of Initialize()
	InitializeBindGroups();
	// {End block 'Initialize' (in root '052 - Depth buffer - Next - vanilla')}
	return true;
}

void Application::Terminate() {
	// Move all the release/destroy/terminate calls here
	// {Begin block 'Terminate' (in root '052 - Depth buffer - Next - vanilla')}
	// {Begin block 'Release the depth texture view' (in root '052 - Depth buffer - Next - vanilla')}
	// Release the depth texture view
	wgpuTextureViewRelease(m_depthTextureView);
	// {End block 'Release the depth texture view' (in root '052 - Depth buffer - Next - vanilla')}
	wgpuBindGroupRelease(m_bindGroup);
	wgpuPipelineLayoutRelease(m_layout);
	wgpuBindGroupLayoutRelease(m_bindGroupLayout);
	wgpuBufferRelease(m_uniformBuffer);
	wgpuBufferRelease(m_pointBuffer);
	wgpuBufferRelease(m_indexBuffer);
	// At the beginning of Terminate()
	wgpuBufferRelease(m_vertexBuffer);
	wgpuRenderPipelineRelease(m_pipeline);
	wgpuSurfaceUnconfigure(m_surface);
	wgpuQueueRelease(m_queue);
	// {Begin block 'Destroy surface' (in root '020 - Opening a window - Next')}
	wgpuSurfaceRelease(m_surface);
	// {End block 'Destroy surface' (in root '020 - Opening a window - Next')}
	wgpuDeviceRelease(m_device);
	glfwDestroyWindow(m_window);
	glfwTerminate();
	// {End block 'Terminate' (in root '052 - Depth buffer - Next - vanilla')}
}

void Application::MainLoop() {
	glfwPollEvents();
	wgpuInstanceProcessEvents(m_instance);
	//                        ^^ We add this prefix to member variables

	// {Begin block 'Main loop content' (in root '039 - A first uniform - Next - vanilla')}
	// {Begin block 'Update uniform buffer' (in root '043 - More uniforms - Next - vanilla')}
	float time = static_cast<float>(glfwGetTime());
	// Upload only the time, whichever its order in the struct
	wgpuQueueWriteBuffer(m_queue, m_uniformBuffer, offsetof(MyUniforms, time), &time, sizeof(time));
	// {End block 'Update uniform buffer' (in root '043 - More uniforms - Next - vanilla')}
	// In Application::MainLoop()
	// {Begin block 'Get the next target texture view' (in root '025 - First Color - Next')}
	// Get the next target texture view
	WGPUTextureView targetView = GetNextSurfaceView();
	if (!targetView) return; // no surface texture, we skip this frame
	// {End block 'Get the next target texture view' (in root '025 - First Color - Next')}
	// {Begin block 'Draw things' (in root '025 - First Color - Next')}
	// {Begin block 'Create Command Encoder' (in root '025 - First Color - Next')}
	WGPUCommandEncoderDescriptor encoderDesc = WGPU_COMMAND_ENCODER_DESCRIPTOR_INIT;
	encoderDesc.label = toWgpuStringView("My command encoder");
	WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(m_device, &encoderDesc);
	// {End block 'Create Command Encoder' (in root '025 - First Color - Next')}
	// {Begin block 'Encode Render Pass' (in root '025 - First Color - Next')}
	WGPURenderPassDescriptor renderPassDesc = WGPU_RENDER_PASS_DESCRIPTOR_INIT;
	// {Begin block 'Describe Render Pass' (in root '052 - Depth buffer - Next - vanilla')}
	WGPURenderPassColorAttachment colorAttachment = WGPU_RENDER_PASS_COLOR_ATTACHMENT_INIT;
	
	// {Begin block 'Describe the attachment' (in root '033 - Multiple Attributes - Option A - Next - vanilla')}
	colorAttachment.view = targetView;
	colorAttachment.loadOp = WGPULoadOp_Clear;
	colorAttachment.storeOp = WGPUStoreOp_Store;
	colorAttachment.clearValue = WGPUColor{ 0.25, 0.25, 0.25, 1.0 };
	// {End block 'Describe the attachment' (in root '033 - Multiple Attributes - Option A - Next - vanilla')}
	
	renderPassDesc.colorAttachmentCount = 1;
	renderPassDesc.colorAttachments = &colorAttachment;
	// We already had a color attachment
	// e.g., renderPassDesc.colorAttachments = &colorAttachment;
	
	// We now add a depth/stencil attachment:
	WGPURenderPassDepthStencilAttachment depthStencilAttachment = WGPU_RENDER_PASS_DEPTH_STENCIL_ATTACHMENT_INIT;
	// {Begin block 'Describe depth/stencil attachment' (in root '052 - Depth buffer - Next - vanilla')}
	// Describe depth/stencil attachment
	
	// The view of the depth texture
	depthStencilAttachment.view = m_depthTextureView;
	
	// The initial value of the depth buffer, meaning "far"
	depthStencilAttachment.depthClearValue = 1.0f;
	
	// Operation settings comparable to the color attachment
	depthStencilAttachment.depthLoadOp = WGPULoadOp_Clear;
	depthStencilAttachment.depthStoreOp = WGPUStoreOp_Store;
	
	// we could turn off writing to the depth buffer globally here
	depthStencilAttachment.depthReadOnly = false; // NB: this is the default
	// {End block 'Describe depth/stencil attachment' (in root '052 - Depth buffer - Next - vanilla')}
	renderPassDesc.depthStencilAttachment = &depthStencilAttachment;
	// {End block 'Describe Render Pass' (in root '052 - Depth buffer - Next - vanilla')}
	
	WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);
	// {Begin block 'Use Render Pass' (in root '039 - A first uniform - Next - vanilla')}
	wgpuRenderPassEncoderSetPipeline(renderPass, m_pipeline);
	wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, m_pointBuffer, 0, wgpuBufferGetSize(m_pointBuffer));
	wgpuRenderPassEncoderSetIndexBuffer(renderPass, m_indexBuffer, WGPUIndexFormat_Uint16, 0, wgpuBufferGetSize(m_indexBuffer));
	
	// Set binding group here!
	wgpuRenderPassEncoderSetBindGroup(renderPass, 0, m_bindGroup, 0, nullptr);
	
	wgpuRenderPassEncoderDrawIndexed(renderPass, m_indexCount, 1, 0, 0, 0);
	// {End block 'Use Render Pass' (in root '039 - A first uniform - Next - vanilla')}
	wgpuRenderPassEncoderEnd(renderPass);
	wgpuRenderPassEncoderRelease(renderPass);
	// {End block 'Encode Render Pass' (in root '025 - First Color - Next')}
	// {Begin block 'Finish encoding and submit' (in root '025 - First Color - Next')}
	WGPUCommandBufferDescriptor cmdBufferDescriptor = WGPU_COMMAND_BUFFER_DESCRIPTOR_INIT;
	cmdBufferDescriptor.label = toWgpuStringView("Command buffer");
	WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &cmdBufferDescriptor);
	wgpuCommandEncoderRelease(encoder); // release encoder after it's finished
	
	// Finally submit the command queue
	std::cout << "Submitting command..." << std::endl;
	wgpuQueueSubmit(m_queue, 1, &command);
	wgpuCommandBufferRelease(command);
	std::cout << "Command submitted." << std::endl;
	// {End block 'Finish encoding and submit' (in root '025 - First Color - Next')}
	// {End block 'Draw things' (in root '025 - First Color - Next')}
	// {Begin block 'Present the surface onto the window' (in root '025 - First Color - Next')}
	// At the end of the frame
	wgpuTextureViewRelease(targetView);
	#ifndef __EMSCRIPTEN__
	wgpuSurfacePresent(m_surface);
	#endif
	// {End block 'Present the surface onto the window' (in root '025 - First Color - Next')}
	// {End block 'Main loop content' (in root '039 - A first uniform - Next - vanilla')}
}

bool Application::IsRunning() {
	return !glfwWindowShouldClose(m_window);
}
// {Begin block 'GetNextSurfaceView method' (in root '025 - First Color - Next')}
WGPUTextureView Application::GetNextSurfaceView() {
    // {Begin block 'Get the next surface texture' (in root '025 - First Color - Next')}
    WGPUSurfaceTexture surfaceTexture = WGPU_SURFACE_TEXTURE_INIT;
    wgpuSurfaceGetCurrentTexture(m_surface, &surfaceTexture);
    if (
        surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal &&
        surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal
    ) {
        return nullptr;
    }
    // {End block 'Get the next surface texture' (in root '025 - First Color - Next')}
    // {Begin block 'Create surface texture view' (in root '025 - First Color - Next')}
    WGPUTextureViewDescriptor viewDescriptor = WGPU_TEXTURE_VIEW_DESCRIPTOR_INIT;
    viewDescriptor.label = toWgpuStringView("Surface texture view");
    viewDescriptor.dimension = WGPUTextureViewDimension_2D; // not to confuse with 2DArray
    WGPUTextureView targetView = wgpuTextureCreateView(surfaceTexture.texture, &viewDescriptor);
    // {End block 'Create surface texture view' (in root '025 - First Color - Next')}
    // {Begin block 'Release the texture' (in root '025 - First Color - Next')}
    // We no longer need the texture, only its view,
    // so we release it at the end of GetNextSurfaceViewData
    wgpuTextureRelease(surfaceTexture.texture);
    // {End block 'Release the texture' (in root '025 - First Color - Next')}
    return targetView;
}
// {End block 'GetNextSurfaceView method' (in root '025 - First Color - Next')}
// {Begin block 'InitializeBuffers method' (in root '039 - A first uniform - Next - vanilla')}
bool Application::InitializeBuffers() {
	// 1. Load from disk into CPU-side vectors pointData and indexData
	// {Begin block 'Load geometry data from file' (in root '050 - A simple example - Next - vanilla')}
	std::vector<float> pointData;
	std::vector<uint16_t> indexData;
	
	bool success = ResourceManager::loadGeometry(
		RESOURCE_DIR  "/pyramid.txt", // <-- switch to the pyramid
		pointData,
		indexData,
		3 /* dimensions */ // <-- new argument
	);
	if (!success) return false;
	
	m_indexCount = static_cast<uint32_t>(indexData.size());
	// {End block 'Load geometry data from file' (in root '050 - A simple example - Next - vanilla')}

	// 2. Create GPU buffers and upload data to them
	// {Begin block 'Create point buffer' (in root '034 - Index Buffer - Next - vanilla')}
	// Create point buffer
	WGPUBufferDescriptor bufferDesc = WGPU_BUFFER_DESCRIPTOR_INIT;
	bufferDesc.size = pointData.size() * sizeof(float);
	bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex; // Vertex usage here!
	m_pointBuffer = wgpuDeviceCreateBuffer(m_device, &bufferDesc);
	
	// Upload geometry data to the buffer
	wgpuQueueWriteBuffer(m_queue, m_pointBuffer, 0, pointData.data(), bufferDesc.size);
	// It is not easy with the auto-generation of code to remove the previously
	// defined `vertexBuffer` attribute, but at the same time some compilers
	// (rightfully) complain if we do not use it. This is a hack to mark the
	// variable as used and have automated build tests pass.
	(void)m_vertexBuffer;
	(void)m_vertexCount;
	// {End block 'Create point buffer' (in root '034 - Index Buffer - Next - vanilla')}
	// {Begin block 'Create index buffer' (in root '034 - Index Buffer - Next - vanilla')}
	// Create index buffer
	// (we reuse the bufferDesc initialized for the vertexBuffer)
	bufferDesc.size = indexData.size() * sizeof(uint16_t);
	// {Begin block 'Fix buffer size' (in root '034 - Index Buffer - Next - vanilla')}
	bufferDesc.size = (bufferDesc.size + 3) & ~3; // round up to the next multiple of 4
	indexData.resize((indexData.size() + 1) & ~1); // round up to the next multiple of 2
	// {End block 'Fix buffer size' (in root '034 - Index Buffer - Next - vanilla')}
	bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index;;
	m_indexBuffer = wgpuDeviceCreateBuffer(m_device, &bufferDesc);
	
	wgpuQueueWriteBuffer(m_queue, m_indexBuffer, 0, indexData.data(), bufferDesc.size);
	// {End block 'Create index buffer' (in root '034 - Index Buffer - Next - vanilla')}

	// 3. Create and fill uniform buffer <-- HERE
	// {Begin block 'Create uniform buffer' (in root '043 - More uniforms - Next - vanilla')}
	bufferDesc.size = sizeof(MyUniforms);
	//                ^^^^^^^^^^^^^^^^^^ This was 4 * sizeof(float)
	
	bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform;
	m_uniformBuffer = wgpuDeviceCreateBuffer(m_device, &bufferDesc);
	// {End block 'Create uniform buffer' (in root '043 - More uniforms - Next - vanilla')}
	// {Begin block 'Upload uniform values' (in root '043 - More uniforms - Next - vanilla')}
	// Upload the initial value of the uniforms
	MyUniforms uniforms;
	uniforms.time = 1.0f;
	uniforms.color = { 0.0f, 1.0f, 0.4f, 1.0f };
	wgpuQueueWriteBuffer(m_queue, m_uniformBuffer, 0, &uniforms, sizeof(uniforms));
	// {End block 'Upload uniform values' (in root '043 - More uniforms - Next - vanilla')}
	return true;
}
// {End block 'InitializeBuffers method' (in root '039 - A first uniform - Next - vanilla')}
// Add this in the main file
// {Begin block 'InitializeBindGroups method' (in root '039 - A first uniform - Next - vanilla')}
void Application::InitializeBindGroups() {
	// {Begin block 'Create bind group' (in root '039 - A first uniform - Next - vanilla')}
	// Create a binding
	WGPUBindGroupEntry binding = WGPU_BIND_GROUP_ENTRY_INIT;
	// {Begin block 'Setup binding' (in root '043 - More uniforms - Next - vanilla')}
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
	// {End block 'Setup binding' (in root '043 - More uniforms - Next - vanilla')}
	
	// A bind group contains one or multiple bindings
	WGPUBindGroupDescriptor bindGroupDesc = WGPU_BIND_GROUP_DESCRIPTOR_INIT;
	bindGroupDesc.layout = m_bindGroupLayout;
	// There must be as many bindings as declared in the layout!
	bindGroupDesc.entryCount = 1;
	bindGroupDesc.entries = &binding;
	m_bindGroup = wgpuDeviceCreateBindGroup(m_device, &bindGroupDesc);
	// {End block 'Create bind group' (in root '039 - A first uniform - Next - vanilla')}
}
// {End block 'InitializeBindGroups method' (in root '039 - A first uniform - Next - vanilla')}
// {End block 'Application implementation' (in root '039 - A first uniform - Next - vanilla')}
// {End block 'file: Application.cpp' (in root '020 - Opening a window - Next')}