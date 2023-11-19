#include "webgpu-utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

// Define the UserData structure
struct UserData {
    WGPUAdapter adapter;
    bool requestEnded;
    WGPUDevice device;
};

// Define the callback function for adapter requests
void onAdapterRequestEnded(WGPURequestAdapterStatus status, WGPUAdapter adapter, const char *message, void *pUserData) {
    struct UserData *userData = (struct UserData *)pUserData;
    if (status == WGPURequestAdapterStatus_Success) {
        userData->adapter = adapter;
    } else {
        printf("Could not get WebGPU adapter: %s\n", message);
    }
    userData->requestEnded = true;
}

// Define the callback function for device requests
void onDeviceRequestEnded(WGPURequestDeviceStatus status, WGPUDevice device, const char *message, void *pUserData) {
    struct UserData *userData = (struct UserData *)pUserData;
    if (status == WGPURequestDeviceStatus_Success) {
        userData->device = device;
    } else {
        printf("Could not get WebGPU device: %s\n", message);
    }
    userData->requestEnded = true;
}

// Function to request a WebGPU adapter
WGPUAdapter requestAdapter(WGPUInstance instance, WGPURequestAdapterOptions const *options) {
    struct UserData userData = {NULL, false};

    // Call to the WebGPU request adapter procedure
    wgpuInstanceRequestAdapter(instance, options, onAdapterRequestEnded, (void *)&userData);

    // Wait until the request is complete
    while (!userData.requestEnded)
        ;

    return userData.adapter;
}

// Function to request a WebGPU device
WGPUDevice requestDevice(WGPUAdapter adapter, WGPUDeviceDescriptor const *descriptor) {
    struct UserData userData = {NULL, false};

    // Call to the WebGPU adapter request device procedure
    wgpuAdapterRequestDevice(adapter, descriptor, onDeviceRequestEnded, (void *)&userData);

    // Wait until the request is complete
    while (!userData.requestEnded)
        ;

    return userData.device;
}

// Function to inspect adapter properties, features, and limits
void inspectAdapter(WGPUAdapter adapter) {
    printf("Adapter properties:\n");
    WGPUAdapterProperties properties = {};
    wgpuAdapterGetProperties(adapter, &properties);
    printf(" - vendorID: %u\n", properties.vendorID);
    printf(" - deviceID: %u\n", properties.deviceID);
    printf(" - name: %s\n", properties.name);
    if (properties.driverDescription) {
        printf(" - driverDescription: %s\n", properties.driverDescription);
    }
    printf(" - adapterType: %u\n", properties.adapterType);
    printf(" - backendType: %u\n", properties.backendType);

    printf("Adapter features:\n");
    size_t featureCount = wgpuAdapterEnumerateFeatures(adapter, NULL);
    WGPUFeatureName *features = (WGPUFeatureName *)malloc(featureCount * sizeof(WGPUFeatureName));
    wgpuAdapterEnumerateFeatures(adapter, features);
    for (size_t i = 0; i < featureCount; ++i) {
        printf(" - %u\n", features[i]);
    }
    free(features);

    printf("Adapter limits:\n");
    WGPUSupportedLimits limits = {};
    wgpuAdapterGetLimits(adapter, &limits);
    printf(" - maxTextureDimension1D: %u\n", limits.limits.maxTextureDimension1D);
    printf(" - maxTextureDimension2D: %u\n", limits.limits.maxTextureDimension2D);
    printf(" - maxTextureDimension3D: %u\n", limits.limits.maxTextureDimension3D);
    printf(" - maxTextureArrayLayers: %u\n", limits.limits.maxTextureArrayLayers);
    printf(" - maxBindGroups: %u\n", limits.limits.maxBindGroups);
    printf(" - maxDynamicUniformBuffersPerPipelineLayout: %u\n", limits.limits.maxDynamicUniformBuffersPerPipelineLayout);
    printf(" - maxDynamicStorageBuffersPerPipelineLayout: %u\n", limits.limits.maxDynamicStorageBuffersPerPipelineLayout);
    printf(" - maxSampledTexturesPerShaderStage: %u\n", limits.limits.maxSampledTexturesPerShaderStage);
    printf(" - maxSamplersPerShaderStage: %u\n", limits.limits.maxSamplersPerShaderStage);
    printf(" - maxStorageBuffersPerShaderStage: %u\n", limits.limits.maxStorageBuffersPerShaderStage);
    printf(" - maxStorageTexturesPerShaderStage: %u\n", limits.limits.maxStorageTexturesPerShaderStage);
    printf(" - maxUniformBuffersPerShaderStage: %u\n", limits.limits.maxUniformBuffersPerShaderStage);
    printf(" - maxUniformBufferBindingSize: %u\n", limits.limits.maxUniformBufferBindingSize);
    printf(" - maxStorageBufferBindingSize: %u\n", limits.limits.maxStorageBufferBindingSize);
    printf(" - minUniformBufferOffsetAlignment: %u\n", limits.limits.minUniformBufferOffsetAlignment);
    printf(" - minStorageBufferOffsetAlignment: %u\n", limits.limits.minStorageBufferOffsetAlignment);
    printf(" - maxVertexBuffers: %u\n", limits.limits.maxVertexBuffers);
    printf(" - maxVertexAttributes: %u\n", limits.limits.maxVertexAttributes);
    printf(" - maxVertexBufferArrayStride: %u\n", limits.limits.maxVertexBufferArrayStride);
    printf(" - maxInterStageShaderComponents: %u\n", limits.limits.maxInterStageShaderComponents);
    printf(" - maxComputeWorkgroupStorageSize: %u\n", limits.limits.maxComputeWorkgroupStorageSize);
    printf(" - maxComputeInvocationsPerWorkgroup: %u\n", limits.limits.maxComputeInvocationsPerWorkgroup);
    printf(" - maxComputeWorkgroupSizeX: %u\n", limits.limits.maxComputeWorkgroupSizeX);
    printf(" - maxComputeWorkgroupSizeY: %u\n", limits.limits.maxComputeWorkgroupSizeY);
    printf(" - maxComputeWorkgroupSizeZ: %u\n", limits.limits.maxComputeWorkgroupSizeZ);
    printf(" - maxComputeWorkgroupsPerDimension: %u\n", limits.limits.maxComputeWorkgroupsPerDimension);
}

