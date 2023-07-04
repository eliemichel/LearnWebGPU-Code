/**
 * This file is part of the "Learn WebGPU for C++" book.
 *   https://github.com/eliemichel/LearnWebGPU
 * 
 * MIT License
 * Copyright (c) 2022 Elie Michel
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#define WEBGPU_CPP_IMPLEMENTATION
#include <webgpu/webgpu.hpp>

#include <glfw3webgpu.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
#include <cassert>

using namespace wgpu;

int main (int, char**) {
	Instance instance = createInstance(InstanceDescriptor{});
	if (!instance) {
		std::cerr << "Could not initialize WebGPU!" << std::endl;
		return 1;
	}

	if (!glfwInit()) {
		std::cerr << "Could not initialize GLFW!" << std::endl;
		return 1;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	GLFWwindow* window = glfwCreateWindow(640, 480, "Learn WebGPU", NULL, NULL);
	if (!window) {
		std::cerr << "Could not open window!" << std::endl;
		return 1;
	}

	std::cout << "Requesting adapter..." << std::endl;
	Surface surface = glfwGetWGPUSurface(instance, window);
	RequestAdapterOptions adapterOpts;
	adapterOpts.compatibleSurface = surface;
	Adapter adapter = instance.requestAdapter(adapterOpts);
	std::cout << "Got adapter: " << adapter << std::endl;

	std::cout << "Requesting device..." << std::endl;
	DeviceDescriptor deviceDesc;
	deviceDesc.label = "My Device";
	deviceDesc.requiredFeaturesCount = 0;
	deviceDesc.requiredLimits = nullptr;
	deviceDesc.defaultQueue.label = "The default queue";
	Device device = adapter.requestDevice(deviceDesc);
	std::cout << "Got device: " << device << std::endl;

	// Add an error callback for more debug info
	auto h = device.setUncapturedErrorCallback([](ErrorType type, char const* message) {
		std::cout << "Device error: type " << type;
		if (message) std::cout << " (message: " << message << ")";
		std::cout << std::endl;
	});

	std::cout << "Allocating GPU memory..." << std::endl;
	
	// Create a first buffer, which we use to upload data to the GPU
	BufferDescriptor bufferDesc;
	bufferDesc.label = "Input buffer: Written from the CPU to the GPU";
	bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::CopySrc;
	bufferDesc.size = 16;
	bufferDesc.mappedAtCreation = false;
	Buffer buffer1 = device.createBuffer(bufferDesc);

	// Create a second buffer, with a `MapRead` usage flag so that we can map it later
	bufferDesc.label = "Output buffer: Read back from the GPU by the CPU";
	bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::MapRead;
	bufferDesc.size = 16;
	bufferDesc.mappedAtCreation = false;
	Buffer buffer2 = device.createBuffer(bufferDesc);

	// Get the command queue, through which we send commands to the GPU
	Queue queue = device.getQueue();

	// Add a callback for debugging when commands in the queue have been executed
#if 0 // not implemented yet by backends
	auto onQueueWorkDone = [](WGPUQueueWorkDoneStatus status, void* /* pUserData */) {
		std::cout << "Queued work finished with status: " << status << std::endl;
	};
	wgpuQueueOnSubmittedWorkDone(queue, onQueueWorkDone, nullptr /* pUserData */);
#endif

	std::cout << "Uploading data to the GPU..." << std::endl;

	// Create some CPU-side data buffer (of size 16 bytes)
	std::vector<uint8_t> numbers(16);
	for (uint8_t i = 0; i < 16; ++i) numbers[i] = i;

	// Copy this from `numbers` (RAM) to `buffer1` (VRAM)
	queue.writeBuffer(buffer1, 0, numbers.data(), numbers.size());

	std::cout << "Sending buffer copy operation..." << std::endl;

	// The only way to create a command buffer (which is needed to to anything
	// else than uploading buffer or texture data) is to use a command encoder.
	CommandEncoder encoder = device.createCommandEncoder(CommandEncoderDescriptor{});

	// We add a command to the encoder:
	//   "Copy the current state of buffer 1 to buffer 2"
	encoder.copyBufferToBuffer(buffer1, 0, buffer2, 0, 16);

	// Finalize the encoding operation, synthesizing the command buffer
	CommandBuffer command = encoder.finish(CommandBufferDescriptor{});
	
	// Submit the encoded command buffer
	queue.submit(1, &command);

	std::cout << "Start downloading result data from the GPU..." << std::endl;

	// The context shared between this main function and the callback.
	struct Context {
		Buffer buffer;
	};
	auto onBuffer2Mapped = [](WGPUBufferMapAsyncStatus status, void* pUserData) {
		Context* context = reinterpret_cast<Context*>(pUserData);
		std::cout << "Buffer 2 mapped with status " << status << std::endl;
		if (status != BufferMapAsyncStatus::Success) return;

		// Get a pointer to wherever the driver mapped the GPU memory to the RAM
		uint8_t* bufferData = (uint8_t*)context->buffer.getConstMappedRange(0, 16);

		// Do stuff with bufferData
		std::cout << "bufferData = [";
		for (int i = 0; i < 16; ++i) {
			if (i > 0) std::cout << ", ";
			std::cout << (int)bufferData[i];
		}
		std::cout << "]" << std::endl;

		// Then do not forget to unmap the memory
		context->buffer.unmap();
	};

	Context context = { buffer2 };
	wgpuBufferMapAsync(buffer2, MapMode::Read, 0, 16, onBuffer2Mapped, (void*)&context);

	while (!glfwWindowShouldClose(window)) {
		// Do nothing, this checks for ongoing asynchronous operations and call their callbacks if needed
#ifdef WEBGPU_BACKEND_WGPU
		// Non-standardized behavior: submit empty queue to flush callbacks
		// (wgpu-native also has a device.poll but its API is more complex)
		queue.submit(0, nullptr);
#else
		// Non-standard Dawn way
		device.tick();
#endif

		// (This is the same idea, for the GLFW library callbacks)
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();

	// Free GPU memory
	wgpuSurfaceRelease(surface);
	buffer1.destroy();
	buffer2.destroy();
	wgpuBufferRelease(buffer1);
	wgpuBufferRelease(buffer2);

	return 0;
}
