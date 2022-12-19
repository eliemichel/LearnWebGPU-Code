set(WGPU ${CMAKE_CURRENT_SOURCE_DIR})

# A pre-compiled target (IMPORTED) that is a dynamically
# linked library (SHARED, meaning .dll or .so).
add_library(webgpu SHARED IMPORTED GLOBAL)

if (MSVC)

	set_target_properties(
		webgpu
		PROPERTIES
			IMPORTED_LOCATION ${WGPU}/wgpu_native.dll
			IMPORTED_IMPLIB ${WGPU}/wgpu_native.lib
			INTERFACE_INCLUDE_DIRECTORIES ${WGPU}
	)
	set(WGPU_RUNTIME_LIB ${WGPU}/wgpu_native.dll PARENT_SCOPE)

else()

	set_target_properties(
		webgpu
		PROPERTIES
			IMPORTED_LOCATION ${WGPU}/libwgpu.so
			INTERFACE_INCLUDE_DIRECTORIES ${WGPU}
	)
	set(WGPU_RUNTIME_LIB ${WGPU}/libwgpu.so PARENT_SCOPE)

endif()

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
