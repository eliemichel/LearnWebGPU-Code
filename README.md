LearnWebGPU - Code
==================

This repository contains the reference code base accompanying the [Learn WebGPU](learnwgpu.com) web book.

Branch `step043`: This corresponds to the code at the end of the page [More uniforms](https://eliemichel.github.io/LearnWebGPU/basic-3d-rendering/shader-uniforms/multiple-uniforms.html).

Building
--------

```
cmake . -B build
cmake --build build
```

Then run either `./build/App` (linux/macOS/MinGW) or `build\Debug\App.exe` (MSVC).
