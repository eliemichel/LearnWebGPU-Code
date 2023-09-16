LearnWebGPU - Code
==================

This repository contains the reference code base accompanying the [Learn WebGPU](learnwgpu.com) web book.

Branch `step030-test-foo-extension`: This corresponds to the code at the end of the page [Custom extensions](https://eliemichel.github.io/LearnWebGPU/appendices/custom-extensions.html).

Building
--------

```
# [...] Build https://github.com/eliemichel/wgpu-native/tree/eliemichel/foo with cargo
cmake -B build
# [...] Update _deps/webgpu-backend-wgpu-src/bin and /include
cmake --build build
```

Then run either `./build/App` (linux/macOS/MinGW) or `build\Debug\App.exe` (MSVC).
