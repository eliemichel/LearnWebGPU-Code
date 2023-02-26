LearnWebGPU - Code
==================

This repository contains the reference code base accompanying the [Learn WebGPU](learnwgpu.com) web book.

Branch `step105`: This corresponds to the code at the end of the page [Specular](https://eliemichel.github.io/LearnWebGPU/basic-3d-rendering/lighting-and-material/specular.html).

Building
--------

```
cmake . -B build
cmake --build build
```

Then run either `./build/App` (linux/macOS/MinGW) or `build\Debug\App.exe` (MSVC).
