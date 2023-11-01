cmake_minimum_required(VERSION 3.1...3.25)
project(
	LearnWebGPU
	VERSION 0.1.0
	LANGUAGES CXX C
)

include(utils.cmake)

# We add an option to enable different settings when developping the app than
# when distributing it.
option(DEV_MODE "Set up development helper settings" ON)

add_subdirectory(glfw)
add_subdirectory(webgpu)
add_subdirectory(glfw3webgpu)
add_subdirectory(imgui)

add_executable(App
	Application.h
	Application.cpp
	ResourceManager.h
	ResourceManager.cpp
	implementations.cpp
	main.cpp
	webgpu-release.h
	stb_image.h
	stb_image_write.h
	save_texture.h
)

if(DEV_MODE)
	# In dev mode, we load resources from the source tree, so that when we
	# dynamically edit resources (like shaders), these are correctly
	# versionned.
	target_compile_definitions(App PRIVATE
		RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/resources"
	)
else()
	# In release mode, we just load resources relatively to wherever the
	# executable is launched from, so that the binary is portable
	target_compile_definitions(App PRIVATE
		RESOURCE_DIR="./resources"
	)
endif()

target_include_directories(App PRIVATE .)
target_link_libraries(App PRIVATE webgpu glfw glfw3webgpu imgui)
target_compile_definitions(App PRIVATE NOMINMAX)

if(MSVC)
	target_compile_options(App PRIVATE /wd4201 /wd4706 /wd4127)
endif(MSVC)

set_target_properties(App PROPERTIES CXX_STANDARD 17)
# target_treat_all_warnings_as_errors(App)
target_copy_webgpu_binaries(App)
