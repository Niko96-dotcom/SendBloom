#pragma once

#include <cmath>

namespace sendbloom
{

struct WetOverdrive
{
    static constexpr float kDrive = 3.0f;
    static constexpr float kAsymPos = 1.12f;

    static float asymmetricTanh (float x) noexcept
    {
        auto scaled = x * kDrive;

        if (scaled > 0.0f)
            scaled *= kAsymPos;

        return std::tanh (scaled) / std::tanh (kDrive);
    }

    static float process (float wet, float distnBlend) noexcept
    {
        const auto driven = asymmetricTanh (wet);
        return wet + distnBlend * (driven - wet);
    }
};

} // namespace sendbloom
