// Define this in **exactly one** file
#define WEBGPU_CPP_IMPLEMENTATION
// Then include this anywhere you need the wrapper
#include <webgpu/webgpu.hpp>
// Make sure that subsequent includes do not also define the implementation
#undef WEBGPU_CPP_IMPLEMENTATION