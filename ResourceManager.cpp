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

#include "ResourceManager.h"

#include "tiny_obj_loader.h"
#include "stb_image.h"

#include <fstream>
#include <array>
#include <vector>
#include <filesystem>

using namespace wgpu;
namespace fs = std::filesystem;

const fs::path cubemapPaths[] = {
	"cubemap-posX.png",
	"cubemap-negX.png",
	"cubemap-posY.png",
	"cubemap-negY.png",
	"cubemap-posZ.png",
	"cubemap-negZ.png",
};

ShaderModule ResourceManager::loadShaderModule(const path& path, Device device) {
	std::ifstream file(path);
	if (!file.is_open()) {
		return nullptr;
	}
	file.seekg(0, std::ios::end);
	size_t size = file.tellg();
	std::string shaderSource(size, ' ');
	file.seekg(0);
	file.read(shaderSource.data(), size);

	ShaderModuleWGSLDescriptor shaderCodeDesc{};
	shaderCodeDesc.chain.next = nullptr;
	shaderCodeDesc.chain.sType = SType::ShaderModuleWGSLDescriptor;
	ShaderModuleDescriptor shaderDesc{};
	shaderDesc.nextInChain = &shaderCodeDesc.chain;
#ifdef WEBGPU_BACKEND_WGPU
	shaderCodeDesc.code = shaderSource.c_str();
	shaderDesc.hintCount = 0;
	shaderDesc.hints = nullptr;
#else // WEBGPU_BACKEND_WGPU
	shaderCodeDesc.source = shaderSource.c_str();
#endif // WEBGPU_BACKEND_WGPU
	return device.createShaderModule(shaderDesc);
}

bool ResourceManager::loadGeometryFromObj(const path& path, std::vector<VertexAttributes>& vertexData) {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string warn;
	std::string err;

	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.string().c_str());

	if (!warn.empty()) {
		std::cout << warn << std::endl;
	}

	if (!err.empty()) {
		std::cerr << err << std::endl;
	}

	if (!ret) {
		return false;
	}

	// Fill in vertexData here
	vertexData.clear();
	for (const auto& shape : shapes) {
		size_t offset = vertexData.size();
		vertexData.resize(offset + shape.mesh.indices.size());

		for (int i = 0; i < vertexData.size(); ++i) {
			const tinyobj::index_t& idx = shape.mesh.indices[i];

			vertexData[offset + i].position = {
				attrib.vertices[3 * idx.vertex_index + 0],
				-attrib.vertices[3 * idx.vertex_index + 2],
				attrib.vertices[3 * idx.vertex_index + 1]
			};

			vertexData[offset + i].normal = {
				attrib.normals[3 * idx.normal_index + 0],
				-attrib.normals[3 * idx.normal_index + 2],
				attrib.normals[3 * idx.normal_index + 1]
			};

			vertexData[offset + i].color = {
				attrib.colors[3 * idx.vertex_index + 0],
				attrib.colors[3 * idx.vertex_index + 1],
				attrib.colors[3 * idx.vertex_index + 2]
			};

			// Fix the UV convention
			vertexData[offset + i].uv = {
				attrib.texcoords[2 * idx.texcoord_index + 0],
				1 - attrib.texcoords[2 * idx.texcoord_index + 1]
			};
		}
	}

	computeTextureFrameAttributes(vertexData);

	return true;
}

void ResourceManager::computeTextureFrameAttributes(std::vector<VertexAttributes>& vertexData) {
	size_t triangleCount = vertexData.size() / 3;
	// We compute the local texture frame per triangle
	for (int t = 0; t < triangleCount; ++t) {
		VertexAttributes* v = &vertexData[3 * t];

		// Formulas from http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-13-normal-mapping/

		vec3 deltaPos1 = v[1].position - v[0].position;
		vec3 deltaPos2 = v[2].position - v[0].position;

		vec2 deltaUV1 = v[1].uv - v[0].uv;
		vec2 deltaUV2 = v[2].uv - v[0].uv;

		float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
		vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
		vec3 bitangent = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r;
		
		// We assign these to the 3 corners of the triangle
		for (int k = 0; k < 3; ++k) {
			vertexData[3 * t + k].tangent = tangent;
			vertexData[3 * t + k].bitangent = bitangent;
		}
	}
}

