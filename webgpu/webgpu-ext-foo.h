#ifndef WEBGPU_EXT_FOO_H_
#define WEBGPU_EXT_FOO_H_

#include <webgpu/webgpu.h>

// Our extra structure types
typedef enum WGPUFooSType {
	// Our new extension, for instance to extend the Render Pipeline
	WGPUFooSType_FooRenderPipelineDescriptor = 0x00001001,

	// Force the enum value to be represented on 32-bit integers
	WGPUFooSType_Force32 = 0x7FFFFFFF
} WGPUFooSType; // (set enum name here as well)

// Our extra feature names
typedef enum WGPUFooFeatureName {
	// Our new feature name
	WGPUFeatureName_Foo = 0x00001001,

	// Force the enum value to be represented on 32-bit integers
	WGPUFooFeatureName_Force32 = 0x7FFFFFFF
} WGPUFooFeatureName; // (set enum name here as well)

// Our extra structures
typedef struct WGPUFooRenderPipelineDescriptor {
	// This first field is conventionally called 'chain'
	WGPUChainedStruct chain;

	// Your custom fields here
	uint32_t foo;
} WGPUFooRenderPipelineDescriptor;

#endif // WEBGPU_EXT_FOO_H_
