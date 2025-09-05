#pragma once
#include <webgpu/webgpu.hpp>

// Forward-declare
struct GLFWwindow;
#include <array>

class Application {
public:
	// Initialize everything and return true if it went all right
	bool Initialize();

	// Uninitialize everything that was initialized
	void Terminate();

	// Draw a frame and handle events
	void MainLoop();

	// Return true as long as the main loop should keep on running
	bool IsRunning();
// After public methods, before private things
private:
	// Internal structs
	struct MyUniforms {
		std::array<float,4> color;
		float time;
		float _pad[3];
	};
	// Have the compiler check byte alignment
	static_assert(sizeof(MyUniforms) % 16 == 0);
private:
	wgpu::TextureView GetNextSurfaceView(); // NEW
// In Application.h
private:
    bool InitializePipeline();
private: // Application methods
	bool InitializeBuffers();
private: // Application methods
	void InitializeBindGroups();

private:
	// We put here all the variables that are shared between init and main loop
	GLFWwindow *m_window = nullptr;
	wgpu::Instance m_instance = nullptr; // NEW
	wgpu::Device m_device = nullptr; // NEW
	wgpu::Queue m_queue = nullptr; // NEW
	wgpu::Surface m_surface = nullptr; // NEW
	private:
		wgpu::RenderPipeline m_pipeline = nullptr;
	private: // In Application.h
		wgpu::TextureFormat m_surfaceFormat = wgpu::TextureFormat::Undefined;
	private: // Application attributes
		wgpu::Buffer m_vertexBuffer = nullptr;
		uint32_t m_vertexCount = 0;
	private: // Application attributes
		wgpu::Buffer m_pointBuffer = nullptr;
		wgpu::Buffer m_indexBuffer = nullptr;
		uint32_t m_indexCount = 0;
	private: // Application attributes
		wgpu::Buffer m_uniformBuffer = nullptr;
	private: // Application attributes
		wgpu::PipelineLayout m_layout = nullptr;
		wgpu::BindGroupLayout m_bindGroupLayout = nullptr;
	private: // Application attributes
		wgpu::BindGroup m_bindGroup = nullptr;
	private: // In Application.h
		wgpu::TextureFormat m_depthTextureFormat = wgpu::TextureFormat::Depth24Plus;
	private: // In Application.h
		wgpu::TextureView m_depthTextureView = nullptr;
};