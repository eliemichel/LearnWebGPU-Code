set(WGPU ${CMAKE_CURRENT_SOURCE_DIR})
if (NOT ARCH)
	set(ARCH x86_64)
endif()

# A pre-compiled target (IMPORTED) that is a dynamically
# linked library (SHARED, meaning .dll, .so or .dylib).
add_library(webgpu SHARED IMPORTED GLOBAL)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")

	set(WGPU_RUNTIME_LIB ${WGPU}/windows-${ARCH}/wgpu_native.dll)
	set_target_properties(
		webgpu
		PROPERTIES
			IMPORTED_LOCATION "${WGPU_RUNTIME_LIB}"
			IMPORTED_IMPLIB "${WGPU}/windows-${ARCH}/wgpu_native.lib"
			INTERFACE_INCLUDE_DIRECTORIES "${WGPU}"
	)

elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")


	set(WGPU_RUNTIME_LIB ${WGPU}/linux-${ARCH}/libwgpu.so)
	set_target_properties(
		webgpu
		PROPERTIES
			IMPORTED_LOCATION "${WGPU_RUNTIME_LIB}"
			INTERFACE_INCLUDE_DIRECTORIES "${WGPU}"
	)

elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")

	set(WGPU_RUNTIME_LIB ${WGPU}/macos-${ARCH}/libwgpu.dylib)
	set_target_properties(
		webgpu
		PROPERTIES
			IMPORTED_LOCATION "${WGPU_RUNTIME_LIB}"
			INTERFACE_INCLUDE_DIRECTORIES "${WGPU}"
	)

else()

	message(FATAL_ERROR "Plateform not supported by this release of WebGPU. You may consider building it yourself from its source (see https://github.com/gfx-rs/wgpu-native)")

endif()

message(STATUS "Using WebGPU runtime from '${WGPU_RUNTIME_LIB}'")
set(WGPU_RUNTIME_LIB ${WGPU_RUNTIME_LIB} PARENT_SCOPE)

# The application's binary must find wgpu.dll or libwgpu.so at runtime,
# so we automatically copy it (it's called WGPU_RUNTIME_LIB in general)
# next to the binary.
function(target_copy_webgpu_binaries Target)
	add_custom_command(
		TARGET ${Target} POST_BUILD
		COMMAND
			${CMAKE_COMMAND} -E copy_if_different
			${WGPU_RUNTIME_LIB}
			$<TARGET_FILE_DIR:${Target}>
	)
endfunction()
