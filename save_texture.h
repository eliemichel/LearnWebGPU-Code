#include "stb_image_write.h"

#include <webgpu/webgpu.hpp>

#include <filesystem>
#include <string>

#ifdef WEBGPU_BACKEND_WGPU
#include <webgpu/wgpu.h>
#endif

bool saveTexture(const std::filesystem::path path, wgpu::Device device, wgpu::Texture texture, int mipLevel) {
	using namespace wgpu;
	
	if (texture.getDimension() != TextureDimension::_2D) {
		throw std::runtime_error("Only 2D textures are supported by save_texture.h!");
	}
	uint32_t width = texture.getWidth() / (1 << mipLevel);
	uint32_t height = texture.getHeight() / (1 << mipLevel);
	uint32_t channels = 4; // TODO: infer from format
	uint32_t componentByteSize = 1; // TODO: infer from format

	uint32_t bytesPerRow = componentByteSize * channels * width;
	// Special case: WebGPU spec forbids texture-to-buffer copy with a
	// bytesPerRow lower than 256 so we first copy to a temporary texture.
	uint32_t paddedBytesPerRow = std::max(256u, bytesPerRow);

	// Create a buffer to get pixels
	BufferDescriptor pixelBufferDesc = Default;
	pixelBufferDesc.mappedAtCreation = false;
	pixelBufferDesc.usage = BufferUsage::MapRead | BufferUsage::CopyDst;
	pixelBufferDesc.size = paddedBytesPerRow * height;
	Buffer pixelBuffer = device.createBuffer(pixelBufferDesc);

	// Start encoding the commands
	Queue queue = device.getQueue();
	CommandEncoder encoder = device.createCommandEncoder(Default);

	// Get pixels from texture to buffer
	ImageCopyTexture source = Default;
	source.texture = texture;
	source.mipLevel = mipLevel;
	ImageCopyBuffer destination = Default;
	destination.buffer = pixelBuffer;
	destination.layout.bytesPerRow = paddedBytesPerRow;
	destination.layout.offset = 0;
	destination.layout.rowsPerImage = height;
	encoder.copyTextureToBuffer(source, destination, { width, height, 1 });

	// Issue commands
	CommandBuffer command = encoder.finish(Default);
	queue.submit(command);

	// Map buffer
	bool done = false;
	bool success = false;
	auto callbackHandle = pixelBuffer.mapAsync(MapMode::Read, 0, pixelBufferDesc.size, [&](BufferMapAsyncStatus status) {
		if (status != BufferMapAsyncStatus::Success) {
			success = false;
		}
		else {
			const unsigned char* pixelData = (const unsigned char*)pixelBuffer.getConstMappedRange(0, pixelBufferDesc.size);
			int writeSuccess = stbi_write_png(path.string().c_str(), (int)width, (int)height, (int)channels, pixelData, paddedBytesPerRow);

			pixelBuffer.unmap();

			success = writeSuccess != 0;
		}
		done = true;
	});

	// Wait for mapping
	while (!done) {
#ifdef WEBGPU_BACKEND_WGPU
		wgpuQueueSubmit(queue, 0, nullptr);
#else
		wgpuDeviceTick(device);
#endif
	}

	// Clean-up

	pixelBuffer.destroy();

#ifdef WEBGPU_BACKEND_WGPU
	wgpuBufferDrop(pixelBuffer);
#else
	wgpuBufferRelease(pixelBuffer);
	wgpuCommandEncoderRelease(encoder);
	wgpuCommandBufferRelease(command);
	wgpuQueueRelease(queue);
#endif

	return success;
}
