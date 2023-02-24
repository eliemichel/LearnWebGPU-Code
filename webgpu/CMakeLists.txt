include(cmake/FetchDawn.cmake)

# Test extraction
make_dawn_available()

# Expose an interface similar to the wgpu-native distribution
# so that this can be a drop-in replacement in the tutorial at
# https://eliemichel.github.io/LearnWebGPU
add_library(webgpu INTERFACE)
target_link_libraries(webgpu INTERFACE webgpu_dawn)
target_include_directories(webgpu INTERFACE
	"${CMAKE_CURRENT_SOURCE_DIR}/include"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/webgpu"
	"${CMAKE_BINARY_DIR}/_deps/dawn-src/include"
	"${CMAKE_BINARY_DIR}/_deps/dawn-src/include/webgpu"
)
# This is used to advertise the flavor of WebGPU that this zip provides
target_compile_definitions(webgpu INTERFACE WEBGPU_BACKEND_DAWN)


# Does nothing, as this dawn-based distribution of WebGPU is statically linked
function(target_copy_webgpu_binaries Target)
endfunction()
