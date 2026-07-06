#include <NoiseGate.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE ("NoiseGate PreSoft yields soft floor when closed", "[gate][NoiseGate]")
{
    sendbloom::NoiseGate preGate;
    sendbloom::NoiseGate postGate;
    preGate.prepare (48000.0, sendbloom::GateProfile::PreSoft);
    postGate.prepare (48000.0, sendbloom::GateProfile::PostHard);

    for (int i = 0; i < 20000; ++i)
    {
        preGate.process (0.00001f, -40.0f);
        postGate.process (0.00001f, -40.0f);
    }

    const auto preGain = preGate.getGain();
    const auto postGain = postGate.getGain();
    REQUIRE (preGain > 0.0f);
    REQUIRE (preGain < 0.1f);
    REQUIRE (postGain == Catch::Approx (0.0f).margin (1e-3f));
    REQUIRE (preGain > postGain);
}

TEST_CASE ("NoiseGate PostHard yields zero when closed", "[gate][NoiseGate]")
{
    sendbloom::NoiseGate gate;
    gate.prepare (48000.0, sendbloom::GateProfile::PostHard);

    for (int i = 0; i < 4800; ++i)
        gate.process (0.00001f, -40.0f);

    REQUIRE (gate.getGain() == Catch::Approx (0.0f).margin (1e-4f));
}

TEST_CASE ("NoiseGate hysteresis prevents chatter", "[gate][NoiseGate]")
{
    sendbloom::NoiseGate gate;
    gate.prepare (48000.0, sendbloom::GateProfile::PostHard);

    const auto openThresh = juce::Decibels::decibelsToGain (-40.0f);
    const auto closeThresh = juce::Decibels::decibelsToGain (-43.0f);
    const auto mid = (openThresh + closeThresh) * 0.5f;

    gate.process (openThresh * 2.0f, -40.0f);
    REQUIRE (gate.getIsOpen());

    gate.process (mid, -40.0f);
    REQUIRE (gate.getIsOpen());

    gate.process (closeThresh * 0.5f, -40.0f);
    REQUIRE_FALSE (gate.getIsOpen());
}

TEST_CASE ("NoiseGate opens above threshold", "[gate][NoiseGate]")
{
    sendbloom::NoiseGate gate;
    gate.prepare (48000.0, sendbloom::GateProfile::PostHard);

    const auto gain = gate.process (1.0f, -40.0f);
    REQUIRE (gain > 0.9f);
}
