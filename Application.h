#pragma once
// In Application.h
#include <webgpu/webgpu.h>

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
    WGPUTextureView GetNextSurfaceView();
// In Application.h
private:
    bool InitializePipeline();
private: // Application methods
	bool InitializeBuffers();
private: // Application methods
	void InitializeBindGroups();

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
	private: // Application attributes
		WGPUBuffer m_uniformBuffer = nullptr;
	private: // Application attributes
		WGPUPipelineLayout m_layout = nullptr;
		WGPUBindGroupLayout m_bindGroupLayout = nullptr;
	private: // Application attributes
		WGPUBindGroup m_bindGroup = nullptr;
};