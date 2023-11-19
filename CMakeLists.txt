cmake_minimum_required(VERSION 3.1...3.25)
project(
	LearnWebGPU
	VERSION 0.1.0
	LANGUAGES CXX C
)

include(utils.cmake)

add_subdirectory(SDL2)
add_subdirectory(webgpu)
add_subdirectory(sdl2webgpu)

add_executable(App
	src/main.c
	src/webgpu-utils.h
	src/webgpu-utils.c
)

target_link_libraries(App PRIVATE SDL2::SDL2 webgpu sdl2webgpu)

set_target_properties(App PROPERTIES
	CXX_STANDARD 17
	VS_DEBUGGER_ENVIRONMENT "DAWN_DEBUG_BREAK_ON_ERROR=1"
)

# target_treat_all_warnings_as_errors(App)
target_copy_webgpu_binaries(App)
