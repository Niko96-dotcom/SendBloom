#include <ParameterCurves.h>
#include <ParameterIDs.h>
#include <PluginProcessor.h>
#include <PressureController.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include <cmath>

namespace
{

constexpr double kSampleRate = 48000.0;

float settleGain (sendbloom::PressureController& c, int samples)
{
    float g = 0.0f;
    for (int i = 0; i < samples; ++i)
        g = c.processSample();
    return g;
}

} // namespace

TEST_CASE ("PressureController disconnected returns unity regardless of host pressure",
           "[send][PressureController]")
{
    sendbloom::PressureController c;
    c.prepare (kSampleRate);
    c.setConnected (false);
    c.setHostPressureTarget (0.0f);
    c.setMidiPressureTarget (0.0f);
    c.setFirmFeel (true);

    REQUIRE (c.processSample() == Catch::Approx (1.0f));

    c.setHostPressureTarget (1.0f);
    REQUIRE (c.processSample() == Catch::Approx (1.0f));
}

TEST_CASE ("PressureController connected at host 0 settles to zero gain",
           "[send][PressureController]")
{
    sendbloom::PressureController c;
    c.prepare (kSampleRate);
    c.setConnected (true);
    c.setHostPressureTarget (0.0f);
    c.setMidiPressureTarget (0.0f);
    c.setFirmFeel (true);

    // Seed nonzero state then release to 0 so we prove settle, not initial zero.
    c.setHostPressureTarget (1.0f);
    settleGain (c, static_cast<int> (kSampleRate * 0.2));
    c.setHostPressureTarget (0.0f);
    const auto gain = settleGain (c, static_cast<int> (kSampleRate * 0.5));
    REQUIRE (gain == Catch::Approx (0.0f).margin (1e-4f));
}

TEST_CASE ("PressureController connected settled gain matches ParameterCurves Firm/Soft",
           "[send][PressureController]")
{
    sendbloom::PressureController firm;
    firm.prepare (kSampleRate);
    firm.setConnected (true);
    firm.setMidiPressureTarget (0.0f);
    firm.setFirmFeel (true);
    firm.setHostPressureTarget (0.5f);
    const auto firmGain = settleGain (firm, static_cast<int> (kSampleRate * 0.5));
    REQUIRE (firmGain
             == Catch::Approx (sendbloom::ParameterCurves::sendGain (0.5f, true)).margin (1e-4f));

    sendbloom::PressureController soft;
    soft.prepare (kSampleRate);
    soft.setConnected (true);
    soft.setMidiPressureTarget (0.0f);
    soft.setFirmFeel (false);
    soft.setHostPressureTarget (0.5f);
    const auto softGain = settleGain (soft, static_cast<int> (kSampleRate * 0.5));
    REQUIRE (softGain
             == Catch::Approx (sendbloom::ParameterCurves::sendGain (0.5f, false)).margin (1e-4f));

    REQUIRE (firmGain != Catch::Approx (softGain).margin (1e-6f));
}

TEST_CASE ("PressureController attack reaches ~63% of settled Firm gain near 3 ms",
           "[send][PressureController]")
{
    sendbloom::PressureController c;
    c.prepare (kSampleRate);
    c.reset();
    c.setConnected (true);
    c.setMidiPressureTarget (0.0f);
    c.setFirmFeel (true);
    c.setHostPressureTarget (0.0f);
    settleGain (c, 64);

    c.setHostPressureTarget (1.0f);
    const auto settled = sendbloom::ParameterCurves::sendGain (1.0f, true);
    const int attackSamples = static_cast<int> (std::lround (0.003 * kSampleRate));
    const auto gainAtTau = settleGain (c, attackSamples);

    // Smooth raw pressure to ~63% at τ, then curve — expect post-curve gain at that pressure.
    const auto expected = sendbloom::ParameterCurves::sendGain (1.0f - std::exp (-1.0f), true);
    REQUIRE (gainAtTau == Catch::Approx (expected).margin (expected * 0.25f));
    REQUIRE (gainAtTau < settled);
}

TEST_CASE ("PressureController release reaches ~37% residual near 25 ms",
           "[send][PressureController]")
{
    sendbloom::PressureController c;
    c.prepare (kSampleRate);
    c.setConnected (true);
    c.setMidiPressureTarget (0.0f);
    c.setFirmFeel (true);
    c.setHostPressureTarget (1.0f);
    settleGain (c, static_cast<int> (kSampleRate * 0.5));

    c.setHostPressureTarget (0.0f);
    const int releaseSamples = static_cast<int> (std::lround (0.025 * kSampleRate));
    const auto gainAtTau = settleGain (c, releaseSamples);

    const auto expected = sendbloom::ParameterCurves::sendGain (std::exp (-1.0f), true);
    REQUIRE (gainAtTau == Catch::Approx (expected).margin (std::max (expected * 0.20f, 1e-4f)));
}

