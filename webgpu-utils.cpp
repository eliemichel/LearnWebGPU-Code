// {Begin block 'file: webgpu-utils.cpp' (in root '019 - Our first shader - Next')}
#include "webgpu-utils.h"

#include <iostream>
#include <vector>
#include <cassert>

#ifdef __EMSCRIPTEN__
#  include <emscripten.h>
#else // __EMSCRIPTEN__
#  include <thread>
#  include <chrono>
#endif // __EMSCRIPTEN__

// {Begin block 'Utility functions' (in root '044 - Dynamic uniforms - Next')}
// {Begin block 'Define toStdStringView' (in root '005 - The Adapter - Next')}
std::string_view toStdStringView(WGPUStringView wgpuStringView) {
	return
		wgpuStringView.data == nullptr
		? std::string_view()
		: wgpuStringView.length == WGPU_STRLEN
		? std::string_view(wgpuStringView.data)
		: std::string_view(wgpuStringView.data, wgpuStringView.length);
}
// {End block 'Define toStdStringView' (in root '005 - The Adapter - Next')}
// {Begin block 'Define toWgpuStringView' (in root '005 - The Adapter - Next')}
WGPUStringView toWgpuStringView(std::string_view stdStringView) {
	return { stdStringView.data(), stdStringView.size() };
}
WGPUStringView toWgpuStringView(const char* cString) {
	return { cString, WGPU_STRLEN };
}
// {End block 'Define toWgpuStringView' (in root '005 - The Adapter - Next')}
// {Begin block 'Define sleepForMilliseconds' (in root '005 - The Adapter - Next')}
void sleepForMilliseconds(unsigned int milliseconds) {
#ifdef __EMSCRIPTEN__
	emscripten_sleep(milliseconds);
#else
	std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
#endif
}
// {End block 'Define sleepForMilliseconds' (in root '005 - The Adapter - Next')}
// All utility functions are regrouped here
// {Begin block 'Request adapter function' (in root '005 - The Adapter - Next')}
/**
 * Utility function to get a WebGPU adapter, so that
 *     WGPUAdapter adapter = requestAdapterSync(options);
 * is roughly equivalent to the JavaScript
 *     const adapter = await navigator.gpu.requestAdapter(options);
 */
