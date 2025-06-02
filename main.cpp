#include "webgpu-utils.h"
#include <vector>
// Includes
#include <webgpu/webgpu.h>
#include <iostream>
#include <thread>
#include <chrono>
#ifdef __EMSCRIPTEN__
#  include <emscripten.h>
#endif
#include <string_view>


// Before main()
const char* shaderSource = R"(
// Bindings are declared for instance at the beginning of the shader
// Declaration of the inputBuffer resource,
// attached to binding #0 of bind group #0
@group(0) @binding(0)
var<storage,read> inputBuffer: array<f32>;

// Declaration of the outputBuffer resource,
// attached to binding #1 of bind group #0
@group(0) @binding(1)
var<storage,read_write> outputBuffer: array<f32>;

// Then we write our entry point
@compute @workgroup_size(32)
fn computeStuff(@builtin(global_invocation_id) threadId: vec3u) {
	// Core behavior of our shader: multiply input by 2 and store result
	// in output buffer:
    outputBuffer[threadId.x] = 2.0 * inputBuffer[threadId.x];
}
)";

int main() {
	// Create all WebGPU object we use throughout the program
	// We create a descriptor
	WGPUInstanceDescriptor desc = {};
	desc.nextInChain = nullptr;
	
	// We create the instance using this descriptor
	#ifdef WEBGPU_BACKEND_EMSCRIPTEN
	WGPUInstance instance = wgpuCreateInstance(nullptr);
	#else //  WEBGPU_BACKEND_EMSCRIPTEN
	WGPUInstance instance = wgpuCreateInstance(&desc);
	#endif //  WEBGPU_BACKEND_EMSCRIPTEN
	// We can check whether there is actually an instance created
	if (!instance) {
	    std::cerr << "Could not initialize WebGPU!" << std::endl;
	    return 1;
	}
	
	// Display the object (WGPUInstance is a simple pointer, it may be
	// copied around without worrying about its size).
	std::cout << "WGPU instance: " << instance << std::endl;
	std::cout << "Requesting adapter..." << std::endl;
	
	WGPURequestAdapterOptions adapterOpts = {};
	adapterOpts.nextInChain = nullptr;
	WGPUAdapter adapter = requestAdapterSync(instance, &adapterOpts);
	
	std::cout << "Got adapter: " << adapter << std::endl;
	inspectAdapter(adapter);
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
	WGPUDevice device = requestDeviceSync(instance, adapter, &deviceDesc);
	
	std::cout << "Got device: " << device << std::endl;
	// We no longer need to access the adapter once we have the device
	wgpuAdapterRelease(adapter);
	inspectDevice(device);
	// Before encoding commands:
	// Number of floats in the buffers
	size_t elementCount = 64;
	
	WGPUBufferDescriptor inputBufferDesc = WGPU_BUFFER_DESCRIPTOR_INIT;
	inputBufferDesc.label = toWgpuStringView("Input Buffer");
	inputBufferDesc.size = elementCount * sizeof(float);
	inputBufferDesc.usage = WGPUBufferUsage_Storage;
	inputBufferDesc.mappedAtCreation = true;
	WGPUBuffer inputBuffer = wgpuDeviceCreateBuffer(device, &inputBufferDesc);
	
	WGPUBufferDescriptor outputBufferDesc = WGPU_BUFFER_DESCRIPTOR_INIT;
	outputBufferDesc.label = toWgpuStringView("Output Buffer");
	outputBufferDesc.size = elementCount * sizeof(float);
	outputBufferDesc.usage = WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc;
	WGPUBuffer outputBuffer = wgpuDeviceCreateBuffer(device, &outputBufferDesc);
	
	WGPUBufferDescriptor stagingBufferDesc = WGPU_BUFFER_DESCRIPTOR_INIT;
	stagingBufferDesc.label = toWgpuStringView("Staging Buffer");
	stagingBufferDesc.size = elementCount * sizeof(float);
	stagingBufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead;
	WGPUBuffer stagingBuffer = wgpuDeviceCreateBuffer(device, &stagingBufferDesc);
	
	float* inputBufferData = static_cast<float*>(
		wgpuBufferGetMappedRange(inputBuffer, 0, WGPU_WHOLE_MAP_SIZE)
	);
	// Write 0.0, 0.1, 0.2, 0.3, ... in inputBuffer
	for (size_t i = 0 ; i < elementCount ; ++i) {
		inputBufferData[i] = static_cast<float>(i) * 0.1f;
	}
	wgpuBufferUnmap(inputBuffer);
	// At the beginning of our program:
	WGPUShaderModuleDescriptor moduleDesc = WGPU_SHADER_MODULE_DESCRIPTOR_INIT;
	moduleDesc.label = toWgpuStringView("Our first compute shader");
	// We create a chained descriptor dedicated to the WGSL source:
	WGPUShaderSourceWGSL wgslSourceDesc = WGPU_SHADER_SOURCE_WGSL_INIT;
	// The only payload of WGPUShaderSourceWGSL is the `code` field:
	wgslSourceDesc.code = toWgpuStringView(shaderSource);
	
	// And we connect it to the shader module descriptor through the nextInChain pointer:
	moduleDesc.nextInChain = &wgslSourceDesc.chain;
	
	WGPUShaderModule shaderModule = wgpuDeviceCreateShaderModule(device, &moduleDesc);
	WGPUComputePipelineDescriptor pipelineDesc = WGPU_COMPUTE_PIPELINE_DESCRIPTOR_INIT;
	// Description of our pipeline
	pipelineDesc.label = toWgpuStringView("Our simple pipeline");
	pipelineDesc.compute.module = shaderModule;
	pipelineDesc.compute.entryPoint = toWgpuStringView("computeStuff");
	
	WGPUComputePipeline pipeline = wgpuDeviceCreateComputePipeline(device, &pipelineDesc);
	
	// Once the compute pipeline is ready, we no longer need the shader module:
	wgpuShaderModuleRelease(shaderModule);
	std::vector<WGPUBindGroupEntry> bindGroupEntries(2, WGPU_BIND_GROUP_ENTRY_INIT);
	bindGroupEntries[0].binding = 0;
	bindGroupEntries[0].buffer = inputBuffer;
	bindGroupEntries[1].binding = 1;
	bindGroupEntries[1].buffer = outputBuffer;
	
	WGPUBindGroupDescriptor bindGroupDesc = WGPU_BIND_GROUP_DESCRIPTOR_INIT;
	bindGroupDesc.entries = bindGroupEntries.data();
	bindGroupDesc.entryCount = bindGroupEntries.size();
	bindGroupDesc.layout = wgpuComputePipelineGetBindGroupLayout(pipeline, 0);
	
	WGPUBindGroup bindGroup = wgpuDeviceCreateBindGroup(device, &bindGroupDesc);
	wgpuBindGroupLayoutRelease(bindGroupDesc.layout);

	WGPUQueue queue = wgpuDeviceGetQueue(device);
	WGPUCommandEncoderDescriptor encoderDesc = WGPU_COMMAND_ENCODER_DESCRIPTOR_INIT;
	encoderDesc.label = toWgpuStringView("My command encoder");
	WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &encoderDesc);
	WGPUComputePassEncoder computePass = wgpuCommandEncoderBeginComputePass(encoder, nullptr);
	wgpuComputePassEncoderSetPipeline(computePass, pipeline);
	wgpuComputePassEncoderSetBindGroup(computePass, 0, bindGroup, 0, nullptr);
	uint32_t workgroupSizeX = 32; // the value specified in @workgroup_size(...)
	uint32_t workgroupCountX = divideAndCeil((uint32_t)elementCount, workgroupSizeX);
	wgpuComputePassEncoderDispatchWorkgroups(computePass, workgroupCountX, 1, 1);
	wgpuComputePassEncoderEnd(computePass);
	wgpuComputePassEncoderRelease(computePass);
	// After the end of the compute pass, we copy the whole output buffer into the staging buffer
	wgpuCommandEncoderCopyBufferToBuffer(encoder, outputBuffer, 0, stagingBuffer, 0, stagingBufferDesc.size);
	WGPUCommandBufferDescriptor cmdBufferDescriptor = WGPU_COMMAND_BUFFER_DESCRIPTOR_INIT;
	cmdBufferDescriptor.label = toWgpuStringView("Command buffer");
	WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &cmdBufferDescriptor);
	wgpuCommandEncoderRelease(encoder); // release encoder after it's finished
	
	// Finally submit the command queue
	std::cout << "Submitting command..." << std::endl;
	wgpuQueueSubmit(queue, 1, &command);
	wgpuCommandBufferRelease(command);
	std::cout << "Command submitted." << std::endl;
	// Removed
	fetchBufferDataSync(instance, stagingBuffer, [&](const void* data) {
		const float* floatData = static_cast<const float*>(data);	
		std::cout << "Result: [";
		for (size_t i = 0 ; i < elementCount ; ++i) {
			if (i > 0) std::cout << ", ";
			std::cout << floatData[i];
		}
		std::cout << "]" << std::endl;
	});

	// At the end of our program, we release the pipeline.
	wgpuComputePipelineRelease(pipeline);
	wgpuBufferRelease(inputBuffer);
	wgpuBufferRelease(outputBuffer);
	wgpuBufferRelease(stagingBuffer);
	// At the end
	wgpuQueueRelease(queue);
	wgpuDeviceRelease(device);
	// We clean up the WebGPU instance
	wgpuInstanceRelease(instance);

	return 0;
}