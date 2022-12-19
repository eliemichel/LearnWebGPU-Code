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

#include "glfw3webgpu.h"
#include "webgpu-utils.h"

#include <GLFW/glfw3.h>

#include <webgpu.h>
#include <wgpu.h> // wgpuTextureViewDrop

#include <iostream>
#include <vector>
#include <cassert>

#define UNUSED(x) (void)x;

int main (int, char**) {
	WGPUInstanceDescriptor desc = {};
	desc.nextInChain = nullptr;
	WGPUInstance instance = wgpuCreateInstance(&desc);
	if (!instance) {
		std::cerr << "Could not initialize WebGPU!" << std::endl;
		return 1;
	}

	if (!glfwInit()) {
		std::cerr << "Could not initialize GLFW!" << std::endl;
		return 1;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow* window = glfwCreateWindow(640, 480, "Learn WebGPU", NULL, NULL);
	if (!window) {
		std::cerr << "Could not open window!" << std::endl;
		return 1;
	}

	std::cout << "Requesting adapter..." << std::endl;
	WGPUSurface surface = glfwGetWGPUSurface(instance, window);
	WGPURequestAdapterOptions adapterOpts = {};
	adapterOpts.nextInChain = nullptr;
	adapterOpts.compatibleSurface = surface;
	WGPUAdapter adapter = requestAdapter(instance, &adapterOpts);
	std::cout << "Got adapter: " << adapter << std::endl;

	std::cout << "Requesting device..." << std::endl;
    WGPUDeviceDescriptor deviceDesc = {};
	deviceDesc.nextInChain = nullptr;
	deviceDesc.label = "My Device";
	deviceDesc.requiredFeaturesCount = 0;
	deviceDesc.requiredLimits = nullptr;
	deviceDesc.defaultQueue.nextInChain = nullptr;
	deviceDesc.defaultQueue.label = "The default queue";
	WGPUDevice device = requestDevice(adapter, &deviceDesc);
	std::cout << "Got device: " << device << std::endl;

	auto onDeviceError = [](WGPUErrorType type, char const* message, void* /* pUserData */) {
		std::cout << "Uncaptured device error: type " << type;
		if (message) std::cout << " (" << message << ")";
		std::cout << std::endl;
	};
	wgpuDeviceSetUncapturedErrorCallback(device, onDeviceError, nullptr /* pUserData */);

	std::cout << "Allocating GPU memory..." << std::endl;
	
	// Create a first buffer, which we use to upload data to the GPU
	WGPUBufferDescriptor bufferDesc = {};
	bufferDesc.nextInChain = nullptr;
	bufferDesc.label = "Input buffer: Written from the CPU to the GPU";
	bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_CopySrc;
	bufferDesc.size = 16;
	bufferDesc.mappedAtCreation = false;
	WGPUBuffer buffer1 = wgpuDeviceCreateBuffer(device, &bufferDesc);

	// Create a second buffer, with a `MapRead` usage flag so that we can map it later
	bufferDesc.nextInChain = nullptr;
	bufferDesc.label = "Output buffer: Read back from the GPU by the CPU";
	bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead;
	bufferDesc.size = 16;
	bufferDesc.mappedAtCreation = false;
	WGPUBuffer buffer2 = wgpuDeviceCreateBuffer(device, &bufferDesc);

	std::cout << "Configuring command queue..." << std::endl;

	// Get the command queue, through which we send commands to the GPU
	WGPUQueue queue = wgpuDeviceGetQueue(device);

	// Add a callback for debugging when commands in the queue have been executed
#if 0 // not implemented yet by the wgpu-native backend
	auto onQueueWorkDone = [](WGPUQueueWorkDoneStatus status, void* /* pUserData */) {
		std::cout << "Queued work finished with status: " << status << std::endl;
	};
	wgpuQueueOnSubmittedWorkDone(queue, onQueueWorkDone, nullptr /* pUserData */);
#endif

	std::cout << "Uploading data to the GPU..." << std::endl;

	// Create some CPU-side data buffer (of size 16 bytes)
	std::vector<unsigned char> numbers(16);
	for (unsigned char i = 0; i < 16; ++i) numbers[i] = i;

	// Copy this from `numbers` (RAM) to `buffer1` (VRAM)
	wgpuQueueWriteBuffer(queue, buffer1, 0, numbers.data(), numbers.size());

	std::cout << "Sending buffer copy operation..." << std::endl;

	// The only way to create a command buffer (which is needed to to anything
	// else than uploading buffer or texture data) is to use a command encoder.
	WGPUCommandEncoderDescriptor encoderDesc = {};
	encoderDesc.nextInChain = nullptr;
	encoderDesc.label = "Command encoder";
	WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &encoderDesc);

	// We add a command to the encoder:
	//   "Copy the current state of buffer 1 to buffer 2"
	wgpuCommandEncoderCopyBufferToBuffer(encoder, buffer1, 0, buffer2, 0, 16);

	// Finalize the encoding operation, synthesizing the command buffer
	WGPUCommandBufferDescriptor cmdBufferDescriptor = {};
	cmdBufferDescriptor.nextInChain = nullptr;
	cmdBufferDescriptor.label = "Command buffer";
	WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &cmdBufferDescriptor);
	
	// Submit the encoded command buffer
	wgpuQueueSubmit(queue, 1, &command);

	std::cout << "Start downloading result data from the GPU..." << std::endl;

	// The context shared between this main function and the callback.
	struct Context {
		WGPUBuffer buffer;
	};
	auto onBuffer2Mapped = [](WGPUBufferMapAsyncStatus status, void* pUserData) {
		Context* context = reinterpret_cast<Context*>(pUserData);
		std::cout << "Buffer 2 mapped with status " << status << std::endl;
		if (status != WGPUBufferMapAsyncStatus_Success) return;

		// Get a pointer to wherever the driver mapped the GPU memory to the RAM
		unsigned char* bufferData = (unsigned char*)wgpuBufferGetMappedRange(context->buffer, 0, 16);

		// Do stuff with bufferData
		std::cout << "bufferData = [";
		for (unsigned char i = 0; i < 16; ++i) {
			if (i > 0) std::cout << ", ";
			std::cout << (int)bufferData[i];
		}
		std::cout << "]" << std::endl;

		// Then do not forget to unmap the memory
		wgpuBufferUnmap(context->buffer);
	};

	Context context = { buffer2 };
	wgpuBufferMapAsync(buffer2, WGPUMapMode_Read, 0, 16, onBuffer2Mapped, (void*)&context);

	while (!glfwWindowShouldClose(window)) {
		// Do nothing, this checks for ongoing asynchronous operations and call their callbacks if needed
		// NB: Our wgpu-native backend provides a more explicit but non-standard wgpuDevicePoll(device) to do this.
		wgpuQueueSubmit(queue, 0, nullptr);

		// (This is the same idea, for the GLFW library callbacks)
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();

	// Free GPU memory
	wgpuBufferDestroy(buffer1);
	wgpuBufferDestroy(buffer2);

	return 0;
}
