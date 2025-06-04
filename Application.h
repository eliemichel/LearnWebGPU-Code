#pragma once
// In Application.h
#include <webgpu/webgpu.h>

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
    WGPUTextureView GetNextSurfaceView();
// In Application.h
private:
    bool InitializePipeline();
private: // Application methods
	bool InitializeBuffers();

private:
	// We put here all the variables that are shared between init and main loop
	// All these can be initialized to nullptr
	GLFWwindow *m_window = nullptr;
	WGPUInstance m_instance = nullptr;
	WGPUDevice m_device = nullptr;
	WGPUQueue m_queue = nullptr;
	WGPUSurface m_surface = nullptr;
	private:
		WGPURenderPipeline m_pipeline = nullptr;
	private: // In Application.h
		WGPUTextureFormat m_surfaceFormat = WGPUTextureFormat_Undefined;
	private: // Application attributes
		WGPUBuffer m_vertexBuffer = nullptr;
		uint32_t m_vertexCount = 0;
	private: // Application attributes
		WGPUBuffer m_pointBuffer = nullptr;
		WGPUBuffer m_indexBuffer = nullptr;
		uint32_t m_indexCount = 0;
};