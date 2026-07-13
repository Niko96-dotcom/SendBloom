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
    REQUIRE (gate.getGain() == Catch::Approx (1.0f).margin (1.0e-4f));

    // Silence holds the gate open for kHoldMs before it issues the close command.
    // CORE-11 measures the ramp from that command, so advance through the hold
    // until isOpen flips, tracking the last open-sample gain (g0).
    float g0 = gate.getGain();
    int guard = 0;
    const int holdGuard =
        static_cast<int> (kSampleRate * (sendbloom::NoiseGate::kHoldMs + 2.0) * 0.001);

    while (gate.getIsOpen())
    {
        g0 = gate.getGain();
        gate.process (0.0f, kThresholdDb);
        REQUIRE (++guard < holdGuard);
    }

    // First sample after the close command: a ramp step, not a one-sample snap.
    const float g1 = gate.getGain();
    REQUIRE (g1 > 0.0f);
    REQUIRE (g1 < 1.0f);
    REQUIRE (std::abs (g0 - g1) <= 0.05f);

    const int oneMsSamples = static_cast<int> (kSampleRate * 0.001);
    float gainAt1ms = g1;

    for (int i = 1; i < oneMsSamples; ++i)
        gainAt1ms = gate.process (0.0f, kThresholdDb);

    REQUIRE (gainAt1ms <= 1.0e-4f);
}
