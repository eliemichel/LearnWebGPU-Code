set(EMSCRIPTEN_ROOT_PATH "$ENV{EMSDK}/upstream/emscripten")
# Include the original Emscripten toolchain file
#include("$ENV{EMSDK}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake")
set(CMAKE_SYSTEM_NAME Emscripten)
set(CMAKE_SYSTEM_VERSION 1)

set(CMAKE_CROSSCOMPILING TRUE)
set_property(GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS FALSE)

# Advertise Emscripten as a 32-bit platform (as opposed to
# CMAKE_SYSTEM_PROCESSOR=x86_64 for 64-bit platform), since some projects (e.g.
# OpenCV) use this to detect bitness.
# Allow users to ovewrite this on the command line with -DEMSCRIPTEN_SYSTEM_PROCESSOR=arm.
if (NOT DEFINED EMSCRIPTEN_SYSTEM_PROCESSOR)
  set(EMSCRIPTEN_SYSTEM_PROCESSOR x86)
endif()
set(CMAKE_SYSTEM_PROCESSOR ${EMSCRIPTEN_SYSTEM_PROCESSOR})

# Tell CMake how it should instruct the compiler to generate multiple versions
# of an outputted .so library: e.g. "libfoo.so, libfoo.so.1, libfoo.so.1.4" etc.
# This feature is activated if a shared library project has the property
# SOVERSION defined.
set(CMAKE_SHARED_LIBRARY_SONAME_C_FLAG "-Wl,-soname,")

# In CMake, CMAKE_HOST_WIN32 is set when we are cross-compiling from Win32 to
# Emscripten:
# http://www.cmake.org/cmake/help/v2.8.12/cmake.html#variable:CMAKE_HOST_WIN32
# The variable WIN32 is set only when the target arch that will run the code
# will be WIN32, so unset WIN32 when cross-compiling.
set(WIN32)

# The same logic as above applies for APPLE and CMAKE_HOST_APPLE, so unset
# APPLE.
set(APPLE)

# And for UNIX and CMAKE_HOST_UNIX. However, Emscripten is often able to mimic
# being a Linux/Unix system, in which case a lot of existing CMakeLists.txt
# files can be configured for Emscripten while assuming UNIX build, so this is
# left enabled.
set(UNIX 1)

# Do a no-op access on the CMAKE_TOOLCHAIN_FILE variable so that CMake will not
# issue a warning on it being unused.
if (CMAKE_TOOLCHAIN_FILE)
endif()

# Normalize, convert Windows backslashes to forward slashes or CMake will crash.
get_filename_component(EMSCRIPTEN_ROOT_PATH "${EMSCRIPTEN_ROOT_PATH}" ABSOLUTE)

list(APPEND CMAKE_MODULE_PATH "${EMSCRIPTEN_ROOT_PATH}/cmake/Modules")

if (CMAKE_HOST_WIN32)
  set(EMCC_SUFFIX ".bat")
else()
  set(EMCC_SUFFIX "")
endif()

# Specify the compilers to use for C and C++
set(CMAKE_C_COMPILER "${EMSCRIPTEN_ROOT_PATH}/emcc${EMCC_SUFFIX}")
set(CMAKE_CXX_COMPILER "${EMSCRIPTEN_ROOT_PATH}/em++${EMCC_SUFFIX}")

# Then override the value of CMAKE_CXX_COMPILER_CLANG_SCAN_DEPS
set(CMAKE_CXX_COMPILER_CLANG_SCAN_DEPS "${CMAKE_CURRENT_LIST_FILE}/emscan-deps" CACHE FILEPATH "Custom clang-scan-deps path" FORCE)
set(CMAKE_CXX_COMPILER_CLANG_SCAN_DEPS "${CMAKE_CURRENT_LIST_FILE}/emscan-deps")
set(CMAKE_C_COMPILER_CLANG_SCAN_DEPS "${CMAKE_CURRENT_LIST_FILE}/emscan-deps" CACHE FILEPATH "Custom clang-scan-deps path" FORCE)
set(CMAKE_C_COMPILER_CLANG_SCAN_DEPS "${CMAKE_CURRENT_LIST_FILE}/emscan-deps")
