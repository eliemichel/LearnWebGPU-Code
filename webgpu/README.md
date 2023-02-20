<div align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="https://raw.githubusercontent.com/eliemichel/LearnWebGPU/main/images/webgpu-dark.svg">
    <source media="(prefers-color-scheme: light)" srcset="https://raw.githubusercontent.com/eliemichel/LearnWebGPU/main/images/webgpu-light.svg">
    <img alt="Learn WebGPU Logo" src="images/webgpu-dark.svg" width="200">
  </picture>

  <a href="https://github.com/eliemichel/LearnWebGPU">LearnWebGPU</a> &nbsp;|&nbsp; <a href="https://github.com/eliemichel/WebGPU-Cpp">WebGPU-C++</a> &nbsp;|&nbsp; <a href="https://github.com/eliemichel/glfw3webgpu">glfw3webgpu</a> &nbsp;|&nbsp; <a href="https://github.com/eliemichel/WebGPU-binaries">WebGPU-binaries</a>
</div>

WebGPU distribution - Dawn
==========================

This branch is an alternative the binary release used by the [Learn WebGPU for native C++](https://eliemichel.github.io/LearnWebGPU) tutorial series that are based on [wgpu-native](https://github.com/gfx-rs/wgpu-native).

This distribution is based instead on a concurrent implementation, namely Google's [Dawn](https://dawn.googlesource.com/dawn). It is not a binary release but rather a CMake script that builds the WebGPU implementation together with your project (expect longer build time the first time then).

**Differences with Dawn**

 - Replace Dawn's custom gclient tool with a basic Python script (no need to install `depot_tools` then).
 - Add [webgpu.hpp](https://github.com/eliemichel/WebGPU-Cpp) to also provide a common C++ API with the wgpu-native implementation.
