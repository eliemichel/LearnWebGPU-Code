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

	WGPUQueue queue = wgpuDeviceGetQueue(device);
	WGPUCommandEncoderDescriptor encoderDesc = WGPU_COMMAND_ENCODER_DESCRIPTOR_INIT;
	encoderDesc.label = toWgpuStringView("My command encoder");
	WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &encoderDesc);
	wgpuCommandEncoderInsertDebugMarker(encoder, toWgpuStringView("Do one thing"));
	wgpuCommandEncoderInsertDebugMarker(encoder, toWgpuStringView("Do another thing"));
	WGPUCommandBufferDescriptor cmdBufferDescriptor = WGPU_COMMAND_BUFFER_DESCRIPTOR_INIT;
	cmdBufferDescriptor.label = toWgpuStringView("Command buffer");
	WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &cmdBufferDescriptor);
	wgpuCommandEncoderRelease(encoder); // release encoder after it's finished
	
	// Finally submit the command queue
	std::cout << "Submitting command..." << std::endl;
	wgpuQueueSubmit(queue, 1, &command);
	wgpuCommandBufferRelease(command);
	std::cout << "Command submitted." << std::endl;
	// Our callback invoked when GPU instructions have been executed
	auto onQueuedWorkDone = [](
		WGPUQueueWorkDoneStatus status,
		void* userdata1,
		void* /* userdata2 */
	) {
		// Display a warning when status is not success
		if (status != WGPUQueueWorkDoneStatus_Success) {
			std::cout << "Warning: wgpuQueueOnSubmittedWorkDone failed, this is suspicious!" << std::endl;
		}
	
		// Interpret userdata1 as a pointer to a boolean (and turn it into a
		// mutable reference), then turn it to 'true'
		bool& workDone = *reinterpret_cast<bool*>(userdata1);
		workDone = true;
	};
	
	// Create the boolean that will be passed to the callback as userdata1
	// and initialize it to 'false'
	bool workDone = false;
	
	// Create the callback info
	WGPUQueueWorkDoneCallbackInfo callbackInfo = WGPU_QUEUE_WORK_DONE_CALLBACK_INFO_INIT;
	callbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;
	callbackInfo.callback = onQueuedWorkDone;
	callbackInfo.userdata1 = &workDone; // pass the address of workDone
	
	// Add the async operation to the queue
	wgpuQueueOnSubmittedWorkDone(queue, callbackInfo);
	
	// Hand the execution to the WebGPU instance until onQueuedWorkDone gets invoked
	wgpuInstanceProcessEvents(instance);
	while (!workDone) {
		sleepForMilliseconds(200);
		wgpuInstanceProcessEvents(instance);
	}
	
	std::cout << "All queued instructions have been executed!" << std::endl;

	// At the end
	wgpuQueueRelease(queue);
	wgpuDeviceRelease(device);
	// We clean up the WebGPU instance
	wgpuInstanceRelease(instance);

	return 0;
}