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

if (NOT EMSCRIPTEN)
	add_subdirectory(glfw)
endif()
add_subdirectory(webgpu)
if (NOT EMSCRIPTEN)
	add_subdirectory(glfw3webgpu)
endif()
add_subdirectory(imgui)

add_executable(App
	Application.h
	Application.cpp
	MarchingCubesRenderer.h
	MarchingCubesRenderer.cpp
	ResourceManager.h
	ResourceManager.cpp
	implementations.cpp
	main.cpp
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
target_link_libraries(App PRIVATE glfw webgpu imgui)
if (NOT EMSCRIPTEN)
	target_link_libraries(App PRIVATE glfw3webgpu)
endif()

if (EMSCRIPTEN)
	set(SHELL_FILE shell.html)

	target_link_options(App PRIVATE
		-sUSE_GLFW=3
		-sUSE_WEBGPU
		-sALLOW_MEMORY_GROWTH
		--shell-file "${CMAKE_CURRENT_SOURCE_DIR}/${SHELL_FILE}"
		--preload-file "${CMAKE_CURRENT_SOURCE_DIR}/resources"
	)

	# Make sure to re-link when the shell file changes
	set_property(
		TARGET App
		PROPERTY LINK_DEPENDS
		"${CMAKE_CURRENT_SOURCE_DIR}/${SHELL_FILE}"
	)

	set_target_properties(App PROPERTIES SUFFIX ".html")
endif()

if(MSVC)
	target_compile_options(App PRIVATE /wd4201)
endif(MSVC)

set_target_properties(App PROPERTIES CXX_STANDARD 17)
target_treat_all_warnings_as_errors(App)
target_copy_webgpu_binaries(App)
