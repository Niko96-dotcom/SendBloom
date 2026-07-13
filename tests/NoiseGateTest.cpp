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
    const auto closeThresh =
        juce::Decibels::decibelsToGain (-40.0f - sendbloom::NoiseGate::kHysteresisDb);
    const auto mid = (openThresh + closeThresh) * 0.5f;

    gate.process (openThresh * 2.0f, -40.0f);
    REQUIRE (gate.getIsOpen());

    // Sitting between the close and open thresholds keeps it open (hysteresis).
    for (int i = 0; i < 512; ++i)
        gate.process (mid, -40.0f);
    REQUIRE (gate.getIsOpen());

    // Dropping well below close for longer than the hold window closes it.
    for (int i = 0; i < 2048; ++i)
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

TEST_CASE ("NoiseGate linear threshold path matches dB path",
           "[gate][NoiseGate][performance][regression]")
{
    sendbloom::NoiseGate dbGate;
    sendbloom::NoiseGate linearGate;
    dbGate.prepare (48000.0, sendbloom::GateProfile::PreSoft);
    linearGate.prepare (48000.0, sendbloom::GateProfile::PreSoft);

    constexpr float thresholdDb = -43.0f;
    const auto thresholdLinear = juce::Decibels::decibelsToGain (thresholdDb);

    for (int i = 0; i < 10000; ++i)
    {
        const auto envelope = (i % 700) < 250 ? 0.02f : 0.00001f;
        const auto dbGain = dbGate.process (envelope, thresholdDb);
        const auto linearGain = linearGate.processLinear (envelope, thresholdLinear);
        REQUIRE (linearGain == Catch::Approx (dbGain).margin (1.0e-7f));
    }
}
