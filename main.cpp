// Includes
#include <webgpu/webgpu.h>
#include <iostream>
#include <thread>
#include <chrono>
#ifdef __EMSCRIPTEN__
#  include <emscripten.h>
#endif
#include <string_view>

std::string_view toStdStringView(WGPUStringView wgpuStringView) {
	return
		wgpuStringView.data == nullptr
		? std::string_view()
		: wgpuStringView.length == WGPU_STRLEN
		? std::string_view(wgpuStringView.data)
		: std::string_view(wgpuStringView.data, wgpuStringView.length);
}
WGPUStringView toWgpuStringView(std::string_view stdStringView) {
	return { stdStringView.data(), stdStringView.size() };
}
WGPUStringView toWgpuStringView(const char* cString) {
	return { cString, WGPU_STRLEN };
}
void sleepForMilliseconds(unsigned int milliseconds) {
#ifdef __EMSCRIPTEN__
	emscripten_sleep(milliseconds);
#else
	std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
#endif
}
// All utility functions are regrouped here
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
			std::cerr << "Error while requesting adapter: " << toStdStringView(message) << std::endl;
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
void inspectAdapter(WGPUAdapter adapter) {
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
}

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


	wgpuAdapterRelease(adapter);
	// We clean up the WebGPU instance
	wgpuInstanceRelease(instance);

	return 0;
}