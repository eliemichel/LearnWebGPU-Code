// {Begin block 'file: Application.h' (in root '020 - Opening a window - Next')}
#pragma once
// {Begin block 'Includes in Application.h' (in root '043 - More uniforms - Next')}
#include <webgpu/webgpu.hpp>

// Forward-declare
struct GLFWwindow;
#include <array>
// {End block 'Includes in Application.h' (in root '043 - More uniforms - Next')}

// {Begin block 'Application class' (in root '043 - More uniforms - Next')}
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
	// {Begin block 'Define uniform struct' (in root '043 - More uniforms - Next')}
	struct MyUniforms {
		std::array<float,4> color;
		float time;
		float _pad[3];
	};
	// Have the compiler check byte alignment
	static_assert(sizeof(MyUniforms) % 16 == 0);
	// {End block 'Define uniform struct' (in root '043 - More uniforms - Next')}
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
	// {Begin block 'Application attributes' (in root '044 - Dynamic uniforms - Next')}
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
		wgpu::Buffer m_vertexBuffer;
		uint32_t m_vertexCount;
	private: // Application attributes
		wgpu::Buffer m_pointBuffer;
		wgpu::Buffer m_indexBuffer;
		uint32_t m_indexCount;
	private: // Application attributes
		wgpu::Buffer m_uniformBuffer;
	private: // Application attributes
		wgpu::PipelineLayout m_layout;
		wgpu::BindGroupLayout m_bindGroupLayout;
	private: // Application attributes
		wgpu::BindGroup m_bindGroup;
	private: // In Application.h
		uint32_t m_uniformStride;
	// {End block 'Application attributes' (in root '044 - Dynamic uniforms - Next')}
};
// {End block 'Application class' (in root '043 - More uniforms - Next')}
// {End block 'file: Application.h' (in root '020 - Opening a window - Next')}