TEST_CASE ("PressureController clamps host and midi targets to [0,1]",
           "[send][PressureController]")
{
    sendbloom::PressureController c;
    c.prepare (kSampleRate);
    c.setConnected (true);
    c.setFirmFeel (true);
    c.setHostPressureTarget (2.0f);
    c.setMidiPressureTarget (-1.0f);
    const auto gain = settleGain (c, static_cast<int> (kSampleRate * 0.5));
    REQUIRE (gain == Catch::Approx (sendbloom::ParameterCurves::sendGain (1.0f, true)).margin (1e-4f));
}

TEST_CASE ("PressureController advances release while disconnected before reconnect",
           "[send][PressureController][reconnect][regression]")
{
    sendbloom::PressureController c;
    c.prepare (kSampleRate);
    c.setConnected (true);
    c.setFirmFeel (true);
    c.setHostPressureTarget (1.0f);
    settleGain (c, static_cast<int> (kSampleRate * 0.2));

    c.setConnected (false);
    c.setHostPressureTarget (0.0f);
    REQUIRE (settleGain (c, static_cast<int> (kSampleRate * 0.5))
             == Catch::Approx (1.0f));

    c.setConnected (true);
    REQUIRE (c.processSample() == Catch::Approx (0.0f).margin (1.0e-4f));
}

TEST_CASE ("PressureController keeps current disconnected MIDI and clears stale connected MIDI",
           "[send][PressureController][midi][reconnect][regression]")
{
    sendbloom::PressureController c;
    c.prepare (kSampleRate);
    c.setConnected (true);
    c.setFirmFeel (true);
    c.setMidiPressureTarget (1.0f);
    settleGain (c, static_cast<int> (kSampleRate * 0.2));

    c.setConnected (false); // transition clears the old connected CC1 value
    c.setMidiPressureTarget (0.0f); // current CC1 observed while disconnected
    settleGain (c, static_cast<int> (kSampleRate * 0.5));
    c.setConnected (true);
    REQUIRE (c.processSample() == Catch::Approx (0.0f).margin (1.0e-4f));

    c.setConnected (false);
    c.setMidiPressureTarget (1.0f); // a current disconnected CC1 value is authoritative
    settleGain (c, static_cast<int> (kSampleRate * 0.5));
    c.setConnected (true);
    REQUIRE (c.processSample()
             == Catch::Approx (sendbloom::ParameterCurves::sendGain (1.0f, true)).margin (1.0e-4f));
}

TEST_CASE ("PressureController combines host and MIDI pressure by maximum",
           "[send][PressureController][midi]")
{
    sendbloom::PressureController c;
    c.prepare (kSampleRate);
    c.setConnected (true);
    c.setFirmFeel (true);
    c.setHostPressureTarget (0.25f);
    c.setMidiPressureTarget (0.75f);
    auto gain = settleGain (c, static_cast<int> (kSampleRate * 0.5));
    REQUIRE (gain
             == Catch::Approx (sendbloom::ParameterCurves::sendGain (0.75f, true)).margin (1.0e-4f));

    c.setHostPressureTarget (0.9f);
    gain = settleGain (c, static_cast<int> (kSampleRate * 0.5));
    REQUIRE (gain
             == Catch::Approx (sendbloom::ParameterCurves::sendGain (0.9f, true)).margin (1.0e-4f));
}

TEST_CASE ("PressureController applies Firm Soft changes made while disconnected",
           "[send][PressureController][reconnect]")
{
    sendbloom::PressureController c;
    c.prepare (kSampleRate);
    c.setConnected (false);
    c.setHostPressureTarget (0.5f);
    c.setFirmFeel (false);
    settleGain (c, static_cast<int> (kSampleRate * 0.5));

    c.setConnected (true);
    REQUIRE (c.processSample()
             == Catch::Approx (sendbloom::ParameterCurves::sendGain (0.5f, false)).margin (1.0e-4f));
}

