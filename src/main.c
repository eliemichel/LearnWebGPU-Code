#define SDL_MAIN_HANDLED
#include <sdl2webgpu.h>
#include <SDL2/SDL.h>

#include "webgpu-utils.h"

#include <stdio.h>
#include <assert.h>

void errorCallback(WGPUErrorType type, char const* message, void* userdata) {
    printf("Device error: type %d", type);
    if (message) printf(" (message: %s)", message);
    printf("\n");
}

int main(int, char**) {
    WGPUInstanceDescriptor instanceDesc;
    instanceDesc.nextInChain = NULL;
    WGPUInstance instance = wgpuCreateInstance(&instanceDesc);
    if (!instance) {
        fprintf(stderr, "Could not initialize WebGPU!\n");
        return 1;
    }

    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Could not initialize SDL! Error: %s\n", SDL_GetError());
        return 1;
    }

    int windowFlags = 0;
    SDL_Window* window = SDL_CreateWindow("Learn WebGPU", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, windowFlags);
    if (!window) {
        fprintf(stderr, "Could not open window!\n");
        return 1;
    }

    printf("Requesting adapter...\n");
    WGPUSurface surface = SDL_GetWGPUSurface(instance, window);
    WGPURequestAdapterOptions adapterOpts;
    adapterOpts.nextInChain = NULL;
    adapterOpts.compatibleSurface = surface;
    WGPUAdapter adapter = requestAdapter(instance, &adapterOpts);
    printf("Got adapter: %lld\n", (long long)adapter);

    printf("Requesting device...\n");
    WGPUDeviceDescriptor deviceDesc;
    deviceDesc.nextInChain = NULL;
    deviceDesc.label = "My Device";
    deviceDesc.requiredFeaturesCount = 0;
    deviceDesc.requiredLimits = NULL;
    deviceDesc.defaultQueue.label = "The default queue";
    WGPUDevice device = requestDevice(adapter, &deviceDesc);
    printf("Got device: %lld\n", (long long)device);

    // Add an error callback for more debug info
    wgpuDeviceSetUncapturedErrorCallback(device, errorCallback, NULL);

    WGPUQueue queue = wgpuDeviceGetQueue(device);

    printf("Creating swapchain...\n");
#ifdef WEBGPU_BACKEND_WGPU
    WGPUTextureFormat swapChainFormat = wgpuSurfaceGetPreferredFormat(surface, adapter);
#else
    WGPUTextureFormat swapChainFormat = WGPUTextureFormat_BGRA8Unorm;
#endif
    WGPUSwapChainDescriptor swapChainDesc;
    swapChainDesc.nextInChain = NULL;
    swapChainDesc.width = 640;
    swapChainDesc.height = 480;
    swapChainDesc.usage = WGPUTextureUsage_RenderAttachment;
    swapChainDesc.format = swapChainFormat;
    swapChainDesc.presentMode = WGPUPresentMode_Fifo;
    WGPUSwapChain swapChain = wgpuDeviceCreateSwapChain(device, surface, &swapChainDesc);
    printf("Swapchain: %lld\n", (long long)swapChain);

    printf("Creating shader module...\n");
    const char* shaderSource = 
        "@vertex\n"
        "fn vs_main(@builtin(vertex_index) in_vertex_index: u32) -> @builtin(position) vec4<f32> {\n"
        "    var p = vec2f(0.0, 0.0);\n"
        "    if (in_vertex_index == 0u) {\n"
        "        p = vec2f(-0.5, -0.5);\n"
        "    } else if (in_vertex_index == 1u) {\n"
        "        p = vec2f(0.5, -0.5);\n"
        "    } else {\n"
        "        p = vec2f(0.0, 0.5);\n"
        "    }\n"
        "    return vec4f(p, 0.0, 1.0);\n"
        "}\n"
        "\n"
        "@fragment\n"
        "fn fs_main() -> @location(0) vec4f {\n"
        "    return vec4f(0.0, 0.4, 1.0, 1.0);\n"
        "}\n";

    WGPUShaderModuleDescriptor shaderDesc;
    shaderDesc.nextInChain = NULL;
#ifdef WEBGPU_BACKEND_WGPU
    shaderDesc.hintCount = 0;
    shaderDesc.hints = NULL;
