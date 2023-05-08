#pragma once

#include <glm/glm.hpp>

// From https://google.github.io/filament/Filament.html#importancesamplingfortheibl
glm::vec2 hammersley(uint32_t i, float numSamples);
