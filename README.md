LearnWebGPU - Code
==================

This repository contains the reference code base accompanying the [Learn WebGPU](https://eliemichel.github.io/LearnWebGPU) C++ programming guide.

Branch `step210`: This corresponds to the code at the end of the page [Cubemap Conversion](https://eliemichel.github.io/LearnWebGPU/basic-compute/image-processing/cubemap-conversion.html).

Building
--------

```
cmake . -B build
cmake --build build
```

Then run either `./build/App` (linux/macOS/MinGW) or `build\Debug\App.exe` (MSVC).
