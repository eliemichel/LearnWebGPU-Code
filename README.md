<div align="center">
	<picture>
		<source media="(prefers-color-scheme: dark)" srcset="https://raw.githubusercontent.com/eliemichel/LearnWebGPU/main/images/webgpu-dark.svg">
		<source media="(prefers-color-scheme: light)" srcset="https://raw.githubusercontent.com/eliemichel/LearnWebGPU/main/images/webgpu-light.svg">
		<img alt="Learn WebGPU Logo" src="images/webgpu-dark.svg" width="200">
	</picture>
</div>

LearnWebGPU - Code
==================

This repository contains the reference code base accompanying the [Learn WebGPU](https://eliemichel.github.io/LearnWebGPU) web book.

Each step of the book is stored in a different branch. You can look at them incrementally, or compare them using GitHub's branch comparator.

Available step branches
-----------------------

 - [`step000`](../../tree/step000)
 - [`step001`](../../tree/step001)
 - [`step005`](../../tree/step005)
 - [`step010`](../../tree/step010)
 - [`step015`](../../tree/step015)
 - [`step018`](../../tree/step018)
 - [`step020`](../../tree/step020)
 - [`step025`](../../tree/step025)
 - [`step030`](../../tree/step030)

Building
--------

```
cmake . -B build
cmake --build build
```

Then run either `./build/App` (linux/macOS/MinGW) or `build\Debug\App.exe` (MSVC).
