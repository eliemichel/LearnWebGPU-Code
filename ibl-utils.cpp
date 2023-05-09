#include "ibl-utils.h"
#include "filament/Image.h"

#include "float16_t.hpp"
#include <glm/glm.hpp>

#include <fstream>
#include <vector>
#include <array>

// Most of this file comes from
// https://github.com/google/filament/blob/main/libs/ibl/src/CubemapIBL.cpp
// and
// https://github.com/google/filament/blob/main/libs/ibl/src/CubemapSH.cpp

constexpr float PI = 3.14159265359f;
constexpr const double F_E = 2.71828182845904523536028747135266250;
constexpr const double F_LOG2E = 1.44269504088896340735992468100189214;
constexpr const double F_LOG10E = 0.434294481903251827651128918916605082;
constexpr const double F_LN2 = 0.693147180559945309417232121458176568;
constexpr const double F_LN10 = 2.30258509299404568401799145468436421;
constexpr const double F_PI = 3.14159265358979323846264338327950288;
constexpr const double F_PI_2 = 1.57079632679489661923132169163975144;
constexpr const double F_PI_4 = 0.785398163397448309615660845819875721;
constexpr const double F_1_PI = 0.318309886183790671537767526745028724;
constexpr const double F_2_PI = 0.636619772367581343075535053490057448;
constexpr const double F_2_SQRTPI = 1.12837916709551257389615890312154517;
constexpr const double F_SQRT2 = 1.41421356237309504880168872420969808;
constexpr const double F_SQRT1_2 = 0.707106781186547524400844362104849039;
constexpr const double F_TAU = 2.0 * F_PI;

using namespace ibl_utils;
using glm::vec2;
using glm::vec3;
using ssize_t = signed long long;
using filament::ibl::Image;

/* ****************** DFG ****************** */

inline vec2 hammersley(uint32_t i, float iN) {
    constexpr float tof = 0.5f / 0x80000000U;
    uint32_t bits = i;
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return { i * iN, bits * tof };
}

