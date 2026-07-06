#pragma once

namespace sendbloom
{

class OutputStage
{
public:
    static float processSample (float sample, float gainLinear) noexcept
    {
        return sample * gainLinear;
    }
};

} // namespace sendbloom
