/**
 * This file is part of the "Learn WebGPU for C++" book.
 *   https://github.com/eliemichel/LearnWebGPU
 *
 * MIT License
 * Copyright (c) 2022-2023 Elie Michel
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#if 0
#include "Application.h"

int main(int, char**) {
	Application app;
	app.onInit();

	while (app.isRunning()) {
		app.onFrame();
	}

	app.onFinish();
	return 0;
}
#endif

#include "ibl-utils.h"
#include "filament/Image.h"
#include "stb_image.h"

#include <fstream>
#include <filesystem>
#include <array>
#include <memory>

namespace fs = std::filesystem;
using namespace ibl_utils;

const fs::path cubemapPaths[] = {
	"cubemap-posX.png",
	"cubemap-negX.png",
	"cubemap-posY.png",
	"cubemap-negY.png",
	"cubemap-posZ.png",
	"cubemap-negZ.png",
};

int main(int, char**) {
	size_t cubemapSize = 0;
	std::array<uint8_t*, 6> pixelData;
	for (uint32_t layer = 0; layer < 6; ++layer) {
		int width, height, channels;
		fs::path path = fs::path(RESOURCE_DIR) / "autumn_park" / "cubemap-mip0" / cubemapPaths[layer];
		pixelData[layer] = stbi_load(path.string().c_str(), &width, &height, &channels, 4 /* force 4 channels */);
		if (nullptr == pixelData[layer]) throw std::runtime_error("Could not load input texture!");
		if (layer == 0) {
			cubemapSize = (size_t)width;
		}
	}

	Cubemap cm(cubemapSize);
	std::array<std::unique_ptr<filament::ibl::Image>, 6> faceImages;
	for (uint32_t layer = 0; layer < 6; ++layer) {
		auto& image = faceImages[layer];
		image = std::make_unique<filament::ibl::Image>(cubemapSize, cubemapSize);
		for (size_t j = 0; j < cubemapSize; ++j) {
			for (size_t i = 0; i < cubemapSize; ++i) {
				uint8_t* rawPixel = &pixelData[layer][(j * cubemapSize + i) * 4];
				glm::vec3* pixel = reinterpret_cast<glm::vec3*>(image->getPixelRef(i, j));
				pixel->r = rawPixel[0] / 255.0f;
				pixel->g = rawPixel[1] / 255.0f;
				pixel->b = rawPixel[2] / 255.0f;
			}
		}
		cm.setImageForFace((Cubemap::Face)layer, *image);
	}

	SphericalHarmonics SH = computeSH(cm);
	std::ofstream f(RESOURCE_DIR "/SH.txt");
	for (glm::vec3& sh : SH) {
		f << sh.x << "\t" << sh.y << "\t" << sh.z << "\n";
	}
	return 0;
}
