cmake_minimum_required(VERSION 3.1...3.25)
project(
	LearnWebGPU
	VERSION 0.1.0
	LANGUAGES CXX C
)

include(utils.cmake)

add_subdirectory(glfw)
add_subdirectory(webgpu)

add_executable(App
	main.cpp
	# We directly add the glfw3webgpu source files to our project
	glfw3webgpu.h
	glfw3webgpu.c
)

target_link_libraries(App PRIVATE glfw webgpu)

set_target_properties(App PROPERTIES CXX_STANDARD 17)
target_treat_all_warnings_as_errors(App)
target_copy_webgpu_binaries(App)
