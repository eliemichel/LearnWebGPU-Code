LearnWebGPU - Code
==================

This repository contains the reference code base accompanying the [Learn WebGPU](https://eliemichel.github.io/LearnWebGPU/) web book.

Branch `step034-vanilla`: This corresponds to the code at the end of the page [Index Buffer](https://eliemichel.github.io/LearnWebGPU/basic-3d-rendering/input-geometry/index-buffer.html).

Building
--------

```
cmake . -B build
cmake --build build
```

Then run either `./build/App` (linux/macOS/MinGW) or `build\Debug\App.exe` (MSVC).
