/**
 * This file is part of the "Learn WebGPU for C++" book.
 *   https://eliemichel.github.io/LearnWebGPU
 * 
 * MIT License
 * Copyright (c) 2022-2023 Elie Michel
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

#include "webgpu-utils.h"

#include <glfw3webgpu.h>
#include <GLFW/glfw3.h>

#include <webgpu/webgpu.h>

#include <iostream>
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
	WGPURequestAdapterOptions adapterOpts = {};
	adapterOpts.nextInChain = nullptr;
	adapterOpts.compatibleSurface = glfwGetWGPUSurface(instance, window);
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

	inspectDevice(device);

	// Get the main and only command queue used to send instructions to the GPU
	WGPUQueue queue = wgpuDeviceGetQueue(device);

#ifdef WEBGPU_BACKEND_DAWN
	// Add a callback to monitor the moment queued work winished
	auto onQueueWorkDone = [](WGPUQueueWorkDoneStatus status, void* /* pUserData */) {
	    std::cout << "Queued work finished with status: " << status << std::endl;
	};
	wgpuQueueOnSubmittedWorkDone(queue, 0 /* non standard argument for Dawn */, onQueueWorkDone, nullptr /* pUserData */);
#endif // WEBGPU_BACKEND_DAWN

	// We create a command encoder to be able to create command buffers
	WGPUCommandEncoderDescriptor encoderDesc = {};
	encoderDesc.nextInChain = nullptr;
	encoderDesc.label = "My command encoder";
	WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &encoderDesc);

	// Encode some mock commands
	wgpuCommandEncoderInsertDebugMarker(encoder, "Do one thing");
	wgpuCommandEncoderInsertDebugMarker(encoder, "Do another thing");

	// Encode commands into a command buffer
	WGPUCommandBufferDescriptor cmdBufferDescriptor = {};
	cmdBufferDescriptor.nextInChain = nullptr;
	cmdBufferDescriptor.label = "Command buffer";
	WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &cmdBufferDescriptor);
	wgpuCommandEncoderRelease(encoder); // release encoder after it's finished

	// Finally submit the command queue
	std::cout << "Submitting command..." << std::endl;
	wgpuQueueSubmit(queue, 1, &command);
	wgpuCommandBufferRelease(command); // release command buffer once submitted

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
	}

	wgpuQueueRelease(queue);
	wgpuDeviceRelease(device);
	wgpuAdapterRelease(adapter);
	wgpuInstanceRelease(instance);
	wgpuSurfaceRelease(surface);
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}

