// {Begin block 'file: main.cpp' (in root '020 - Opening a window - Next')}
// {Begin block 'Includes' (in root '020 - Opening a window - Next')}
// {Begin block 'Includes in main.cpp' (in root '020 - Opening a window - Next')}
// In main.cpp
#include "Application.h"

#ifdef __EMSCRIPTEN__
#  include <emscripten.h>
#endif // __EMSCRIPTEN__
// {End block 'Includes in main.cpp' (in root '020 - Opening a window - Next')}
// {End block 'Includes' (in root '020 - Opening a window - Next')}

// {Begin block 'Main function' (in root '020 - Opening a window - Next')}
int main() {
	Application app;

	if (!app.Initialize()) {
		return 1;
	}

	// {Begin block 'Main loop' (in root '020 - Opening a window - Next')}
	#ifdef __EMSCRIPTEN__
		// {Begin block 'Emscripten main loop' (in root '020 - Opening a window - Next')}
		// Equivalent of the main loop when using Emscripten:
		auto callback = [](void *arg) {
		    //                   ^^^ 2. We get the address of the app in the callback.
		    Application* pApp = reinterpret_cast<Application*>(arg);
		    //                  ^^^^^^^^^^^^^^^^ 3. We force this address to be interpreted
		    //                                      as a pointer to an Application object.
		    pApp->MainLoop(); // 4. We can use the application object
		};
		emscripten_set_main_loop_arg(callback, &app, 0, true);
		//                                     ^^^^ 1. We pass the address of our application object.
		// {End block 'Emscripten main loop' (in root '020 - Opening a window - Next')}
	#else // __EMSCRIPTEN__
		while (app.IsRunning()) {
			app.MainLoop();
		}
	#endif // __EMSCRIPTEN__
	// {End block 'Main loop' (in root '020 - Opening a window - Next')}

	app.Terminate();

	return 0;
}
// {End block 'Main function' (in root '020 - Opening a window - Next')}
// {End block 'file: main.cpp' (in root '020 - Opening a window - Next')}