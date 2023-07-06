LearnWebGPU - Code
==================

This repository contains the reference code base accompanying the [Learn WebGPU](learnwgpu.com) web book.

Branch `step050`: This corresponds to the code at the end of the page [A simple example](http://localhost:8000/basic-3d-rendering/3d-meshes/a-simple-example.html).

Building
--------

```
cmake . -B build
cmake --build build
```

Then run either `./build/App` (linux/macOS/MinGW) or `build\Debug\App.exe` (MSVC).
