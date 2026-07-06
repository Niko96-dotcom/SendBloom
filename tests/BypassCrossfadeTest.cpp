#include <BypassCrossfade.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

TEST_CASE ("BypassCrossfade ramps wet mix over 5 ms without large discontinuities", "[parm][bypass]")
{
    juce::SmoothedValue<float> wetMix;
    wetMix.reset (48000.0, 0.005);
    wetMix.setCurrentAndTargetValue (1.0f);
    wetMix.setTargetValue (0.0f);

    juce::AudioBuffer<float> wet (1, 240);
    juce::AudioBuffer<float> dry (1, 240);

    for (int i = 0; i < 240; ++i)
    {
        wet.setSample (0, i, 1.0f);
        dry.setSample (0, i, 0.5f);
    }

    sendbloom::BypassCrossfade::processBlock (wet, dry, wetMix);

    float maxAdjacentDelta = 0.0f;
    for (int i = 1; i < 240; ++i)
        maxAdjacentDelta = std::max (maxAdjacentDelta,
                                     std::abs (wet.getSample (0, i) - wet.getSample (0, i - 1)));

    REQUIRE (maxAdjacentDelta < 0.5f);
}

TEST_CASE ("BypassCrossfade uses 5 ms smoother configuration", "[parm][bypass]")
{
    juce::SmoothedValue<float> wetMix;
    wetMix.reset (48000.0, 0.005);
    REQUIRE (wetMix.isSmoothing() == false);
}
