#pragma once

#include <cmath>

namespace sendbloom
{

struct PlaceholderWetDirt
{
    static float process (float wet, float distnBlend) noexcept
    {
        const auto driven = std::tanh (wet * 3.0f);
        return wet + distnBlend * (driven - wet);
    }
};

} // namespace sendbloom
