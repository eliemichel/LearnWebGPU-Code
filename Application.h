// {Begin block 'file: Application.h' (in root '020 - Opening a window - Next')}
#pragma once
// {Begin block 'Includes in Application.h' (in root '028 - C++ Wrapper - Next')}
#include <webgpu/webgpu.hpp>

// Forward-declare
struct GLFWwindow;
// {End block 'Includes in Application.h' (in root '028 - C++ Wrapper - Next')}

// {Begin block 'Application class' (in root '025 - First Color - Next')}
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

private:
	// We put here all the variables that are shared between init and main loop
	// {Begin block 'Application attributes' (in root '028 - C++ Wrapper - Next')}
	GLFWwindow *m_window = nullptr;
	wgpu::Instance m_instance = nullptr; // NEW
	wgpu::Device m_device = nullptr; // NEW
	wgpu::Queue m_queue = nullptr; // NEW
	wgpu::Surface m_surface = nullptr; // NEW
	// {End block 'Application attributes' (in root '028 - C++ Wrapper - Next')}
};
// {End block 'Application class' (in root '025 - First Color - Next')}
// {End block 'file: Application.h' (in root '020 - Opening a window - Next')}