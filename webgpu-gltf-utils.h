/**
 * Conversion utilities from GLTF to WebGPU
 */
#pragma once

#include "tiny_gltf.h"

#include <webgpu/webgpu.hpp>
#include <glm/glm.hpp>

namespace wgpu::gltf {

// Return Undefined if not supported
wgpu::VertexFormat vertexFormatFromAccessor(const tinygltf::Accessor& accessor);

// Return Undefined if not supported
wgpu::IndexFormat indexFormatFromAccessor(const tinygltf::Accessor& accessor);

// Return Force32 if not supported
wgpu::FilterMode filterModeFromGltf(int tinygltfFilter);

// Return Force32 if not supported
wgpu::MipmapFilterMode mipmapFilterModeFromGltf(int tinygltfFilter);

// Return Force32 if not supported
wgpu::AddressMode addressModeFromGltf(int tinygltfWrap);

// Return Undefined if not supported
// If useClosestMatch is true, return a format that can contain the provided image, even though it has additional channels
wgpu::TextureFormat textureFormatFromGltfImage(const tinygltf::Image& image, bool useClosestMatch = false);

// Build a transform matrix either from node.matrix or the TRS fields
glm::mat4 nodeMatrix(const tinygltf::Node& node);

// Get the topology from prim.mode
// return Force32 if not compatible
wgpu::PrimitiveTopology primitiveTopologyFromGltf(const tinygltf::Primitive& prim);

} // namespace wgpu::gltf
