// {Begin block 'file: ResourceManager.cpp' (in root '037 - Loading from file - Next - vanilla')}
// ResourceManager.cpp
#include "ResourceManager.h"
// {Begin block 'Other ResourceManager.cpp includes' (in root '037 - Loading from file - Next - vanilla')}
// Add to ResourceManager.cpp includes
#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
// ResourceManager.cpp
#include "webgpu-utils.h"
// {End block 'Other ResourceManager.cpp includes' (in root '037 - Loading from file - Next - vanilla')}

// {Begin block 'ResourceManager member implementations' (in root '037 - Loading from file - Next - vanilla')}
// {Begin block 'Implementation of ResourceManager::loadGeometry' (in root '050 - A simple example - Next - vanilla')}
bool ResourceManager::loadGeometry(
	const std::filesystem::path& path,
	std::vector<float>& pointData,
	std::vector<uint16_t>& indexData,
	int dimensions // <-- new argument
) {
	std::ifstream file(path);
	if (!file.is_open()) {
		std::cerr << "Could not load geometry!" << std::endl;
		return false;
	}

	pointData.clear();
	indexData.clear();

	enum class Section {
		None,
		Points,
		Indices,
	};
	Section currentSection = Section::None;

	float value;
	uint16_t index;
	std::string line;
	while (!file.eof()) {
		getline(file, line);
		
		// overcome the `CRLF` problem
		if (!line.empty() && line.back() == '\r') {
			line.pop_back();
		}
		
		if (line == "[points]") {
			currentSection = Section::Points;
		}
		else if (line == "[indices]") {
			currentSection = Section::Indices;
		}
		else if (line[0] == '#' || line.empty()) {
			// Do nothing, this is a comment
		}
		else if (currentSection == Section::Points) {
			std::istringstream iss(line);
			// Get x, y, z, r, g, b
			for (int i = 0; i < dimensions + 3; ++i) {
			    //              ^^^^^^^^^^^^^^ This was a 5
				iss >> value;
				pointData.push_back(value);
			}
		}
		else if (currentSection == Section::Indices) {
			std::istringstream iss(line);
			// Get corners #0 #1 and #2
			for (int i = 0; i < 3; ++i) {
				iss >> index;
				indexData.push_back(index);
			}
		}
	}
	return true;
}
// {End block 'Implementation of ResourceManager::loadGeometry' (in root '050 - A simple example - Next - vanilla')}
// {Begin block 'Implementation of ResourceManager::loadShaderModule' (in root '037 - Loading from file - Next - vanilla')}
WGPUShaderModule ResourceManager::loadShaderModule(const std::filesystem::path& path, WGPUDevice device) {
	std::ifstream file(path);
	if (!file.is_open()) {
		std::cerr << "Could not load geometry!" << std::endl;
		return nullptr;
	}
	file.seekg(0, std::ios::end);
	size_t size = file.tellg();
	std::string shaderSource(size, ' ');
	file.seekg(0);
	file.read(shaderSource.data(), size);

	WGPUShaderSourceWGSL wgslDesc = WGPU_SHADER_SOURCE_WGSL_INIT;
    wgslDesc.code = toWgpuStringView(shaderSource);
    WGPUShaderModuleDescriptor shaderDesc = WGPU_SHADER_MODULE_DESCRIPTOR_INIT;
    shaderDesc.nextInChain = &wgslDesc.chain;
    shaderDesc.label = toWgpuStringView(path.string());
    return wgpuDeviceCreateShaderModule(device, &shaderDesc);
}
// {End block 'Implementation of ResourceManager::loadShaderModule' (in root '037 - Loading from file - Next - vanilla')}
// {End block 'ResourceManager member implementations' (in root '037 - Loading from file - Next - vanilla')}
// {End block 'file: ResourceManager.cpp' (in root '037 - Loading from file - Next - vanilla')}