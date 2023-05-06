#include "ibl-utils.h"

#include <glm/glm.hpp>

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
