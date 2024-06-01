<img src="https://github.com/eliemichel/glfw3webgpu/actions/workflows/cmake.yml/badge.svg" alt="CMake Badge" />

<div align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="https://raw.githubusercontent.com/eliemichel/LearnWebGPU/main/images/webgpu-dark.svg">
    <source media="(prefers-color-scheme: light)" srcset="https://raw.githubusercontent.com/eliemichel/LearnWebGPU/main/images/webgpu-light.svg">
    <img alt="Learn WebGPU Logo" src="images/webgpu-dark.svg" width="200">
  </picture>

  <a href="https://github.com/eliemichel/LearnWebGPU">LearnWebGPU</a> &nbsp;|&nbsp; <a href="https://github.com/eliemichel/WebGPU-Cpp">WebGPU-C++</a> &nbsp;|&nbsp; <a href="https://github.com/eliemichel/WebGPU-distribution">WebGPU-distribution</a><br/>
  <a href="https://github.com/eliemichel/glfw3webgpu">glfw3webgpu</a> &nbsp;|&nbsp; <a href="https://github.com/eliemichel/sdl2webgpu">sdl2webgpu</a>
  
  <a href="https://discord.gg/2Tar4Kt564"><img src="https://img.shields.io/static/v1?label=Discord&message=Join%20us!&color=blue&logo=discord&logoColor=white" alt="Discord | Join us!"/></a>
</div>


GLFW WebGPU Extension
=====================

This is an extension for the great [GLFW](https://www.glfw.org/) library for using it with **WebGPU native**. It was written as part of the [Learn WebGPU for native C++](https://eliemichel.github.io/LearnWebGPU) tutorial series.

Table of Contents
-----------------

 - [Overview](#overview)
 - [Usage](#usage)
 - [Example](#example)
 - [License](#license)

Overview
--------

This extension simply provides the following function:

```C
WGPUSurface glfwGetWGPUSurface(WGPUInstance instance, GLFWwindow* window);
```

Given a GLFW window, `glfwGetWGPUSurface` returns a WebGPU *surface* that corresponds to the window's back-end. This is a process that is highly platform-specific, which is why I believe it belongs to GLFW.

Usage
-----

Your project must link to an implementation of WebGPU (providing `webgpu.h`) and of course to GLFW. Then:

**Option A** If you use CMake, you can simply include this project as a subdirectory with `add_subdirectory(glfw3webgpu)` (see the content of [`CMakeLists.txt`](CMakeLists.txt)).

**Option B** Just copy [`glfw3webgpu.h`](glfw3webgpu.h) and [`glfw3webgpu.c`](glfw3webgpu.c) to your project's source tree. On MacOS, you must add the compile option `-x objective-c` and the link libraries `-framework Cocoa`, `-framework CoreVideo`, `-framework IOKit`, and `-framework QuartzCore`.

Example
-------

Thanks to this extension it is possible to simply write a fully cross-platform WebGPU hello world:

```C
#include "glfw3webgpu.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <webgpu/webgpu.h>
#include <stdio.h>

int main(int argc, char* argv[]) {
	// Init WebGPU
	WGPUInstanceDescriptor desc;
	desc.nextInChain = NULL;
	WGPUInstance instance = wgpuCreateInstance(&desc);

	// Init GLFW
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow* window = glfwCreateWindow(640, 480, "Learn WebGPU", NULL, NULL);

	// Here we create our WebGPU surface from the window!
	WGPUSurface surface = glfwGetWGPUSurface(instance, window);
	printf("surface = %p", surface);

	// Terminate GLFW
	while (!glfwWindowShouldClose(window)) glfwPollEvents();
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
```

**NB** The linking process depends on the implementation of WebGPU that you are using. You can find detailed instructions for the `wgpu-native` implementation in [this Hello WebGPU chapter](https://eliemichel.github.io/LearnWebGPU/getting-started/hello-webgpu.html). You may also check out [`examples/CMakeLists.txt`](examples/CMakeLists.txt).

License
-------

```
MIT License
Copyright (c) 2022-2023 Élie Michel and the wgpu-native authors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```
