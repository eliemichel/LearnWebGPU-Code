LearnWebGPU - Code
==================

This repository contains the reference code base accompanying the [Learn WebGPU](learnwgpu.com) web book.

Branch `step030-sdl2-c`: This corresponds to the code at the end of the page [Hello Triangle](https://eliemichel.github.io/LearnWebGPU/basic-3d-rendering/hello-triangle.html).

This branch contains a minimal pure C implementation of the hello-triangle example using SDL2


Building
--------

```
cmake . -B build
cmake --build build
```

Then run either `./build/App` (linux/macOS/MinGW) or `build\Debug\App.exe` (MSVC).