TEST_CASE ("Project restore initializes connected and disconnected pressure modes",
           "[send][PressureController][state][regression]")
{
    using namespace sendbloom::ParameterIDs;

    for (const bool connected : { false, true })
    {
        sendbloom::PluginProcessor source;
        source.getAPVTS().getParameter (sendConnected)->setValueNotifyingHost (
            connected ? 1.0f : 0.0f);
        source.getAPVTS().getParameter (sendAmount)->setValueNotifyingHost (0.75f);
        source.getAPVTS().getParameter (sendFeel)->setValueNotifyingHost (1.0f); // Soft

        juce::MemoryBlock state;
        source.getStateInformation (state);

        sendbloom::PluginProcessor restored;
        restored.setStateInformation (state.getData(), static_cast<int> (state.getSize()));
        restored.prepareToPlay (kSampleRate, 512);

        const auto gain = restored.pressureController.processSample();
        INFO ("connected " << connected);
        if (connected)
            REQUIRE (gain
                     == Catch::Approx (sendbloom::ParameterCurves::sendGain (0.75f, false))
                            .margin (1.0e-4f));
        else
            REQUIRE (gain == Catch::Approx (1.0f));
    }
}

// SEND-14 in-range: pressure semantics are sample-rate based and must match across
// prepared host block sizes {64,128,256,512}. Oversized numSamples > preparedMaxBlock_
// still forces dry wet=0 until Phase 21 — do not claim full host-block invariance;
// leave [v1][contract][oversized-block] red.
TEST_CASE ("SEND-14 PressureController connect/rest/press match across prepared block sizes",
           "[send][SEND-14]")
{
    constexpr int kBlockSizes[] = { 64, 128, 256, 512 };
    constexpr int kSettleSamples = static_cast<int> (kSampleRate * 0.5);

    struct Snapshot
    {
        float disconnectedUnity = 0.0f;
        float connectedRest = 0.0f;
        float connectedPress = 0.0f;
        float connectedRelease = 0.0f;
    };

    Snapshot reference {};
    bool haveReference = false;

    for (const int blockSize : kBlockSizes)
    {
        INFO ("prepared block size " << blockSize);

        // Disconnected → unity (always-on), independent of chunking.
        {
            sendbloom::PressureController c;
            c.prepare (kSampleRate);
            c.setConnected (false);
            c.setHostPressureTarget (1.0f);
            c.setMidiPressureTarget (0.0f);
            c.setFirmFeel (true);

            float last = 0.0f;
            int remaining = kSettleSamples;
            while (remaining > 0)
            {
                const int n = std::min (blockSize, remaining);
                for (int i = 0; i < n; ++i)
                    last = c.processSample();
                remaining -= n;
            }
            REQUIRE (last == Catch::Approx (1.0f).margin (1e-4f));
            if (! haveReference)
                reference.disconnectedUnity = last;
            else
                REQUIRE (last == Catch::Approx (reference.disconnectedUnity).margin (1e-4f));
        }

        // Connected rest → 0; press → Firm curve(1); release → 0.
        {
            sendbloom::PressureController c;
            c.prepare (kSampleRate);
            c.setConnected (true);
            c.setMidiPressureTarget (0.0f);
            c.setFirmFeel (true);
            c.setHostPressureTarget (0.0f);

            auto processChunks = [&] (int samples) {
                float last = 0.0f;
                int remaining = samples;
                while (remaining > 0)
                {
                    const int n = std::min (blockSize, remaining);
                    for (int i = 0; i < n; ++i)
                        last = c.processSample();
                    remaining -= n;
                }
                return last;
            };

            const auto rest = processChunks (kSettleSamples);
            REQUIRE (rest == Catch::Approx (0.0f).margin (1e-4f));

            c.setHostPressureTarget (1.0f);
            const auto press = processChunks (kSettleSamples);
            const auto expectedPress = sendbloom::ParameterCurves::sendGain (1.0f, true);
            REQUIRE (press == Catch::Approx (expectedPress).margin (1e-4f));

            c.setHostPressureTarget (0.0f);
            const auto released = processChunks (kSettleSamples);
            REQUIRE (released == Catch::Approx (0.0f).margin (1e-4f));

            if (! haveReference)
            {
                reference.connectedRest = rest;
                reference.connectedPress = press;
                reference.connectedRelease = released;
                haveReference = true;
            }
            else
            {
                REQUIRE (rest == Catch::Approx (reference.connectedRest).margin (1e-4f));
                REQUIRE (press == Catch::Approx (reference.connectedPress).margin (1e-4f));
                REQUIRE (released == Catch::Approx (reference.connectedRelease).margin (1e-4f));
            }
        }
    }
}