// Equivalent of std::bit_width that is available from C++20 onward
uint32_t bit_width(uint32_t m) {
	if (m == 0) return 0;
	else { uint32_t w = 0; while (m >>= 1) ++w; return w; }
}

template<typename component_t>
static void writeMipMaps(
	Device device,
	Texture texture,
	Extent3D textureSize,
	uint32_t mipLevelCount,
	const component_t* pixelData
) {
	Queue queue = device.getQueue();

	// Arguments telling which part of the texture we upload to
	ImageCopyTexture destination;
	destination.texture = texture;
	destination.origin = { 0, 0, 0 };
	destination.aspect = TextureAspect::All;

	// Arguments telling how the C++ side pixel memory is laid out
	TextureDataLayout source;
	source.offset = 0;

	// Create image data
	Extent3D mipLevelSize = textureSize;
	std::vector<component_t> previousLevelPixels;
	Extent3D previousMipLevelSize;
	for (uint32_t level = 0; level < mipLevelCount; ++level) {
		std::vector<component_t> pixels(4 * mipLevelSize.width * mipLevelSize.height);
		if (level == 0) {
			// We cannot really avoid this copy since we need this
			// in previousLevelPixels at the next iteration
			memcpy(pixels.data(), pixelData, pixels.size() * sizeof(component_t));
		}
		else {
			// Create mip level data
			for (uint32_t i = 0; i < mipLevelSize.width; ++i) {
				for (uint32_t j = 0; j < mipLevelSize.height; ++j) {
					component_t* p = &pixels[4 * (j * mipLevelSize.width + i)];
					// Get the corresponding 4 pixels from the previous level
					component_t* p00 = &previousLevelPixels[4 * ((2 * j + 0) * previousMipLevelSize.width + (2 * i + 0))];
					component_t* p01 = &previousLevelPixels[4 * ((2 * j + 0) * previousMipLevelSize.width + (2 * i + 1))];
					component_t* p10 = &previousLevelPixels[4 * ((2 * j + 1) * previousMipLevelSize.width + (2 * i + 0))];
					component_t* p11 = &previousLevelPixels[4 * ((2 * j + 1) * previousMipLevelSize.width + (2 * i + 1))];
					// Average
					p[0] = (p00[0] + p01[0] + p10[0] + p11[0]) / (component_t)4;
					p[1] = (p00[1] + p01[1] + p10[1] + p11[1]) / (component_t)4;
					p[2] = (p00[2] + p01[2] + p10[2] + p11[2]) / (component_t)4;
					p[3] = (p00[3] + p01[3] + p10[3] + p11[3]) / (component_t)4;
				}
			}
		}

		// Upload data to the GPU texture
		destination.mipLevel = level;
		source.bytesPerRow = 4 * mipLevelSize.width * sizeof(component_t);
		source.rowsPerImage = mipLevelSize.height;
		queue.writeTexture(destination, pixels.data(), pixels.size() * sizeof(component_t), source, mipLevelSize);

		previousLevelPixels = std::move(pixels);
		previousMipLevelSize = mipLevelSize;
		mipLevelSize.width /= 2;
		mipLevelSize.height /= 2;
	}
}

Texture ResourceManager::loadTexture(const path& path, Device device, TextureView * pTextureView) {
	int width, height, channels;
	uint8_t* pixelData = stbi_load(path.string().c_str(), &width, &height, &channels, 4 /* force 4 channels */);
	if (nullptr == pixelData) return nullptr;

	TextureDescriptor textureDesc;
	textureDesc.dimension = TextureDimension::_2D;
	textureDesc.format = TextureFormat::RGBA8Unorm;
	textureDesc.size = { (unsigned int)width, (unsigned int)height, 1 };
	textureDesc.mipLevelCount = bit_width(std::max(textureDesc.size.width, textureDesc.size.height));
	textureDesc.sampleCount = 1;
	textureDesc.usage = TextureUsage::TextureBinding | TextureUsage::CopyDst;
	textureDesc.viewFormatCount = 0;
	textureDesc.viewFormats = nullptr;
	Texture texture = device.createTexture(textureDesc);

	writeMipMaps(device, texture, textureDesc.size, textureDesc.mipLevelCount, pixelData);

	stbi_image_free(pixelData);

	if (pTextureView) {
		TextureViewDescriptor textureViewDesc;
		textureViewDesc.aspect = TextureAspect::All;
		textureViewDesc.baseArrayLayer = 0;
		textureViewDesc.arrayLayerCount = 1;
		textureViewDesc.baseMipLevel = 0;
		textureViewDesc.mipLevelCount = textureDesc.mipLevelCount;
		textureViewDesc.dimension = TextureViewDimension::_2D;
		textureViewDesc.format = textureDesc.format;
		*pTextureView = texture.createView(textureViewDesc);
	}

	return texture;
}

