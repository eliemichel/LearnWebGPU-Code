cmake_minimum_required(VERSION 3.0...3.25)
project(
	LearnWebGPU
	VERSION 0.1.0
	LANGUAGES CXX C
)

# Add glfw
if (NOT EMSCRIPTEN)
	add_subdirectory(glfw)
else()
	# Emscripten has built-in support for GLFW but requires the `-sUSE_GLFW=3` link option:
	add_library(glfw INTERFACE)
	target_link_options(glfw INTERFACE -sUSE_GLFW=3)
endif()

add_subdirectory(webgpu)
# Add a little extension of glfw that connects it to WebGPU
add_subdirectory(glfw3webgpu)

add_executable(App
	main.cpp
	webgpu-utils.h
	webgpu-utils.cpp
)

# Add glfw and glfw3webgpu as dependencies of our App
target_link_libraries(App PRIVATE glfw webgpu glfw3webgpu)

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

if (EMSCRIPTEN)
	set_target_properties(App PROPERTIES SUFFIX ".html")
	target_link_options(App PRIVATE -sASYNCIFY)
endif()
