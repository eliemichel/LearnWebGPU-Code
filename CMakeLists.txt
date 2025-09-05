# {Begin block 'file: CMakeLists.txt' (in root '000 - Project setup - Next')}
cmake_minimum_required(VERSION 3.16...3.30)
project(
	LearnWebGPU # name of the project, which will also be the name of the visual studio solution if you use it
	VERSION 0.1.0 # any version number
	LANGUAGES CXX C # programming languages used by the project
)

# {Begin block 'Define app target' (in root '010 - The Device - Next')}
# {Begin block 'Dependency subdirectories' (in root '020 - Opening a window - Next')}
if (NOT EMSCRIPTEN)
	# Add the 'glfw' directory, which contains the definition of a 'glfw' target
	add_subdirectory(glfw)
else()
	# Create a mock 'glfw' target that just sets the `-sUSE_GLFW=3` link option:
	add_library(glfw INTERFACE)
	target_link_options(glfw INTERFACE -sUSE_GLFW=3)
endif()
# In both cases, we can now link to the 'glfw' target
# Include webgpu directory, to define the 'webgpu' target
add_subdirectory(webgpu)
# After adding glfw and webgpu directories:
add_subdirectory(glfw3webgpu)
# {End block 'Dependency subdirectories' (in root '020 - Opening a window - Next')}

add_executable(App
	# {Begin block 'App source files' (in root '028 - C++ Wrapper - Next')}
	main.cpp
	webgpu-utils.h
	webgpu-utils.cpp
	Application.h
	Application.cpp
	implementations.cpp
	# {End block 'App source files' (in root '028 - C++ Wrapper - Next')}
)

# {Begin block 'Link libraries' (in root '020 - Opening a window - Next')}
# After defining the App target:
target_link_libraries(App PRIVATE glfw webgpu glfw3webgpu)
target_copy_webgpu_binaries(App)
# {End block 'Link libraries' (in root '020 - Opening a window - Next')}
# {End block 'Define app target' (in root '010 - The Device - Next')}

# {Begin block 'Recommended extras' (in root '000 - Project setup - Next')}
set_target_properties(App PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
	COMPILE_WARNING_AS_ERROR ON
)
if (MSVC)
	target_compile_options(App PRIVATE /W4)
else()
	target_compile_options(App PRIVATE -Wall -Wextra -pedantic)
endif()
if (XCODE)
	set_target_properties(App PROPERTIES
		XCODE_GENERATE_SCHEME ON
		XCODE_SCHEME_ENABLE_GPU_FRAME_CAPTURE_MODE "Metal"
	)
endif()
# NB: This only works if put in the top-level CMakeLists.txt
set_directory_properties(PROPERTIES
	VS_STARTUP_PROJECT App
)
# {End block 'Recommended extras' (in root '000 - Project setup - Next')}
# Options that are specific to Emscripten
if (EMSCRIPTEN)
    # {Begin block 'Emscripten-specific options' (in root '005 - The Adapter - Next')}
    # Generate a full web page rather than a simple WebAssembly module
    set_target_properties(App PROPERTIES SUFFIX ".html")
    # Enable the use of emscripten_sleep()
    target_link_options(App PRIVATE -sASYNCIFY)
    # {End block 'Emscripten-specific options' (in root '005 - The Adapter - Next')}
endif()
# {End block 'file: CMakeLists.txt' (in root '000 - Project setup - Next')}