Texture ResourceManager::loadPrefilteredCubemap(const path& rootPath, Device device, TextureView* pTextureView) {
	Extent3D cubemapSize = { 0, 0, 6 };
	std::vector<std::array<uint8_t*, 6>> pixelData;

	// Load all mip levels
	for (uint32_t level = 0; level < 100; ++level) {
		fs::path dirpath = rootPath / ("cubemap-mip" + std::to_string(level));
		if (!fs::is_directory(dirpath)) {
			break;
		}
		std::array<uint8_t*, 6> mipPixelData;
		for (uint32_t layer = 0; layer < 6; ++layer) {
			int width, height, channels;
			fs::path path = dirpath / cubemapPaths[layer];
			mipPixelData[layer] = stbi_load(path.string().c_str(), &width, &height, &channels, 4 /* force 4 channels */);
			if (nullptr == mipPixelData[layer]) throw std::runtime_error("Could not load input texture!");
			if (level == 0 && layer == 0) {
				cubemapSize.width = (uint32_t)width;
				cubemapSize.height = (uint32_t)height;
			}
		}
		pixelData.push_back(mipPixelData);
	}

	if (pixelData.empty()) {
		return nullptr;
	}

	TextureDescriptor textureDesc;
	textureDesc.dimension = TextureDimension::_2D;
	textureDesc.format = TextureFormat::RGBA8Unorm;
	textureDesc.size = cubemapSize;
	textureDesc.mipLevelCount = (uint32_t)pixelData.size();
	textureDesc.sampleCount = 1;
	textureDesc.usage = TextureUsage::TextureBinding | TextureUsage::CopyDst;
	textureDesc.viewFormatCount = 0;
	textureDesc.viewFormats = nullptr;
	Texture texture = device.createTexture(textureDesc);

	// Upload texture data for all faces and all MIP levels
	ImageCopyTexture destination;
	destination.texture = texture;
	destination.aspect = TextureAspect::All;
	destination.mipLevel = 0;

	TextureDataLayout source;
	source.offset = 0;

	Queue queue = device.getQueue();
	
	Extent3D mipLayerSize = { cubemapSize.width , cubemapSize.height , 1 };
	for (uint32_t level = 0; level < textureDesc.mipLevelCount; ++level) {
		source.bytesPerRow = 4 * mipLayerSize.width;
		source.rowsPerImage = mipLayerSize.height;

		for (uint32_t layer = 0; layer < 6; ++layer) {
			destination.origin = { 0, 0, layer };
			destination.mipLevel = level;
			queue.writeTexture(
				destination,
				pixelData[level][layer],
				(size_t)(4 * mipLayerSize.width * mipLayerSize.height),
				source,
				mipLayerSize
			);

			// Free CPU-side data
			stbi_image_free(pixelData[level][layer]);
		}

		mipLayerSize.width /= 2;
		mipLayerSize.height /= 2;
	}

#ifdef WEBGPU_BACKEND_DAWN
	wgpuQueueRelease(queue);
#endif

	if (pTextureView) {
		TextureViewDescriptor textureViewDesc;
		textureViewDesc.aspect = TextureAspect::All;
		textureViewDesc.baseArrayLayer = 0;
		textureViewDesc.arrayLayerCount = 6;
		textureViewDesc.baseMipLevel = 0;
		textureViewDesc.mipLevelCount = textureDesc.mipLevelCount;
		textureViewDesc.dimension = TextureViewDimension::Cube;
		textureViewDesc.format = textureDesc.format;
		*pTextureView = texture.createView(textureViewDesc);
	}

	return texture;
}
