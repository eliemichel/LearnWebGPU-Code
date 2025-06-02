// In Application.cpp
#include "Application.h"
#include "webgpu-utils.h"

#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
// In Application.cpp
#include <glfw3webgpu.h>
#include <cassert>

const char* shaderSource = R"(
/**
 * A structure with fields labeled with vertex attribute locations can be used
 * as input to the entry point of a shader.
 */
struct VertexInput {
	@location(0) position: vec2f,
	@location(1) color: vec3f,
};
/**
 * A structure with fields labeled with builtins and locations can also be used
 * as *output* of the vertex shader, which is also the input of the fragment
 * shader.
 */
struct VertexOutput {
	@builtin(position) position: vec4f,
	// The location here does not refer to a vertex attribute, it just means
	// that this field must be handled by the rasterizer.
	// (It can also refer to another field of another struct that would be used
	// as input to the fragment shader.)
	@location(0) color: vec3f,
};

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
	//                         ^^^^^^^^^^^^ We return a custom struct
	// In vs_main()
	var out: VertexOutput; // create the output struct
	out.position = vec4f(in.position, 0.0, 1.0); // same as what we used to directly return
	out.color = in.color; // forward the color attribute to the fragment shader
	return out;
}

@fragment
// Or we can use a custom struct whose fields are labeled
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
	//     ^^^^^^^^^^^^^^^^ Use for instance the same struct as what the vertex outputs
	// In fs_main()
	return vec4f(in.color, 1.0); // use the interpolated color coming from the vertex shader
}
)";
bool Application::InitializePipeline() {
    // In Initialize() or in a dedicated InitializePipeline()
    WGPUShaderSourceWGSL wgslDesc = WGPU_SHADER_SOURCE_WGSL_INIT;
    wgslDesc.code = toWgpuStringView(shaderSource);
    WGPUShaderModuleDescriptor shaderDesc = WGPU_SHADER_MODULE_DESCRIPTOR_INIT;
    shaderDesc.nextInChain = &wgslDesc.chain; // connect the chained extension
    shaderDesc.label = toWgpuStringView("Shader source from Application.cpp");
    WGPUShaderModule shaderModule = wgpuDeviceCreateShaderModule(m_device, &shaderDesc);
    WGPURenderPipelineDescriptor pipelineDesc = WGPU_RENDER_PIPELINE_DESCRIPTOR_INIT;
    // We now have 2 attributes in different buffers
    std::vector<WGPUVertexBufferLayout> vertexBufferLayouts(2);
    
    // Position attribute
    // Position attribute remains untouched
    WGPUVertexAttribute positionAttrib;
    positionAttrib.shaderLocation = 0; // @location(0)
    positionAttrib.format = WGPUVertexFormat_Float32x2; // size of position
    positionAttrib.offset = 0;
    
    vertexBufferLayouts[0].attributeCount = 1;
    vertexBufferLayouts[0].attributes = &positionAttrib;
    vertexBufferLayouts[0].arrayStride = 2 * sizeof(float); // stride = size of position
    vertexBufferLayouts[0].stepMode = WGPUVertexStepMode_Vertex;
    
    // Color attribute
    // Color attribute
    WGPUVertexAttribute colorAttrib;
    colorAttrib.shaderLocation = 1; // @location(1)
    colorAttrib.format = WGPUVertexFormat_Float32x3; // size of color
    colorAttrib.offset = 0;
    
    vertexBufferLayouts[1].attributeCount = 1;
    vertexBufferLayouts[1].attributes = &colorAttrib;
    vertexBufferLayouts[1].arrayStride = 3 * sizeof(float); // stride = size of color
    vertexBufferLayouts[1].stepMode = WGPUVertexStepMode_Vertex;
    
    pipelineDesc.vertex.bufferCount = static_cast<uint32_t>(vertexBufferLayouts.size());
    pipelineDesc.vertex.buffers = vertexBufferLayouts.data();
    pipelineDesc.vertex.module = shaderModule;
    pipelineDesc.vertex.entryPoint = toWgpuStringView("vs_main");
    WGPUFragmentState fragmentState = WGPU_FRAGMENT_STATE_INIT;
    fragmentState.module = shaderModule;
    fragmentState.entryPoint = toWgpuStringView("fs_main");
    WGPUColorTargetState colorTarget = WGPU_COLOR_TARGET_STATE_INIT;
    colorTarget.format = m_surfaceFormat;
    WGPUBlendState blendState = WGPU_BLEND_STATE_INIT;
    colorTarget.blend = &blendState;
    fragmentState.targetCount = 1;
    fragmentState.targets = &colorTarget;
    pipelineDesc.fragment = &fragmentState;
    m_pipeline = wgpuDeviceCreateRenderPipeline(m_device, &pipelineDesc);
    wgpuShaderModuleRelease(shaderModule);
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
	m_instance = wgpuCreateInstance(nullptr);
	
	// Get adapter
	std::cout << "Requesting adapter..." << std::endl;
	m_surface = glfwCreateWindowWGPUSurface(m_instance, m_window);
	
	WGPURequestAdapterOptions adapterOpts = WGPU_REQUEST_ADAPTER_OPTIONS_INIT;
	adapterOpts.compatibleSurface = m_surface;
	//                              ^^^^^^^^^ Use the surface here
	
	WGPUAdapter adapter = requestAdapterSync(m_instance, &adapterOpts);
	std::cout << "Got adapter: " << adapter << std::endl;
	
	std::cout << "Requesting device..." << std::endl;
	WGPUDeviceDescriptor deviceDesc = WGPU_DEVICE_DESCRIPTOR_INIT;
	// Any name works here, that's your call
	deviceDesc.label = toWgpuStringView("My Device");
	std::vector<WGPUFeatureName> features;
	// No required feature for now
	deviceDesc.requiredFeatureCount = features.size();
	deviceDesc.requiredFeatures = features.data();
	// Make sure 'features' lives until the call to wgpuAdapterRequestDevice!
	WGPULimits requiredLimits = WGPU_LIMITS_INIT;
	// We leave 'requiredLimits' untouched for now
	deviceDesc.requiredLimits = &requiredLimits;
	// Make sure that the 'requiredLimits' variable lives until the call to wgpuAdapterRequestDevice!
	deviceDesc.defaultQueue.label = toWgpuStringView("The Default Queue");
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
	deviceDesc.deviceLostCallbackInfo.callback = onDeviceLost;
	deviceDesc.deviceLostCallbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;
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
	deviceDesc.uncapturedErrorCallbackInfo.callback = onDeviceError;
	// NB: 'device' is now declared at the class level
	m_device = requestDeviceSync(m_instance, adapter, &deviceDesc);
	std::cout << "Got device: " << m_device << std::endl;
	
	m_queue = wgpuDeviceGetQueue(m_device);
	
	WGPUSurfaceConfiguration config = WGPU_SURFACE_CONFIGURATION_INIT;
	
	// Configuration of the textures created for the underlying swap chain
	config.width = 640;
	config.height = 480;
	config.device = m_device;
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
	config.presentMode = WGPUPresentMode_Fifo;
	config.alphaMode = WGPUCompositeAlphaMode_Auto;
	
	wgpuSurfaceConfigure(m_surface, &config);
	m_surfaceFormat = config.format;
	
	// We no longer need to access the adapter
	wgpuAdapterRelease(adapter);
	// At the end of Initialize()
	if (!InitializePipeline()) return false;
	// At the end of Initialize()
	if (!InitializeBuffers()) return false;
	return true;
}

