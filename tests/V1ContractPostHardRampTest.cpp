#include <NoiseGate.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

TEST_CASE ("v1 PostHard close uses sub-ms ramp not one-sample snap",
           "[v1][contract][posthard][CORE-10]")
{
    // CORE-10/11 / ADR-V1-11: after open→close, first closed sample gain in (0,1),
    // |Δgain| ≤ 0.05, and gain ≤ 1e-4 within 1 ms. Current PostHard snaps to 0.
    constexpr double kSampleRate = 48000.0;
    constexpr float kThresholdDb = -40.0f;

    sendbloom::NoiseGate gate;
    gate.prepare (kSampleRate, sendbloom::GateProfile::PostHard);

    const auto openThresh = juce::Decibels::decibelsToGain (kThresholdDb);
    gate.process (openThresh * 2.0f, kThresholdDb);
    REQUIRE (gate.getIsOpen());

    const float g0 = gate.getGain();
    REQUIRE (g0 == Catch::Approx (1.0f).margin (1.0e-4f));

    const float g1 = gate.process (0.0f, kThresholdDb); // first close sample
    REQUIRE_FALSE (gate.getIsOpen());

    // Intended failure: current floorGain==0 snap sets g1=0 immediately.
    REQUIRE (g1 > 0.0f);
    REQUIRE (g1 < 1.0f);
    REQUIRE (std::abs (g0 - g1) <= 0.05f);

    const int oneMsSamples = static_cast<int> (kSampleRate * 0.001);
    float gainAt1ms = g1;

    for (int i = 1; i < oneMsSamples; ++i)
        gainAt1ms = gate.process (0.0f, kThresholdDb);

    REQUIRE (gainAt1ms <= 1.0e-4f);
}
