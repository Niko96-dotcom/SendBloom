#include <NoiseGate.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

// ADR-V1-11c: there is one gate circuit with one envelope. The Gate switch moves
// it pre<->post; it does not swap in a gentler gate. These tests pin the single
// envelope; GatePlacementTest.cpp pins what each position does with it.

TEST_CASE ("NoiseGate closes to exact zero", "[gate][NoiseGate]")
{
    sendbloom::NoiseGate gate;
    gate.prepare (48000.0);

    for (int i = 0; i < 4800; ++i)
        gate.process (0.00001f, -40.0f);

    REQUIRE_FALSE (gate.getIsOpen());
    REQUIRE (gate.getGain() == 0.0f);
}

TEST_CASE ("NoiseGate reaches its floor within the 15 ms budget", "[gate][NoiseGate][CORE-13]")
{
    // The former PreSoft profile needed 705 ms to reach -40 dB. One circuit means
    // one close speed, and it is the fast one — this is what keeps hum out of the
    // tank in the pre position.
    sendbloom::NoiseGate gate;
    gate.prepare (48000.0);

    gate.process (juce::Decibels::decibelsToGain (-40.0f) * 2.0f, -40.0f);
    REQUIRE (gate.getIsOpen());

    int samples = 0;
    constexpr int kFifteenMs = 720;

    for (; samples < kFifteenMs; ++samples)
    {
        gate.process (0.0f, -40.0f);

        if (gate.getGain() <= 1.0e-4f)
            break;
    }

    REQUIRE (samples < kFifteenMs);
    REQUIRE (gate.getGain() == Catch::Approx (0.0f).margin (1e-4f));
}

TEST_CASE ("NoiseGate hysteresis prevents chatter", "[gate][NoiseGate]")
{
    sendbloom::NoiseGate gate;
    gate.prepare (48000.0);

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
    gate.prepare (48000.0);

    const auto gain = gate.process (1.0f, -40.0f);
    REQUIRE (gain > 0.9f);
}

TEST_CASE ("NoiseGate linear threshold path matches dB path",
           "[gate][NoiseGate][performance][regression]")
{
    sendbloom::NoiseGate dbGate;
    sendbloom::NoiseGate linearGate;
    dbGate.prepare (48000.0);
    linearGate.prepare (48000.0);

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

TEST_CASE ("NoiseGate close speed is sample-rate independent", "[gate][NoiseGate]")
{
    for (double sampleRate : { 44100.0, 48000.0, 88200.0, 96000.0 })
    {
        sendbloom::NoiseGate gate;
        gate.prepare (sampleRate);
        gate.process (1.0f, -40.0f);

        int samples = 0;
        const int guard = static_cast<int> (sampleRate * 0.05);

        for (; samples < guard; ++samples)
        {
            gate.process (0.0f, -40.0f);

            if (gate.getGain() <= 1.0e-4f)
                break;
        }

        const auto ms = 1000.0 * samples / sampleRate;
        INFO ("sample rate " << sampleRate << " closed in " << ms << " ms");
        // Hold (5 ms) + close ramp (0.75 ms), independent of host rate.
        REQUIRE (ms > 5.0);
        REQUIRE (ms < 7.0);
    }
}
