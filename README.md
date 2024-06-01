LearnWebGPU - Code
==================

This repository contains the reference code base accompanying the [Learn WebGPU](https://eliemichel.github.io/LearnWebGPU) web book.

Branch `step010`: This corresponds to the code at the end of the page [The Device](https://eliemichel.github.io/LearnWebGPU/getting-started/adapter-and-device/the-device.html).

Building
--------

```
cmake . -B build
cmake --build build
```

Then run either `./build/App` (linux/macOS/MinGW) or `build\Debug\App.exe` (MSVC).
