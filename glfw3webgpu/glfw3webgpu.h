/**
 * This is an extension of GLFW for WebGPU, abstracting away the details of
 * OS-specific operations.
 * 
 * This file is part of the "Learn WebGPU for C++" book.
 *   https://eliemichel.github.io/LearnWebGPU
 * 
 * MIT License
 * Copyright (c) 2022-2024 Elie Michel and the wgpu-native authors
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

#ifndef _glfw3_webgpu_h_
#define _glfw3_webgpu_h_

#include <webgpu/webgpu.h>
#include <GLFW/glfw3.h>

#ifdef __cplusplus
extern "C" {
#endif

/*! @brief Creates a WebGPU surface for the specified window.
 *
 *  This function creates a WGPUSurface object for the specified window.
 *
 *  If the surface cannot be created, this function returns `NULL`.
 *
 *  It is the responsibility of the caller to destroy the window surface. The
 *  window surface must be destroyed using `wgpuSurfaceRelease`.
 *
 *  @param[in] instance The WebGPU instance to create the surface in.
 *  @param[in] window The window to create the surface for.
 *  @return The handle of the surface.  This is set to `NULL` if an error
 *  occurred.
 *
 *  @ingroup webgpu
 */
WGPUSurface glfwCreateWindowWGPUSurface(WGPUInstance instance, GLFWwindow* window);

#ifdef __cplusplus
}
#endif

#endif // _glfw3_webgpu_h_