WGPUAdapter requestAdapterSync(WGPUInstance instance, WGPURequestAdapterOptions const * options) {
	// A simple structure holding the local information shared with the
	// onAdapterRequestEnded callback.
	struct UserData {
		WGPUAdapter adapter = nullptr;
		bool requestEnded = false;
	};
	UserData userData;

	// Callback called by wgpuInstanceRequestAdapter when the request returns
	// This is a C++ lambda function, but could be any function defined in the
	// global scope. It must be non-capturing (the brackets [] are empty) so
	// that it behaves like a regular C function pointer, which is what
	// wgpuInstanceRequestAdapter expects (WebGPU being a C API). The workaround
	// is to convey what we want to capture through the userdata1 pointer,
	// provided as the last argument of wgpuInstanceRequestAdapter and received
	// by the callback as its last argument.
	auto onAdapterRequestEnded = [](
		WGPURequestAdapterStatus status,
		WGPUAdapter adapter,
		WGPUStringView message,
		void* userdata1,
		void* /* userdata2 */
	) {
		UserData& userData = *reinterpret_cast<UserData*>(userdata1);
		if (status == WGPURequestAdapterStatus_Success) {
			userData.adapter = adapter;
		} else {
			// {Begin block 'Display error message' (in root '005 - The Adapter - Next')}
			std::cerr << "Error while requesting adapter: " << toStdStringView(message) << std::endl;
			// {End block 'Display error message' (in root '005 - The Adapter - Next')}
		}
		userData.requestEnded = true;
	};

	// Build the callback info
	WGPURequestAdapterCallbackInfo callbackInfo = {
		/* nextInChain = */ nullptr,
		/* mode = */ WGPUCallbackMode_AllowProcessEvents,
		/* callback = */ onAdapterRequestEnded,
		/* userdata1 = */ &userData,
		/* userdata2 = */ nullptr
	};

	// Call to the WebGPU request adapter procedure
	wgpuInstanceRequestAdapter(instance, options, callbackInfo);

	// We wait until userData.requestEnded gets true

	// Hand the execution to the WebGPU instance so that it can check for
	// pending async operations, in which case it invokes our callbacks.
	// NB: We test once before the loop not to wait for 200ms in case it is
	// already ready
	wgpuInstanceProcessEvents(instance);

	while (!userData.requestEnded) {
		// Waiting for 200 ms to avoid asking too often to process events
		sleepForMilliseconds(200);

		wgpuInstanceProcessEvents(instance);
	}

	return userData.adapter;
}
// {End block 'Request adapter function' (in root '005 - The Adapter - Next')}
void inspectAdapter(WGPUAdapter adapter) {
	// {Begin block 'Inspect adapter' (in root '005 - The Adapter - Next')}
	WGPULimits supportedLimits = {};
	supportedLimits.nextInChain = nullptr;
	
	bool success = wgpuAdapterGetLimits(adapter, &supportedLimits) == WGPUStatus_Success;
	
	if (success) {
		std::cout << "Adapter limits:" << std::endl;
		std::cout << " - maxTextureDimension1D: " << supportedLimits.maxTextureDimension1D << std::endl;
		std::cout << " - maxTextureDimension2D: " << supportedLimits.maxTextureDimension2D << std::endl;
		std::cout << " - maxTextureDimension3D: " << supportedLimits.maxTextureDimension3D << std::endl;
		std::cout << " - maxTextureArrayLayers: " << supportedLimits.maxTextureArrayLayers << std::endl;
	}
	// Prepare the struct where features will be listed
	WGPUSupportedFeatures features;
	
	// Get adapter features. This may allocate memory that we must later free with wgpuSupportedFeaturesFreeMembers()
	wgpuAdapterGetFeatures(adapter, &features);
	
	std::cout << "Adapter features:" << std::endl;
	std::cout << std::hex; // Write integers as hexadecimal to ease comparison with webgpu.h literals
	for (size_t i = 0; i < features.featureCount; ++i) {
		std::cout << " - 0x" << features.features[i] << std::endl;
	}
	std::cout << std::dec; // Restore decimal numbers
	
	// Free the memory that had potentially been allocated by wgpuAdapterGetFeatures()
	wgpuSupportedFeaturesFreeMembers(features);
	// One shall no longer use features beyond this line.
	WGPUAdapterInfo properties;
	properties.nextInChain = nullptr;
	wgpuAdapterGetInfo(adapter, &properties);
	std::cout << "Adapter properties:" << std::endl;
	std::cout << " - vendorID: " << properties.vendorID << std::endl;
	std::cout << " - vendorName: " << toStdStringView(properties.vendor) << std::endl;
	std::cout << " - architecture: " << toStdStringView(properties.architecture) << std::endl;
	std::cout << " - deviceID: " << properties.deviceID << std::endl;
	std::cout << " - name: " << toStdStringView(properties.device) << std::endl;
	std::cout << " - driverDescription: " << toStdStringView(properties.description) << std::endl;
	std::cout << std::hex;
	std::cout << " - adapterType: 0x" << properties.adapterType << std::endl;
	std::cout << " - backendType: 0x" << properties.backendType << std::endl;
	std::cout << std::dec; // Restore decimal numbers
	wgpuAdapterInfoFreeMembers(properties);
	// {End block 'Inspect adapter' (in root '005 - The Adapter - Next')}
}
// {Begin block 'Request device function' (in root '010 - The Device - Next')}
/**
 * Utility function to get a WebGPU device, so that
 *     WGPUDevice device = requestDeviceSync(adapter, options);
 * is roughly equivalent to
 *     const device = await adapter.requestDevice(descriptor);
 * It is very similar to requestAdapter
 */
