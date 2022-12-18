LearnWebGPU - Code
==================

This repository contains the reference code base accompanying the [Learn WebGPU](https://eliemichel.github.io/LearnWebGPU) web book.

Each step of the book is stored in a different branch. You can look at them incrementally, or compare them using GitHub's branch comparator.

Available step branches
-----------------------

 - [`step000`](./tree/step000)

Building
--------

```
cmake . -B build
cmake --build build
```

Then run either `./build/App` (linux/macOS/MinGW) or `build\Debug\App.exe` (MSVC).
