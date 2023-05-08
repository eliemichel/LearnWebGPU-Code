#include "ibl-utils.h"

#include "float16_t.hpp"
#include <glm/glm.hpp>

#include <fstream>

// Most of this comes from https://github.com/google/filament/blob/main/libs/ibl/src/CubemapIBL.cpp

constexpr float PI = 3.14159265359f;

#if 0

float GDFG(float NoV, float NoL, float a) {
    float a2 = a * a;
    float GGXL = NoV * std::sqrt((-NoL * a2 + NoL) * NoL + a2);
    float GGXV = NoL * std::sqrt((-NoV * a2 + NoV) * NoV + a2);
    return (2 * NoL) / (GGXV + GGXL);
}

glm::vec2 DFG(float NoV, float a, int sampleCount) {
    glm::vec3 V;
    V.x = std::sqrt(1.0f - NoV * NoV);
    V.y = 0.0f;
    V.z = NoV;

    glm::vec2 r = 0.0f;
    for (uint i = 0; i < sampleCount; i++) {
        glm::vec2 Xi = hammersley(i, sampleCount);
        glm::vec3 H = importanceSampleGGX(Xi, a, N);
        glm::vec3 L = 2.0f * dot(V, H) * H - V;

        float VoH = clamp(dot(V, H), 0.0f, 1.0f);
        float NoL = clamp(L.z, 0.0f, 1.0f);
        float NoH = clamp(H.z, 0.0f, 1.0f);

        if (NoL > 0.0f) {
            float G = GDFG(NoV, NoL, a);
            float Gv = G * VoH / NoH;
            float Fc = pow(1 - VoH, 5.0f);
            r.x += Gv * (1 - Fc);
            r.y += Gv * Fc;
        }
    }
    return r * (1.0f / sampleCount);
}

#endif

#include <vector>
#include <array>

class Image {
public:
    uint32_t getWidth() const { return m_strides[0]; }
    uint32_t getHeight() const { return m_strides[1]; }
    void setPixel(uint32_t x, uint32_t y, const glm::vec3& data) {
        uint32_t offset = x + y * m_strides[0];
        m_data[offset] = glm::vec4(data, 1.0);
    }

private:
    std::array<uint32_t, 2> m_strides;
    std::vector<glm::vec4> m_data;
};

inline glm::vec2 hammersley(uint32_t i, float iN) {
    constexpr float tof = 0.5f / 0x80000000U;
    uint32_t bits = i;
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return { i * iN, bits * tof };
}

static glm::vec3 hemisphereImportanceSampleDggx(glm::vec2 u, float a) { // pdf = D(a) * cosTheta
    const float phi = 2.0f * PI * u.x;
    // NOTE: (aa-1) == (a-1)(a+1) produces better fp accuracy
    const float cosTheta2 = (1 - u.y) / (1 + (a + 1) * ((a - 1) * u.y));
    const float cosTheta = std::sqrt(cosTheta2);
    const float sinTheta = std::sqrt(1 - cosTheta2);
    return { sinTheta * std::cos(phi), sinTheta * std::sin(phi), cosTheta };
}

static float pow5(float x) {
    const float x2 = x * x;
    return x2 * x2 * x;
}

static float Visibility(float NoV, float NoL, float a) {
    // Heitz 2014, "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs"
    // Height-correlated GGX
    const float a2 = a * a;
    const float GGXL = NoV * std::sqrt((NoL - NoL * a2) * NoL + a2);
    const float GGXV = NoL * std::sqrt((NoV - NoV * a2) * NoV + a2);
    return 0.5f / (GGXV + GGXL);
}

static glm::vec2 DFV(float NoV, float linearRoughness, size_t numSamples) {
    glm::vec2 r = { 0, 0 };
    const glm::vec3 V(std::sqrt(1 - NoV * NoV), 0, NoV);
    for (size_t i = 0; i < numSamples; i++) {
        const glm::vec2 u = hammersley(uint32_t(i), 1.0f / numSamples);
        const glm::vec3 H = hemisphereImportanceSampleDggx(u, linearRoughness);
        const glm::vec3 L = 2 * dot(V, H) * H - V;
        const float VoH = glm::clamp(dot(V, H), 0.0f, 1.0f);
        const float NoL = glm::clamp(L.z, 0.0f, 1.0f);
        const float NoH = glm::clamp(H.z, 0.0f, 1.0f);
        if (NoL > 0) {
            const float v = Visibility(NoV, NoL, linearRoughness) * NoL * (VoH / NoH);
            const float Fc = pow5(1 - VoH);
            r.x += v * (1.0f - Fc);
            r.y += v * Fc;
        }
    }
    return r * (4.0f / numSamples);
}

