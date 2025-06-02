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
	// 1. We build a descriptor (called 'A' because we will have multiple buffers)
	WGPUBufferDescriptor bufferDescA = WGPU_BUFFER_DESCRIPTOR_INIT;
	bufferDescA.size = 256;
	// Buffer A is *written* on CPU, and used as *source* of a GPU-side copy
	bufferDescA.usage = WGPUBufferUsage_MapWrite | WGPUBufferUsage_CopySrc;
	bufferDescA.label = toWgpuStringView("Buffer A");
	// Map Buffer A at creation in order to upload its initial value
	bufferDescA.mappedAtCreation = true;
	
	// 2. We create the buffer from its descriptor
	WGPUBuffer bufferA = wgpuDeviceCreateBuffer(device, &bufferDescA);
	// We build a second buffer, called B
	WGPUBufferDescriptor bufferDescB = WGPU_BUFFER_DESCRIPTOR_INIT;
	// buffer B is shorter than buffer A in this example
	bufferDescB.size = 32;
	// Buffer B is *read* on CPU, and used as *destination* of a GPU-side copy
	bufferDescB.usage = WGPUBufferUsage_MapRead | WGPUBufferUsage_CopyDst;
	bufferDescB.label = toWgpuStringView("Buffer B");
	WGPUBuffer bufferB = wgpuDeviceCreateBuffer(device, &bufferDescB);
	// Get a pointer to the entire mapped buffer and interpret it as 8-bit unsigned integers
	uint8_t* bufferDataA = static_cast<uint8_t*>(
		wgpuBufferGetMappedRange(bufferA, 0, WGPU_WHOLE_MAP_SIZE)
	);
	// Write 0, 1, 2, 3, ... in bufferA
	for (size_t i = 0 ; i < 256 ; ++i) {
		bufferDataA[i] = static_cast<uint8_t>(i);
	}
	wgpuBufferUnmap(bufferA);
	// Do NOT use bufferDataA beyond this point!

	WGPUQueue queue = wgpuDeviceGetQueue(device);
	WGPUCommandEncoderDescriptor encoderDesc = WGPU_COMMAND_ENCODER_DESCRIPTOR_INIT;
	encoderDesc.label = toWgpuStringView("My command encoder");
	WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &encoderDesc);
	wgpuCommandEncoderCopyBufferToBuffer(
		encoder,
		bufferA,
		16, // sourceOffset
		bufferB,
		0, // destinationOffset
		bufferDescB.size
	);
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
	// In main()
	fetchBufferDataSync(instance, bufferB, [&](const void* data) {
		auto* bufferDataB = static_cast<const char*>(data);
		std::cout << "Buffer B: [";
		for (size_t i = 0 ; i < bufferDescB.size ; ++i) {
			if (i > 0) std::cout << ", ";
			std::cout << static_cast<int>(bufferDataB[i]); // cast to display as int rather than char
		}
		std::cout << "]" << std::endl;
	});

	// At the end of the program:
	wgpuBufferRelease(bufferA);
	wgpuBufferRelease(bufferB);
	// At the end
	wgpuQueueRelease(queue);
	wgpuDeviceRelease(device);
	// We clean up the WebGPU instance
	wgpuInstanceRelease(instance);

	return 0;
}