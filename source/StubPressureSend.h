#pragma once

namespace sendbloom
{

struct StubPressureSend
{
    static float process (float input, float sendGain) noexcept
    {
        return input * sendGain;
    }
};

} // namespace sendbloom
