# This is only meant to be included as a subdirectory in another project.
# It assumes that targets 'SDL2' and 'webgpu' exist.
# Loot at examples/CMakeLists.txt to see how to use it in a project.

# The sdl2webgpu target
add_library(sdl2webgpu STATIC sdl2webgpu.c)
target_include_directories(sdl2webgpu PUBLIC .)
target_link_libraries(sdl2webgpu PUBLIC SDL2::SDL2 webgpu)

if (APPLE)
  target_compile_options(sdl2webgpu PRIVATE -x objective-c)
  target_link_libraries(sdl2webgpu PRIVATE "-framework Cocoa" "-framework CoreVideo" "-framework IOKit" "-framework QuartzCore")
endif ()