static vec3 hemisphereImportanceSampleDggx(vec2 u, float a) { // pdf = D(a) * cosTheta
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

static vec2 DFV(float NoV, float linearRoughness, size_t numSamples) {
    vec2 r = { 0, 0 };
    const glm::vec3 V(std::sqrt(1 - NoV * NoV), 0, NoV);
    for (size_t i = 0; i < numSamples; i++) {
        const vec2 u = hammersley(uint32_t(i), 1.0f / numSamples);
        const vec3 H = hemisphereImportanceSampleDggx(u, linearRoughness);
        const vec3 L = 2 * dot(V, H) * H - V;
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

/* ****************** SPHERICAL HARMONICS ****************** */

template<typename STATE>
using ScanlineProc = std::function<
    void(STATE& state, size_t y, Cubemap::Face f, Cubemap::Texel* data, size_t width)>;

template<typename STATE>
using ReduceProc = std::function<void(STATE& state)>;

template<typename STATE>
void cubemapForEach(
    const Cubemap& cm,
    ScanlineProc<STATE> proc,
    ReduceProc<STATE> reduce,
    const STATE& prototype) {

    const size_t dim = cm.getDimensions();

    STATE s;
    s = prototype;

    for (size_t faceIndex = 0; faceIndex < 6; faceIndex++) {
        const Cubemap::Face f = (Cubemap::Face)faceIndex;
        const Image& image(cm.getImageForFace(f));
        for (size_t y = 0; y < dim; y++) {
            Cubemap::Texel* data = static_cast<Cubemap::Texel*>(image.getPixelRef(0, y));
            proc(s, y, f, data, dim);
        }
    }
    reduce(s);
}


static constexpr float factorial(size_t n, size_t d = 1) {
    d = std::max(size_t(1), d);
    n = std::max(size_t(1), n);
    float r = 1.0;
    if (n == d) {
        // intentionally left blank
    }
    else if (n > d) {
        for (; n > d; n--) {
            r *= n;
        }
    }
    else {
        for (; d > n; d--) {
            r *= d;
        }
        r = 1.0f / r;
    }
    return r;
}

static inline constexpr size_t SHindex(ssize_t m, size_t l) {
    return l * (l + 1) + m;
}

void computeShBasis(
    float* SHb,
    size_t numBands,
    const vec3& s)
{
#if 0
    // Reference implementation
    float phi = atan2(s.x, s.y);
    for (size_t l = 0; l < numBands; l++) {
        SHb[SHindex(0, l)] = Legendre(l, 0, s.z);
        for (size_t m = 1; m <= l; m++) {
            float p = Legendre(l, m, s.z);
            SHb[SHindex(-m, l)] = std::sin(m * phi) * p;
            SHb[SHindex(m, l)] = std::cos(m * phi) * p;
        }
    }
#endif

    /*
     * TODO: all the Legendre computation below is identical for all faces, so it
     * might make sense to pre-compute it once. Also note that there is
     * a fair amount of symmetry within a face (which we could take advantage of
     * to reduce the pre-compute table).
     */

     /*
      * Below, we compute the associated Legendre polynomials using recursion.
      * see: http://mathworld.wolfram.com/AssociatedLegendrePolynomial.html
      *
      * Note [0]: s.z == cos(theta) ==> we only need to compute P(s.z)
      *
      * Note [1]: We in fact compute P(s.z) / sin(theta)^|m|, by removing
      * the "sqrt(1 - s.z*s.z)" [i.e.: sin(theta)] factor from the recursion.
      * This is later corrected in the ( cos(m*phi), sin(m*phi) ) recursion.
      */

      // s = (x, y, z) = (sin(theta)*cos(phi), sin(theta)*sin(phi), cos(theta))

      // handle m=0 separately, since it produces only one coefficient
    float Pml_2 = 0;
    float Pml_1 = 1;
    SHb[0] = Pml_1;
    for (size_t l = 1; l < numBands; l++) {
        float Pml = ((2 * l - 1.0f) * Pml_1 * s.z - (l - 1.0f) * Pml_2) / l;
        Pml_2 = Pml_1;
        Pml_1 = Pml;
        SHb[SHindex(0, l)] = Pml;
    }
    float Pmm = 1;
    for (ssize_t m = 1; m < (ssize_t)numBands; m++) {
        Pmm = (1.0f - 2 * m) * Pmm;      // See [1], divide by sqrt(1 - s.z*s.z);
        Pml_2 = Pmm;
        Pml_1 = (2 * m + 1.0f) * Pmm * s.z;
        // l == m
        SHb[SHindex(-m, m)] = Pml_2;
        SHb[SHindex(m, m)] = Pml_2;
        if (m + 1 < (ssize_t)numBands) {
            // l == m+1
            SHb[SHindex(-m, m + 1)] = Pml_1;
            SHb[SHindex(m, m + 1)] = Pml_1;
            for (size_t l = m + 2; l < numBands; l++) {
                float Pml = ((2 * l - 1.0f) * Pml_1 * s.z - (l + m - 1.0f) * Pml_2) / (l - m);
                Pml_2 = Pml_1;
                Pml_1 = Pml;
                SHb[SHindex(-m, l)] = Pml;
                SHb[SHindex(m, l)] = Pml;
            }
        }
    }

    // At this point, SHb contains the associated Legendre polynomials divided
    // by sin(theta)^|m|. Below we compute the SH basis.
    //
    // ( cos(m*phi), sin(m*phi) ) recursion:
    // cos(m*phi + phi) == cos(m*phi)*cos(phi) - sin(m*phi)*sin(phi)
    // sin(m*phi + phi) == sin(m*phi)*cos(phi) + cos(m*phi)*sin(phi)
    // cos[m+1] == cos[m]*s.x - sin[m]*s.y
    // sin[m+1] == sin[m]*s.x + cos[m]*s.y
    //
    // Note that (d.x, d.y) == (cos(phi), sin(phi)) * sin(theta), so the
    // code below actually evaluates:
    //      (cos((m*phi), sin(m*phi)) * sin(theta)^|m|
    float Cm = s.x;
    float Sm = s.y;
    for (ssize_t m = 1; m <= (ssize_t)numBands; m++) {
        for (size_t l = m; l < numBands; l++) {
            SHb[SHindex(-m, l)] *= Sm;
            SHb[SHindex(m, l)] *= Cm;
        }
        float Cm1 = Cm * s.x - Sm * s.y;
        float Sm1 = Sm * s.x + Cm * s.y;
        Cm = Cm1;
        Sm = Sm1;
    }
}

/*
 * SH scaling factors:
 *  returns sqrt((2*l + 1) / 4*pi) * sqrt( (l-|m|)! / (l+|m|)! )
 */
float Kml(ssize_t m, size_t l) {
    m = m < 0 ? -m : m;  // abs() is not constexpr
    const float K = (2 * l + 1) * factorial(size_t(l - m), size_t(l + m));
    return std::sqrt(K) * (float)(F_2_SQRTPI * 0.25);
}

std::vector<float> Ki(size_t numBands) {
    const size_t numCoefs = numBands * numBands;
    std::vector<float> K(numCoefs);
    for (size_t l = 0; l < numBands; l++) {
        K[SHindex(0, l)] = Kml(0, l);
        for (ssize_t m = 1; m <= (ssize_t)l; m++) {
            K[SHindex(m, l)] =
                K[SHindex(-m, l)] = (float)F_SQRT2 * Kml(m, l);
        }
    }
    return K;
}

constexpr float computeTruncatedCosSh(size_t l) {
    if (l == 0) {
        return (float)F_PI;
    }
    else if (l == 1) {
        return 2 * (float)F_PI / 3;
    }
    else if (l & 1u) {
        return 0;
    }
    const size_t l_2 = l / 2;
    float A0 = ((l_2 & 1u) ? 1.0f : -1.0f) / ((l + 2) * (l - 1));
    float A1 = factorial(l, l_2) / (factorial(l_2) * (1 << l));
    return 2 * (float)F_PI * A0 * A1;
}

static inline float sphereQuadrantArea(float x, float y) {
    return std::atan2(x * y, std::sqrt(x * x + y * y + 1));
}

float cubemapSolidAngle(size_t dim, size_t u, size_t v) {
    const float iDim = 1.0f / dim;
    float s = ((u + 0.5f) * 2 * iDim) - 1;
    float t = ((v + 0.5f) * 2 * iDim) - 1;
    const float x0 = s - iDim;
    const float y0 = t - iDim;
    const float x1 = s + iDim;
    const float y1 = t + iDim;
    float solidAngle = sphereQuadrantArea(x0, y0) -
        sphereQuadrantArea(x0, y1) -
        sphereQuadrantArea(x1, y0) +
        sphereQuadrantArea(x1, y1);
    return solidAngle;
}

static SphericalHarmonics computeRawSH(const Cubemap& cm, size_t numBands, bool irradiance) {

    const size_t numCoefs = numBands * numBands;
    SphericalHarmonics SH(numCoefs, vec3(0.0));

    struct State {
        State() = default;
        explicit State(size_t numCoefs) : numCoefs(numCoefs) { }

        State& operator=(State const& rhs) {
            numCoefs = rhs.numCoefs;
            SH.resize(numCoefs);
            std::fill(SH.begin(), SH.end(), vec3(0.0f));
            SHb.resize(numCoefs);
            std::fill(SHb.begin(), SHb.end(), 0.0f);
            return *this;
        }
        size_t numCoefs = 0;
        SphericalHarmonics SH;
        std::vector<float> SHb;
    } prototype(numCoefs);

    cubemapForEach<State>(cm, [&](State& state, size_t y, Cubemap::Face f, Cubemap::Texel const* data, size_t dim) {
            for (size_t x = 0; x < dim; ++x, ++data) {

                vec3 s(cm.getDirectionFor(f, x, y));

                // sample a color
                vec3 color(Cubemap::sampleAt(data));

                // take solid angle into account
                color *= cubemapSolidAngle(dim, x, y);

                computeShBasis(state.SHb.data(), numBands, s);

                // apply coefficients to the sampled color
                for (size_t i = 0; i < numCoefs; i++) {
                    state.SH[i] += color * state.SHb[i];
                }
            }
        },
        [&](State& state) {
            for (size_t i = 0; i < numCoefs; i++) {
                SH[i] += state.SH[i];
            }
        }, prototype);

    // precompute the scaling factor K
    std::vector<float> K = Ki(numBands);

    // apply truncated cos (irradiance)
    if (irradiance) {
        for (size_t l = 0; l < numBands; l++) {
            const float truncatedCosSh = computeTruncatedCosSh(size_t(l));
            K[SHindex(0, l)] *= truncatedCosSh;
            for (ssize_t m = 1; m <= (ssize_t)l; m++) {
                K[SHindex(-m, l)] *= truncatedCosSh;
                K[SHindex(m, l)] *= truncatedCosSh;
            }
        }
    }

    // apply all the scale factors
    for (size_t i = 0; i < numCoefs; i++) {
        SH[i] *= K[i];
    }
    return SH;
}


static void preprocessSHForShader(SphericalHarmonics& SH) {
    constexpr size_t numBands = 3;
    constexpr size_t numCoefs = numBands * numBands;
    assert(SH.size() == numCoefs);

    // Coefficient for the polynomial form of the SH functions -- these were taken from
    // "Stupid Spherical Harmonics (SH)" by Peter-Pike Sloan
    // They simply come for expanding the computation of each SH function.
    //
    // To render spherical harmonics we can use the polynomial form, like this:
    //          c += sh[0] * A[0];
    //          c += sh[1] * A[1] * s.y;
    //          c += sh[2] * A[2] * s.z;
    //          c += sh[3] * A[3] * s.x;
    //          c += sh[4] * A[4] * s.y * s.x;
    //          c += sh[5] * A[5] * s.y * s.z;
    //          c += sh[6] * A[6] * (3 * s.z * s.z - 1);
    //          c += sh[7] * A[7] * s.z * s.x;
    //          c += sh[8] * A[8] * (s.x * s.x - s.y * s.y);
    //
    // To save math in the shader, we pre-multiply our SH coefficient by the A[i] factors.
    // Additionally, we include the lambertian diffuse BRDF 1/pi.

    constexpr float M_SQRT_PI = 1.7724538509f;
    constexpr float M_SQRT_3 = 1.7320508076f;
    constexpr float M_SQRT_5 = 2.2360679775f;
    constexpr float M_SQRT_15 = 3.8729833462f;
    constexpr float A[numCoefs] = {
                  1.0f / (2.0f * M_SQRT_PI),    // 0  0
            -M_SQRT_3 / (2.0f * M_SQRT_PI),    // 1 -1
             M_SQRT_3 / (2.0f * M_SQRT_PI),    // 1  0
            -M_SQRT_3 / (2.0f * M_SQRT_PI),    // 1  1
             M_SQRT_15 / (2.0f * M_SQRT_PI),    // 2 -2
            -M_SQRT_15 / (2.0f * M_SQRT_PI),    // 3 -1
             M_SQRT_5 / (4.0f * M_SQRT_PI),    // 3  0
            -M_SQRT_15 / (2.0f * M_SQRT_PI),    // 3  1
             M_SQRT_15 / (4.0f * M_SQRT_PI)     // 3  2
    };

    for (size_t i = 0; i < numCoefs; i++) {
        SH[i] *= A[i] * F_1_PI;
    }
}

/* ****************** PUBLIC API ****************** */

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
                vec3 r = { DFV(NoV, linear_roughness, 1024), 0.0f };

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

SphericalHarmonics computeSH(const Cubemap& cm) {
    SphericalHarmonics SH = computeRawSH(cm, 3 /* bands */, true /* irradiance */);
    preprocessSHForShader(SH);
    return SH;
}

} // namespace ibl_utils
