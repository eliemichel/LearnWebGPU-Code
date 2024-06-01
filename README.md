LearnWebGPU - Code
==================

This repository contains the reference code base accompanying the [Learn WebGPU](learnwgpu.com) web book.

Branch `step005`: This corresponds to the code at the end of the page [The Adapter](learnwgpu.com/getting-started/adapter-and-device/the-adapter.html).

Building
--------

```
cmake . -B build
cmake --build build
```

Then run either `./build/App` (linux/macOS/MinGW) or `build\Debug\App.exe` (MSVC).
