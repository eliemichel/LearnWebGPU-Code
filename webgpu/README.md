<div align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="https://raw.githubusercontent.com/eliemichel/LearnWebGPU/main/images/webgpu-dark.svg">
    <source media="(prefers-color-scheme: light)" srcset="https://raw.githubusercontent.com/eliemichel/LearnWebGPU/main/images/webgpu-light.svg">
    <img alt="Learn WebGPU Logo" src="images/webgpu-dark.svg" width="200">
  </picture>

  <a href="https://github.com/eliemichel/LearnWebGPU">LearnWebGPU</a> &nbsp;|&nbsp; <a href="https://github.com/eliemichel/WebGPU-Cpp">WebGPU-C++</a> &nbsp;|&nbsp; <a href="https://github.com/eliemichel/glfw3webgpu">glfw3webgpu</a> &nbsp;|&nbsp; <a href="https://github.com/eliemichel/WebGPU-binaries">WebGPU-binaries</a>
</div>

WebGPU binaries
===============

This is the binary release used by the [Learn WebGPU for native C++](https://eliemichel.github.io/LearnWebGPU) tutorial series. It is a build of [wgpu-native](https://github.com/gfx-rs/wgpu-native) to which a `CMakeLists.txt` file has been added to easily integrate it to CMake projects.

**Differences with wgpu-native release**

 - Rename binaries for Windows and macOS. (The build process of wgpu-native gives them a wrong name that makes the linking fail.)
 - Add a CMakeLists.txt so that this can be easily integrated into a CMake project (just `add_subdirectory` this repo).
 - Add [webgpu.hpp](https://github.com/eliemichel/WebGPU-Cpp) to also provide a more idiomatic C++ API.

**NB** There is also an experimental [Dawn](https://dawn.googlesource.com/dawn)-based distribution in [the `dawn` branch](https://github.com/eliemichel/WebGPU-binaries/tree/dawn).
