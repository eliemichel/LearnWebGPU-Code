cmake_minimum_required(VERSION 3.1...3.25)
project(
	LearnWebGPU
	VERSION 0.1.0
	LANGUAGES CXX C
)

include(utils.cmake)

# Include glfw directory, to define the 'glfw' target
add_subdirectory(glfw)

add_executable(App
	main.cpp
)

# Add the 'glfw' target as a dependency of our App
target_link_libraries(App PRIVATE glfw)

set_target_properties(App PROPERTIES CXX_STANDARD 17)
target_treat_all_warnings_as_errors(App)
