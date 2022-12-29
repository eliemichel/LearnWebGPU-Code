LearnWebGPU - Code
==================

This repository contains the reference code base accompanying the [Learn WebGPU](learnwgpu.com) web book.

Branch `step030`: This corresponds to the code at the end of the page [Hello Triangle](https://eliemichel.github.io/LearnWebGPU/basic-3d-rendering/hello-triangle.html).

Building
--------

```
cmake . -B build
cmake --build build
```

Then run either `./build/App` (linux/macOS/MinGW) or `build\Debug\App.exe` (MSVC).
