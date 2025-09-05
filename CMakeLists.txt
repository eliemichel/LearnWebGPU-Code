cmake_minimum_required(VERSION 3.16...3.30)
project(
	LearnWebGPU # name of the project, which will also be the name of the visual studio solution if you use it
	VERSION 0.1.0 # any version number
	LANGUAGES CXX C # programming languages used by the project
)

# In file 'CMakeLists.txt'
# Include webgpu directory, to define the 'webgpu' target
add_subdirectory(webgpu)
add_executable(App main.cpp)
# Add the 'webgpu' target as a dependency of our App
target_link_libraries(App PRIVATE webgpu)
# The application's binary must find the .so/.dll/.dylib file at runtime
# so we automatically copy it next to the binary.
target_copy_webgpu_binaries(App)

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
# Options that are specific to Emscripten
if (EMSCRIPTEN)
    # Generate a full web page rather than a simple WebAssembly module
    set_target_properties(App PROPERTIES SUFFIX ".html")
    # Enable the use of emscripten_sleep()
    target_link_options(App PRIVATE -sASYNCIFY)
endif()