// Function to inspect device properties, features, and limits
void inspectDevice(WGPUDevice device) {
    printf("Device features:\n");
    size_t featureCount = wgpuDeviceEnumerateFeatures(device, NULL);
    WGPUFeatureName *features = (WGPUFeatureName *)malloc(featureCount * sizeof(WGPUFeatureName));
    wgpuDeviceEnumerateFeatures(device, features);
    for (size_t i = 0; i < featureCount; ++i) {
        printf(" - %u\n", features[i]);
    }
    free(features);

    printf("Device limits:\n");
    WGPUSupportedLimits limits = {};
    wgpuDeviceGetLimits(device, &limits);
    printf(" - maxTextureDimension1D: %u\n", limits.limits.maxTextureDimension1D);
    printf(" - maxTextureDimension2D: %u\n", limits.limits.maxTextureDimension2D);
    printf(" - maxTextureDimension3D: %u\n", limits.limits.maxTextureDimension3D);
    printf(" - maxTextureArrayLayers: %u\n", limits.limits.maxTextureArrayLayers);
    printf(" - maxBindGroups: %u\n", limits.limits.maxBindGroups);
    printf(" - maxDynamicUniformBuffersPerPipelineLayout: %u\n", limits.limits.maxDynamicUniformBuffersPerPipelineLayout);
    printf(" - maxDynamicStorageBuffersPerPipelineLayout: %u\n", limits.limits.maxDynamicStorageBuffersPerPipelineLayout);
    printf(" - maxSampledTexturesPerShaderStage: %u\n", limits.limits.maxSampledTexturesPerShaderStage);
    printf(" - maxSamplersPerShaderStage: %u\n", limits.limits.maxSamplersPerShaderStage);
    printf(" - maxStorageBuffersPerShaderStage: %u\n", limits.limits.maxStorageBuffersPerShaderStage);
    printf(" - maxStorageTexturesPerShaderStage: %u\n", limits.limits.maxStorageTexturesPerShaderStage);
    printf(" - maxUniformBuffersPerShaderStage: %u\n", limits.limits.maxUniformBuffersPerShaderStage);
    printf(" - maxUniformBufferBindingSize: %u\n", limits.limits.maxUniformBufferBindingSize);
    printf(" - maxStorageBufferBindingSize: %u\n", limits.limits.maxStorageBufferBindingSize);
    printf(" - minUniformBufferOffsetAlignment: %u\n", limits.limits.minUniformBufferOffsetAlignment);
    printf(" - minStorageBufferOffsetAlignment: %u\n", limits.limits.minStorageBufferOffsetAlignment);
    printf(" - maxVertexBuffers: %u\n", limits.limits.maxVertexBuffers);
    printf(" - maxVertexAttributes: %u\n", limits.limits.maxVertexAttributes);
    printf(" - maxVertexBufferArrayStride: %u\n", limits.limits.maxVertexBufferArrayStride);
    printf(" - maxInterStageShaderComponents: %u\n", limits.limits.maxInterStageShaderComponents);
    printf(" - maxComputeWorkgroupStorageSize: %u\n", limits.limits.maxComputeWorkgroupStorageSize);
    printf(" - maxComputeInvocationsPerWorkgroup: %u\n", limits.limits.maxComputeInvocationsPerWorkgroup);
    printf(" - maxComputeWorkgroupSizeX: %u\n", limits.limits.maxComputeWorkgroupSizeX);
    printf(" - maxComputeWorkgroupSizeY: %u\n", limits.limits.maxComputeWorkgroupSizeY);
    printf(" - maxComputeWorkgroupSizeZ: %u\n", limits.limits.maxComputeWorkgroupSizeZ);
    printf(" - maxComputeWorkgroupsPerDimension: %u\n", limits.limits.maxComputeWorkgroupsPerDimension);
}

