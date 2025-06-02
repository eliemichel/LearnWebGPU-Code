#pragma once
#include <webgpu/webgpu.hpp>

// Forward-declare
struct GLFWwindow;

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
private:
	wgpu::TextureView GetNextSurfaceView(); // NEW
// In Application.h
private:
    bool InitializePipeline();

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
};