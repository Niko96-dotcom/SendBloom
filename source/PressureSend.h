#pragma once

#include "ParameterCurves.h"

namespace sendbloom
{

struct PressureSend
{
    static float computeGain (float amountNorm, bool sendConnected, bool firmFeel) noexcept
    {
        if (! sendConnected)
            return 1.0f;

        return ParameterCurves::sendGain (amountNorm, firmFeel);
    }

    static float process (float input, float sendGain) noexcept
    {
        return input * sendGain;
    }
};

} // namespace sendbloom