WGPUDevice requestDeviceSync(WGPUInstance instance, WGPUAdapter adapter, WGPUDeviceDescriptor const * descriptor) {
	struct UserData {
		WGPUDevice device = nullptr;
		bool requestEnded = false;
	};
	UserData userData;

	// The callback
	auto onDeviceRequestEnded = [](
		WGPURequestDeviceStatus status,
		WGPUDevice device,
		WGPUStringView message,
		void* userdata1,
		void* /* userdata2 */
	) {
		UserData& userData = *reinterpret_cast<UserData*>(userdata1);
		if (status == WGPURequestDeviceStatus_Success) {
			userData.device = device;
		} else {
			std::cerr << "Error while requesting device: " << toStdStringView(message) << std::endl;
		}
		userData.requestEnded = true;
	};

	// Build the callback info
	WGPURequestDeviceCallbackInfo callbackInfo = {
		/* nextInChain = */ nullptr,
		/* mode = */ WGPUCallbackMode_AllowProcessEvents,
		/* callback = */ onDeviceRequestEnded,
		/* userdata1 = */ &userData,
		/* userdata2 = */ nullptr
	};

	// Call to the WebGPU request adapter procedure
	wgpuAdapterRequestDevice(adapter, descriptor, callbackInfo);

	// Hand the execution to the WebGPU instance until the request ended
	wgpuInstanceProcessEvents(instance);
	while (!userData.requestEnded) {
		sleepForMilliseconds(200);
		wgpuInstanceProcessEvents(instance);
	}

	return userData.device;
}
// {End block 'Request device function' (in root '010 - The Device - Next')}
// We create a utility function to inspect the device:
void inspectDevice(WGPUDevice device) {
	
	WGPUSupportedFeatures features = WGPU_SUPPORTED_FEATURES_INIT;
	wgpuDeviceGetFeatures(device, &features);
	std::cout << "Device features:" << std::endl;
	std::cout << std::hex;
	for (size_t i = 0; i < features.featureCount; ++i) {
		std::cout << " - 0x" << features.features[i] << std::endl;
	}
	std::cout << std::dec;
	wgpuSupportedFeaturesFreeMembers(features);

	WGPULimits limits = WGPU_LIMITS_INIT;
	bool success = wgpuDeviceGetLimits(device, &limits) == WGPUStatus_Success;

	if (success) {
		std::cout << "Device limits:" << std::endl;
		std::cout << " - maxTextureDimension1D: " << limits.maxTextureDimension1D << std::endl;
		std::cout << " - maxTextureDimension2D: " << limits.maxTextureDimension2D << std::endl;
		std::cout << " - maxTextureDimension3D: " << limits.maxTextureDimension3D << std::endl;
		std::cout << " - maxTextureArrayLayers: " << limits.maxTextureArrayLayers << std::endl;
		// {Begin block 'Extra device limits' (in root '010 - The Device - Next')}
		std::cout << " - maxBindGroups: " << limits.maxBindGroups << std::endl;
		std::cout << " - maxBindGroupsPlusVertexBuffers: " << limits.maxBindGroupsPlusVertexBuffers << std::endl;
		std::cout << " - maxBindingsPerBindGroup: " << limits.maxBindingsPerBindGroup << std::endl;
		std::cout << " - maxDynamicUniformBuffersPerPipelineLayout: " << limits.maxDynamicUniformBuffersPerPipelineLayout << std::endl;
		std::cout << " - maxDynamicStorageBuffersPerPipelineLayout: " << limits.maxDynamicStorageBuffersPerPipelineLayout << std::endl;
		std::cout << " - maxSampledTexturesPerShaderStage: " << limits.maxSampledTexturesPerShaderStage << std::endl;
		std::cout << " - maxSamplersPerShaderStage: " << limits.maxSamplersPerShaderStage << std::endl;
		std::cout << " - maxStorageBuffersPerShaderStage: " << limits.maxStorageBuffersPerShaderStage << std::endl;
		std::cout << " - maxStorageTexturesPerShaderStage: " << limits.maxStorageTexturesPerShaderStage << std::endl;
		std::cout << " - maxUniformBuffersPerShaderStage: " << limits.maxUniformBuffersPerShaderStage << std::endl;
		std::cout << " - maxUniformBufferBindingSize: " << limits.maxUniformBufferBindingSize << std::endl;
		std::cout << " - maxStorageBufferBindingSize: " << limits.maxStorageBufferBindingSize << std::endl;
		std::cout << " - minUniformBufferOffsetAlignment: " << limits.minUniformBufferOffsetAlignment << std::endl;
		std::cout << " - minStorageBufferOffsetAlignment: " << limits.minStorageBufferOffsetAlignment << std::endl;
		std::cout << " - maxVertexBuffers: " << limits.maxVertexBuffers << std::endl;
		std::cout << " - maxBufferSize: " << limits.maxBufferSize << std::endl;
		std::cout << " - maxVertexAttributes: " << limits.maxVertexAttributes << std::endl;
		std::cout << " - maxVertexBufferArrayStride: " << limits.maxVertexBufferArrayStride << std::endl;
		std::cout << " - maxInterStageShaderVariables: " << limits.maxInterStageShaderVariables << std::endl;
		std::cout << " - maxColorAttachments: " << limits.maxColorAttachments << std::endl;
		std::cout << " - maxColorAttachmentBytesPerSample: " << limits.maxColorAttachmentBytesPerSample << std::endl;
		std::cout << " - maxComputeWorkgroupStorageSize: " << limits.maxComputeWorkgroupStorageSize << std::endl;
		std::cout << " - maxComputeInvocationsPerWorkgroup: " << limits.maxComputeInvocationsPerWorkgroup << std::endl;
		std::cout << " - maxComputeWorkgroupSizeX: " << limits.maxComputeWorkgroupSizeX << std::endl;
		std::cout << " - maxComputeWorkgroupSizeY: " << limits.maxComputeWorkgroupSizeY << std::endl;
		std::cout << " - maxComputeWorkgroupSizeZ: " << limits.maxComputeWorkgroupSizeZ << std::endl;
		std::cout << " - maxComputeWorkgroupsPerDimension: " << limits.maxComputeWorkgroupsPerDimension << std::endl;
		std::cout << " - maxStorageBuffersInVertexStage: " << limits.maxStorageBuffersInVertexStage << std::endl;
		std::cout << " - maxStorageTexturesInVertexStage: " << limits.maxStorageTexturesInVertexStage << std::endl;
		std::cout << " - maxStorageBuffersInFragmentStage: " << limits.maxStorageBuffersInFragmentStage << std::endl;
		std::cout << " - maxStorageTexturesInFragmentStage: " << limits.maxStorageTexturesInFragmentStage << std::endl;
		// {End block 'Extra device limits' (in root '010 - The Device - Next')}
	}
}
uint32_t ceilToNextMultiple(uint32_t value, uint32_t step) {
	uint32_t divide_and_ceil = value / step + (value % step == 0 ? 0 : 1);
	return step * divide_and_ceil;
}
// {End block 'Utility functions' (in root '044 - Dynamic uniforms - Next')}
void fetchBufferDataSync(
	WGPUInstance instance,
	WGPUBuffer bufferB,
	std::function<void(const void*)> processBufferData
) {
	// We copy here what used to be in main():
	// {Begin block 'Map and display buffer B' (in root '017 - Playing with buffers - Next')}
	// {Begin block 'Map buffer B' (in root '017 - Playing with buffers - Next')}
	// {Begin block 'Define callback handling the mapped buffer B' (in root '017 - Playing with buffers - Next')}
	// Context passed to `onBufferBMapped` through theuserdata pointer:
	struct OnBufferBMappedContext {
		bool operationEnded = false; // Turned true as soon as the callback is invoked
		bool mappingIsSuccessful = false; // Turned true only if mapping succeeded
	};
	
	// This function has the type WGPUBufferMapCallback as defined in webgpu.h
	auto onBufferBMapped = [](
		WGPUMapAsyncStatus status,
		struct WGPUStringView message,
		void* userdata1,
		void* /* userdata2 */
	) {
		OnBufferBMappedContext& context = *reinterpret_cast<OnBufferBMappedContext*>(userdata1);
		context.operationEnded = true;
		if (status == WGPUMapAsyncStatus_Success) {
			context.mappingIsSuccessful = true;
		} else {
			std::cout << "Could not map buffer B! Status: " << status << ", message: " << toStdStringView(message) << std::endl;
		}
	};
	// {End block 'Define callback handling the mapped buffer B' (in root '017 - Playing with buffers - Next')}
	
	// {Begin block 'Build callback info for mapping buffer B' (in root '017 - Playing with buffers - Next')}
	// We create an instance of the context shared with `onBufferBMapped`
	OnBufferBMappedContext context;
	
	// And we build the callback info:
	WGPUBufferMapCallbackInfo callbackInfo = WGPU_BUFFER_MAP_CALLBACK_INFO_INIT;
	callbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;
	callbackInfo.callback = onBufferBMapped;
	callbackInfo.userdata1 = &context;
	// {End block 'Build callback info for mapping buffer B' (in root '017 - Playing with buffers - Next')}
	
	// And finally we launch the asynchronous operation
	wgpuBufferMapAsync(
		bufferB,
		WGPUMapMode_Read,
		0, // offset
		WGPU_WHOLE_MAP_SIZE,
		callbackInfo
	);
	// {End block 'Map buffer B' (in root '017 - Playing with buffers - Next')}
	
	// {Begin block 'Wait for mapping of buffer B' (in root '017 - Playing with buffers - Next')}
	// Process events until the map operation ended
	wgpuInstanceProcessEvents(instance);
	while (!context.operationEnded) {
		sleepForMilliseconds(200);
		wgpuInstanceProcessEvents(instance);
	}
	// {End block 'Wait for mapping of buffer B' (in root '017 - Playing with buffers - Next')}
	
	if (context.mappingIsSuccessful) {
		// {Begin block 'Display buffer B' (in root '017 - Playing with buffers - Next')}
		const void* bufferData = wgpuBufferGetConstMappedRange(bufferB, 0, WGPU_WHOLE_MAP_SIZE);
		processBufferData(bufferData);
		// {End block 'Display buffer B' (in root '017 - Playing with buffers - Next')}
	}
	// {End block 'Map and display buffer B' (in root '017 - Playing with buffers - Next')}
}
uint32_t divideAndCeil(uint32_t p, uint32_t q) {
	return (p + q - 1) / q;
}
// {End block 'file: webgpu-utils.cpp' (in root '019 - Our first shader - Next')}