// {Begin block 'file: ResourceManager.h' (in root '037 - Loading from file - Next')}
// ResourceManager.h
#pragma once
// {Begin block 'ResourceManager.h includes' (in root '037 - Loading from file - Next')}
// Add to ResourceManager.h includes
#include <vector>
#include <filesystem>
#include <webgpu/webgpu.hpp>
// {End block 'ResourceManager.h includes' (in root '037 - Loading from file - Next')}

class ResourceManager {
public:
	// {Begin block 'Public ResourceManager members' (in root '037 - Loading from file - Next')}
	// {Begin block 'Declaration of ResourceManager::loadGeometry' (in root '050 - A simple example - Next')}
	/**
	 * Load a file from `path` using our ad-hoc format and populate the `pointData`
	 * and `indexData` vectors.
	 */
	static bool loadGeometry(
		const std::filesystem::path& path,
		std::vector<float>& pointData,
		std::vector<uint16_t>& indexData,
		int dimensions // <-- new argument
	);
	// {End block 'Declaration of ResourceManager::loadGeometry' (in root '050 - A simple example - Next')}
	// {Begin block 'Declaration of ResourceManager::loadShaderModule' (in root '037 - Loading from file - Next')}
	/**
	 * Create a shader module for a given WebGPU `device` from a WGSL shader source
	 * loaded from file `path`.
	 */
	static wgpu::ShaderModule loadShaderModule(
		const std::filesystem::path& path,
		wgpu::Device device
	);
	// {End block 'Declaration of ResourceManager::loadShaderModule' (in root '037 - Loading from file - Next')}
	// {End block 'Public ResourceManager members' (in root '037 - Loading from file - Next')}

private:
	// {Begin block 'Private ResourceManager members' (in root '037 - Loading from file - Next')}
	// {End block 'Private ResourceManager members' (in root '037 - Loading from file - Next')}
};
// {End block 'file: ResourceManager.h' (in root '037 - Loading from file - Next')}