void Application::Terminate() {
	// Move all the release/destroy/terminate calls here
	// At the beginning of Terminate()
	wgpuBufferRelease(m_positionBuffer);
	wgpuBufferRelease(m_colorBuffer);
	// At the beginning of Terminate()
	wgpuBufferRelease(m_vertexBuffer);
	wgpuRenderPipelineRelease(m_pipeline);
	wgpuSurfaceUnconfigure(m_surface);
	wgpuQueueRelease(m_queue);
	wgpuSurfaceRelease(m_surface);
	wgpuDeviceRelease(m_device);
	glfwDestroyWindow(m_window);
	glfwTerminate();
}

void Application::MainLoop() {
	glfwPollEvents();
	wgpuInstanceProcessEvents(m_instance);
	//                        ^^ We add this prefix to member variables

	// In Application::MainLoop()
	// Get the next target texture view
	WGPUTextureView targetView = GetNextSurfaceView();
	if (!targetView) return; // no surface texture, we skip this frame
	WGPUCommandEncoderDescriptor encoderDesc = WGPU_COMMAND_ENCODER_DESCRIPTOR_INIT;
	encoderDesc.label = toWgpuStringView("My command encoder");
	WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(m_device, &encoderDesc);
	WGPURenderPassDescriptor renderPassDesc = WGPU_RENDER_PASS_DESCRIPTOR_INIT;
	WGPURenderPassColorAttachment renderPassColorAttachment = WGPU_RENDER_PASS_COLOR_ATTACHMENT_INIT;
	
	renderPassColorAttachment.view = targetView;
	renderPassColorAttachment.loadOp = WGPULoadOp_Clear;
	renderPassColorAttachment.storeOp = WGPUStoreOp_Store;
	renderPassColorAttachment.clearValue = WGPUColor{ 0.25, 0.25, 0.25, 1.0 };
	
	renderPassDesc.colorAttachmentCount = 1;
	renderPassDesc.colorAttachments = &renderPassColorAttachment;
	
	WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);
	wgpuRenderPassEncoderSetPipeline(renderPass, m_pipeline);
	
	// Set vertex buffers while encoding the render pass
	wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, m_positionBuffer, 0, wgpuBufferGetSize(m_positionBuffer));
	wgpuRenderPassEncoderSetVertexBuffer(renderPass, 1, m_colorBuffer, 0, wgpuBufferGetSize(m_colorBuffer));
	//                                               ^ Add a second call to set the second vertex buffer
	
	// We use the `m_vertexCount` variable instead of hard-coding the vertex count
	wgpuRenderPassEncoderDraw(renderPass, m_vertexCount, 1, 0, 0);
	wgpuRenderPassEncoderEnd(renderPass);
	wgpuRenderPassEncoderRelease(renderPass);
	WGPUCommandBufferDescriptor cmdBufferDescriptor = WGPU_COMMAND_BUFFER_DESCRIPTOR_INIT;
	cmdBufferDescriptor.label = toWgpuStringView("Command buffer");
	WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &cmdBufferDescriptor);
	wgpuCommandEncoderRelease(encoder); // release encoder after it's finished
	
	// Finally submit the command queue
	std::cout << "Submitting command..." << std::endl;
	wgpuQueueSubmit(m_queue, 1, &command);
	wgpuCommandBufferRelease(command);
	std::cout << "Command submitted." << std::endl;
	// At the end of the frame
	wgpuTextureViewRelease(targetView);
	#ifndef __EMSCRIPTEN__
	wgpuSurfacePresent(m_surface);
	#endif
}

