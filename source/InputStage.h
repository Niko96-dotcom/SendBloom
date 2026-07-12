#pragma once

#include <cmath>

namespace sendbloom
{

class InputStage
{
public:
    void prepare (double sampleRate) noexcept
    {
        clipHoldSamples = static_cast<int> (std::ceil (0.050 * sampleRate));
        clipHoldCounter = 0;
        clipHoldActive = false;
    }

    float processSample (float monoIn, float gainLinear) noexcept
    {
        auto x = monoIn * gainLinear;
        x = softClip (x);
        updateClipHold (x);
        return x;
    }

    bool isClipHoldActive() const noexcept { return clipHoldActive; }

    void reset() noexcept
    {
        clipHoldCounter = 0;
        clipHoldActive = false;
    }

private:
    static float softClip (float x) noexcept
    {
        constexpr auto ceiling = 0.707946f;
        constexpr auto softKneeStart = ceiling * 0.75f;
        constexpr auto softKneeWidth = ceiling - softKneeStart;
        const auto magnitude = std::abs (x);

        if (magnitude <= softKneeStart)
            return x;

        const auto shaped = softKneeStart
                          + softKneeWidth * std::tanh ((magnitude - softKneeStart)
                                                      / softKneeWidth);
        return std::copysign (shaped, x);
    }

    void updateClipHold (float x) noexcept
    {
        constexpr auto knee = 0.707946f;

        if (std::abs (x) >= knee * 0.99f)
        {
            clipHoldCounter = clipHoldSamples;
            clipHoldActive = true;
            return;
        }

        if (clipHoldCounter > 0)
        {
            --clipHoldCounter;
            clipHoldActive = clipHoldCounter > 0;
            return;
        }

        clipHoldActive = false;
    }

    int clipHoldSamples { 2400 };
    int clipHoldCounter { 0 };
    bool clipHoldActive { false };
};

} // namespace sendbloom
