cmake_minimum_required(VERSION 3.16...3.30)
project(
	LearnWebGPU # name of the project, which will also be the name of the visual studio solution if you use it
	VERSION 0.1.0 # any version number
	LANGUAGES CXX C # programming languages used by the project
)

# In file 'CMakeLists.txt'
add_executable(App main.cpp)

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
endif()