bool Application::IsRunning() {
	return !glfwWindowShouldClose(m_window);
}
WGPUTextureView Application::GetNextSurfaceView() {
    WGPUSurfaceTexture surfaceTexture = WGPU_SURFACE_TEXTURE_INIT;
    wgpuSurfaceGetCurrentTexture(m_surface, &surfaceTexture);
    if (
        surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal &&
        surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal
    ) {
        return nullptr;
    }
    WGPUTextureViewDescriptor viewDescriptor = WGPU_TEXTURE_VIEW_DESCRIPTOR_INIT;
    viewDescriptor.label = toWgpuStringView("Surface texture view");
    viewDescriptor.dimension = WGPUTextureViewDimension_2D; // not to confuse with 2DArray
    WGPUTextureView targetView = wgpuTextureCreateView(surfaceTexture.texture, &viewDescriptor);
    // We no longer need the texture, only its view,
    // so we release it at the end of GetNextSurfaceViewData
    wgpuTextureRelease(surfaceTexture.texture);
    return targetView;
}
bool Application::InitializeBuffers() {
	// x0, y0, x1, y1, ...
	std::vector<float> positionData = {
		-0.45f,  0.5f,
		 0.45f,  0.5f,
		  0.0f, -0.5f,
		 0.47f, 0.47f,
		 0.25f,  0.0f,
		 0.69f,  0.0f
	};
	
	// r0,  g0,  b0, r1,  g1,  b1, ...
	std::vector<float> colorData = {
		1.0, 1.0, 0.0, // (yellow)
		1.0, 0.0, 1.0, // (magenta)
		0.0, 1.0, 1.0, // (cyan)
		1.0, 0.0, 0.0, // (red)
		0.0, 1.0, 0.0, // (green)
		0.0, 0.0, 1.0  // (blue)
	};
	
	m_vertexCount = static_cast<uint32_t>(positionData.size() / 2);
	assert(m_vertexCount == static_cast<uint32_t>(colorData.size() / 3));
	// Create vertex buffers
	WGPUBufferDescriptor bufferDesc;
	bufferDesc.nextInChain = nullptr;
	bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex;
	bufferDesc.mappedAtCreation = false;
	
	bufferDesc.label = toWgpuStringView("Vertex Position");
	bufferDesc.size = positionData.size() * sizeof(float);
	m_positionBuffer = wgpuDeviceCreateBuffer(m_device, &bufferDesc);
	wgpuQueueWriteBuffer(m_queue, m_positionBuffer, 0, positionData.data(), bufferDesc.size);
	
	bufferDesc.label = toWgpuStringView("Vertex Color");
	bufferDesc.size = colorData.size() * sizeof(float);
	m_colorBuffer = wgpuDeviceCreateBuffer(m_device, &bufferDesc);
	wgpuQueueWriteBuffer(m_queue, m_colorBuffer, 0, colorData.data(), bufferDesc.size);
	// It is not easy with the auto-generation of code to remove the previously
	// defined `m_vertexBuffer` attribute, but at the same time some compilers
	// (rightfully) complain if we do not use it. This is a hack to mark the
	// variable as used and have automated build tests pass.
	(void)m_vertexBuffer;
	return true;
}