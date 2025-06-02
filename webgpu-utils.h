#pragma once

#include <webgpu/webgpu.h>

#include <string_view>

/**
 * Convert a WebGPU string view into a C++ std::string_view.
 */
std::string_view toStdStringView(WGPUStringView wgpuStringView);

/**
 * Convert a C++ std::string_view into a WebGPU string view.
 */
WGPUStringView toWgpuStringView(std::string_view stdStringView);

/**
 * Convert a C string into a WebGPU string view
 */
WGPUStringView toWgpuStringView(const char* cString);

/**
 * Sleep for a given number of milliseconds.
 * This works with both native builds and emscripten, provided that -sASYNCIFY
 * compile option is provided when building with emscripten.
 */
void sleepForMilliseconds(unsigned int milliseconds);

/**
 * Utility function to get a WebGPU adapter, so that
 *     WGPUAdapter adapter = requestAdapter(options);
 * is roughly equivalent to
 *     const adapter = await navigator.gpu.requestAdapter(options);
 */
WGPUAdapter requestAdapterSync(WGPUInstance instance, WGPURequestAdapterOptions const * options);

/**
 * Utility function to get a WebGPU device, so that
 *     WGPUAdapter device = requestDevice(adapter, options);
 * is roughly equivalent to
 *     const device = await adapter.requestDevice(descriptor);
 * It is very similar to requestAdapter
 */
WGPUDevice requestDeviceSync(WGPUInstance instance, WGPUAdapter adapter, WGPUDeviceDescriptor const * descriptor);

/**
 * An example of how we can inspect the capabilities of the hardware through
 * the adapter object.
 */
void inspectAdapter(WGPUAdapter adapter);
/**
 * Display information about a device
 */
void inspectDevice(WGPUDevice device);