#include "ChainTestHelpers.h"
#include <ParallelWetMixer.h>
#include <ParameterCurves.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

namespace
{

float legacyDualScaledEqualPowerSum (float dryTap, float wetSample, float levelNorm) noexcept
{
    // Pre-ADR-V1-09 dual-sided equal-power (for regression contrast only).
    constexpr auto halfPi = juce::MathConstants<float>::halfPi;
    const auto dryGain = std::sin (halfPi * (1.0f - levelNorm));
    const auto wetGain = std::sin (halfPi * levelNorm);
    return dryTap * dryGain + wetSample * wetGain;
}

} // namespace

TEST_CASE ("ParallelWetMixer dry tap unity at zero wet gain", "[chain][routing][ParallelWet]")
{
    REQUIRE (sendbloom::ParallelWetMixer::mix (0.25f, 0.0f, 0.0f) == Catch::Approx (0.25f));
}

TEST_CASE ("ParallelWetMixer wet scaling adds to dry tap", "[chain][routing][ParallelWet]")
{
    REQUIRE (sendbloom::ParallelWetMixer::mix (0.25f, 1.0f, 0.5f) == Catch::Approx (0.75f));
}

TEST_CASE ("ParallelWetMixer dry contribution unchanged when wetGain rises", "[chain][routing][ParallelWet]")
{
    const auto dryIn = 0.42f;
    const auto wet = 0.8f;
    const auto atZero = sendbloom::ParallelWetMixer::mix (dryIn, wet, 0.0f);
    const auto atFull = sendbloom::ParallelWetMixer::mix (dryIn, wet, 1.0f);

    REQUIRE (atZero == Catch::Approx (dryIn));
    REQUIRE (atFull == Catch::Approx (dryIn + wet));
}

TEST_CASE ("ParallelWetMixer wetGain matches levelEqualPower wet leg", "[chain][routing][ParallelWet]")
{
    const float levelNorms[] = { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f };

    for (const auto levelNorm : levelNorms)
    {
        float dryLeg = 0.0f;
        float wetLeg = 0.0f;
        sendbloom::ParameterCurves::levelEqualPower (levelNorm, dryLeg, wetLeg);

        const auto mixed = sendbloom::ParallelWetMixer::mix (1.0f, 1.0f, wetLeg);
        REQUIRE (mixed == Catch::Approx (1.0f + wetLeg).margin (1e-5f));
        REQUIRE (wetLeg == Catch::Approx (std::sin (juce::MathConstants<float>::halfPi * levelNorm)).margin (1e-5f));
    }
}

TEST_CASE ("ParallelWetMixer differs from dual-scaled equal-power sum", "[chain][routing][ParallelWet]")
{
    const auto dryTap = 0.5f;
    const auto wetSample = 0.6f;
    const auto levelNorm = 0.65f;

    float dryGain = 0.0f;
    float wetGain = 0.0f;
    sendbloom::ParameterCurves::levelEqualPower (levelNorm, dryGain, wetGain);

    const auto parallelMix = sendbloom::ParallelWetMixer::mix (dryTap, wetSample, wetGain);
    const auto dualScaled = legacyDualScaledEqualPowerSum (dryTap, wetSample, levelNorm);

    REQUIRE (dryGain == Catch::Approx (1.0f).margin (1e-5f));
    REQUIRE (wetGain > 0.0f);
    REQUIRE (parallelMix != Catch::Approx (dualScaled).margin (1e-4f));
    REQUIRE (parallelMix == Catch::Approx (dryTap + wetSample * wetGain).margin (1e-5f));
}