void DFG(Image& dst) {
    const uint32_t width = dst.getWidth();
    const uint32_t height = dst.getHeight();
    for (uint32_t y = 0; y < height; ++y) {
        const float coord = glm::clamp((height - y + 0.5f) / height, 0.0f, 1.0f);

        // map the coordinate in the texture to a linear_roughness,
        // here we're using ^2, but other mappings are possible.
        // ==> coord = sqrt(linear_roughness)
        const float linear_roughness = coord * coord;
        for (uint32_t x = 0; x < width; ++x) {
            // const float NoV = float(x) / (width-1);
            const float NoV = glm::clamp((x + 0.5f) / width, 0.0f, 1.0f);
            glm::vec3 r = { DFV(NoV, linear_roughness, 1024), 0.0f };

            dst.setPixel(x, y, r);
        }
    }
}

namespace ibl_utils {

using namespace wgpu;
using numeric::float16_t;

Texture createDFGTexture(Device device, uint32_t size, wgpu::TextureView* view) {
    // Create texture
    TextureDescriptor desc = Default;
    desc.dimension = TextureDimension::_2D;
    desc.label = "DFG LUT";
    desc.format = TextureFormat::RG16Float;
    desc.mipLevelCount = 1;
    desc.size = { size, size, 1 };
    desc.usage = TextureUsage::CopyDst | TextureUsage::TextureBinding;
    Texture texture = device.createTexture(desc);

    // Generate data
    std::vector<float16_t> data(2 * size * size);
    bool loaded = false;
    if (true) {
        std::ifstream ifs(RESOURCE_DIR "/DFG.bin", std::ios::binary);
        if (ifs.is_open()) {
            ifs.read(reinterpret_cast<char*>(data.data()), data.size() * sizeof(float16_t));
            ifs.close();
            loaded = true;
        }
    }
    if (!loaded) {
        for (uint32_t y = 0; y < size; ++y) {
            const float coord = glm::clamp((size - y + 0.5f) / size, 0.0f, 1.0f);

            // map the coordinate in the texture to a linear_roughness,
            // here we're using ^2, but other mappings are possible.
            // ==> coord = sqrt(linear_roughness)
            const float linear_roughness = coord * coord;
            for (uint32_t x = 0; x < size; ++x) {
                // const float NoV = float(x) / (width-1);
                const float NoV = glm::clamp((x + 0.5f) / size, 0.0f, 1.0f);
                glm::vec3 r = { DFV(NoV, linear_roughness, 1024), 0.0f };

                float16_t* pixel = &data[2 * (y * size + x)];
                pixel[0] = r.x;
                pixel[1] = r.y;
            }
        }
    }

    // Upload data
    Queue queue = device.getQueue();

    ImageCopyTexture dst;
    dst.aspect = TextureAspect::All;
    dst.mipLevel = 0;
    dst.origin = { 0,0,0 };
    dst.texture = texture;

    TextureDataLayout layout;
    layout.offset = 0;
    layout.bytesPerRow = size * 2 * sizeof(float16_t);
    layout.rowsPerImage = size;

    queue.writeTexture(dst, data.data(), data.size() * sizeof(float16_t), layout, desc.size);

#ifdef WEBGPU_BACKEND_DAWN
    wgpuQueueRelease(queue);
#endif

    if (view) {
        TextureViewDescriptor viewDesc;
        viewDesc.arrayLayerCount = 1;
        viewDesc.aspect = TextureAspect::All;
        viewDesc.baseArrayLayer = 0;
        viewDesc.baseMipLevel = 0;
        viewDesc.dimension = TextureViewDimension::_2D;
        viewDesc.format = desc.format;
        viewDesc.label = desc.label;
        viewDesc.mipLevelCount = desc.mipLevelCount;
        *view = texture.createView(viewDesc);
    }

    // DEBUG
    if (false) {
        std::ofstream f(RESOURCE_DIR "/DFG.bin", std::ios::binary);
        if (f.is_open()) {
            f.write(reinterpret_cast<const char*>(data.data()), data.size() * sizeof(float16_t));
            f.close();
        }
    }

    return texture;
}

} // namespace ibl_utils