#endif

    // Use the extension mechanism to load a WGSL shader source code
    WGPUShaderModuleWGSLDescriptor shaderCodeDesc;
    // Set the chained struct's header
    shaderCodeDesc.chain.next = NULL;
    shaderCodeDesc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
    // Connect the chain
    shaderDesc.nextInChain = &shaderCodeDesc.chain;

    // Setup the actual payload of the shader code descriptor
    shaderCodeDesc.code = shaderSource;

    WGPUShaderModule shaderModule = wgpuDeviceCreateShaderModule(device, &shaderDesc);
    printf("Shader module: %lld\n", (long long)shaderModule);

    printf("Creating render pipeline...\n");
    WGPURenderPipelineDescriptor pipelineDesc;
    pipelineDesc.nextInChain = NULL;

    // Vertex fetch
    // (We don't use any input buffer so far)
    pipelineDesc.vertex.bufferCount = 0;
    pipelineDesc.vertex.buffers = NULL;

    // Vertex shader
    pipelineDesc.vertex.module = shaderModule;
    pipelineDesc.vertex.entryPoint = "vs_main";
    pipelineDesc.vertex.constantCount = 0;
    pipelineDesc.vertex.constants = NULL;

    // Primitive assembly and rasterization
    // Each sequence of 3 vertices is considered as a triangle
    pipelineDesc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
    // We'll see later how to specify the order in which vertices should be
    // connected. When not specified, vertices are considered sequentially.
    pipelineDesc.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
    // The face orientation is defined by assuming that when looking
    // from the front of the face, its corner vertices are enumerated
    // in the counter-clockwise (CCW) order.
    pipelineDesc.primitive.frontFace = WGPUFrontFace_CCW;
    // But the face orientation does not matter much because we do not
    // cull (i.e. "hide") the faces pointing away from us (which is often
    // used for optimization).
    pipelineDesc.primitive.cullMode = WGPUCullMode_None;

    // Fragment shader
    WGPUFragmentState fragmentState;
    fragmentState.nextInChain = NULL;
    pipelineDesc.fragment = &fragmentState;
    fragmentState.module = shaderModule;
    fragmentState.entryPoint = "fs_main";
    fragmentState.constantCount = 0;
    fragmentState.constants = NULL;

    // Configure blend state
    WGPUBlendState blendState;
    // Usual alpha blending for the color:
    blendState.color.srcFactor = WGPUBlendFactor_SrcAlpha;
    blendState.color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
    blendState.color.operation = WGPUBlendOperation_Add;
    // We leave the target alpha untouched:
    blendState.alpha.srcFactor = WGPUBlendFactor_Zero;
    blendState.alpha.dstFactor = WGPUBlendFactor_One;
    blendState.alpha.operation = WGPUBlendOperation_Add;

    WGPUColorTargetState colorTarget;
    colorTarget.nextInChain = NULL;
    colorTarget.format = swapChainFormat;
    colorTarget.blend = &blendState;
    colorTarget.writeMask = WGPUColorWriteMask_All; // We could write to only some of the color channels.

    // We have only one target because our render pass has only one output color
    // attachment.
    fragmentState.targetCount = 1;
    fragmentState.targets = &colorTarget;

    // Depth and stencil tests are not used here
    pipelineDesc.depthStencil = NULL;

    // Multi-sampling
    // Samples per pixel
    pipelineDesc.multisample.count = 1;
    // Default value for the mask, meaning "all bits on"
    pipelineDesc.multisample.mask = ~0u;
    // Default value as well (irrelevant for count = 1 anyways)
    pipelineDesc.multisample.alphaToCoverageEnabled = false;

    // Pipeline layout
    pipelineDesc.layout = NULL;

    WGPURenderPipeline pipeline = wgpuDeviceCreateRenderPipeline(device, &pipelineDesc);
    printf("Render pipeline: %lld\n", (long long)pipeline);

    int shouldClose = 0;
    while (!shouldClose) {

        // Poll events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                shouldClose = 1;
                break;

            default:
                break;
            }
        }

        WGPUTextureView nextTexture = wgpuSwapChainGetCurrentTextureView(swapChain);
        if (!nextTexture) {
            fprintf(stderr, "Cannot acquire next swap chain texture\n");
            return 1;
        }

        WGPUCommandEncoderDescriptor commandEncoderDesc;
        commandEncoderDesc.nextInChain = NULL;
        commandEncoderDesc.label = "Command Encoder";
        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &commandEncoderDesc);

        WGPURenderPassDescriptor renderPassDesc;
        renderPassDesc.nextInChain = NULL;

        WGPURenderPassColorAttachment renderPassColorAttachment;
        renderPassColorAttachment.view = nextTexture;
        renderPassColorAttachment.resolveTarget = NULL;
        renderPassColorAttachment.loadOp = WGPULoadOp_Clear;
        renderPassColorAttachment.storeOp = WGPUStoreOp_Store;
        renderPassColorAttachment.clearValue = (WGPUColor){0.9, 0.1, 0.2, 1.0};
        renderPassDesc.colorAttachmentCount = 1;
        renderPassDesc.colorAttachments = &renderPassColorAttachment;

        renderPassDesc.depthStencilAttachment = NULL;
        renderPassDesc.timestampWriteCount = 0;
        renderPassDesc.timestampWrites = NULL;
        WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);

        // In its overall outline, drawing a triangle is as simple as this:
        // Select which render pipeline to use
        wgpuRenderPassEncoderSetPipeline(renderPass, pipeline);
        // Draw 1 instance of a 3-vertices shape
        wgpuRenderPassEncoderDraw(renderPass, 3, 1, 0, 0);

        wgpuRenderPassEncoderEnd(renderPass);

        wgpuTextureViewRelease(nextTexture);

        WGPUCommandBufferDescriptor cmdBufferDesc;
        cmdBufferDesc.nextInChain = NULL;
        cmdBufferDesc.label = "Command buffer";
        WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &cmdBufferDesc);
        wgpuQueueSubmit(queue, 1, &command);

        wgpuSwapChainPresent(swapChain);
    }

    wgpuSwapChainRelease(swapChain);
    wgpuDeviceRelease(device);
    wgpuAdapterRelease(adapter);
    wgpuInstanceRelease(instance);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
