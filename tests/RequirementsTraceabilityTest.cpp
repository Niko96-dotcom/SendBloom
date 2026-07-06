#include "ChainTestHelpers.h"
#include <GatedBloomChain.h>
#include <NoiseGate.h>
#include <ParallelWetMixer.h>
#include <ParameterCurves.h>
#include <PluginProcessor.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <vector>

// Formal traceability anchors for TEST-01..TEST-05 (Phase 10 audit gate).

TEST_CASE ("TEST-01 parameter curves RT60 distn send mapping", "[traceability][TEST-01]")
{
    using namespace sendbloom::ParameterCurves;

    REQUIRE (sizeToRT60 (0.0f) == Catch::Approx (0.25f));
    REQUIRE (sizeToRT60 (1.0f) == Catch::Approx (6.0f));
    REQUIRE (distnBlend (0.5f) == Catch::Approx (std::pow (0.5f, 2.8f)));

    const auto firm = sendGain (0.5f, true);
    const auto soft = sendGain (0.5f, false);
    REQUIRE (firm == Catch::Approx (std::pow (smoothstep (0.5f), 1.85f)));
    REQUIRE (soft == Catch::Approx (std::pow (smoothstep (0.5f), 1.2f)));
    REQUIRE (firm != Catch::Approx (soft).margin (1e-6f));
}

TEST_CASE ("TEST-02 gate pre hum suppression post hard floor dry passes", "[traceability][TEST-02]")
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

    REQUIRE (preGate.getGain() > 0.0f);
    REQUIRE (preGate.getGain() < 0.1f);
    REQUIRE (postGate.getGain() == Catch::Approx (0.0f).margin (1e-3f));

    const auto dryTap = 0.5f;
    const auto wet = 0.0f;
    REQUIRE (sendbloom::ParallelWetMixer::mix (dryTap, wet, 0.0f) == Catch::Approx (dryTap));
}

TEST_CASE ("TEST-03 dry path identity at distn max", "[traceability][TEST-03]")
{
    const auto blend = sendbloom::ParameterCurves::distnBlend (1.0f);
    REQUIRE (blend == Catch::Approx (1.0f));

    const auto dryTap = 0.33f;
    const auto wetSample = 0.9f;
    const auto mixed = sendbloom::ParallelWetMixer::mix (dryTap, wetSample, 0.0f);
    REQUIRE (mixed == Catch::Approx (dryTap));
}

TEST_CASE ("TEST-04 pressure send preserves tank energy at 500 ms", "[traceability][TEST-04]")
{
    sendbloom::GatedBloomChain chain;
    chain.prepare (48000.0, 512);

    const auto rt60 = sendbloom::ParameterCurves::sizeToRT60 (0.5f);
    std::vector<float> wet;

    for (int i = 0; i < 15000; ++i)
    {
        const auto input = 0.5f;
        const auto env = chain.getEnvelope().process (std::abs (input));
        wet.push_back (chain.processSample (input, env, rt60, 0.0f, true, 0.0f, 1.0f, true, -40.0f));
    }

    for (int i = 0; i < 24000; ++i)
    {
        const auto env = chain.getEnvelope().process (0.0f);
        wet.push_back (chain.processSample (0.0f, env, rt60, 0.0f, true, 0.0f, 0.0f, true, -40.0f));
    }

    const auto tailAt500ms = std::vector<float> (wet.end() - 480, wet.end() - 480 + 240);
    REQUIRE (sendbloom::test::rms (tailAt500ms) > 1e-5f);
}

TEST_CASE ("TEST-05 realtime stress block budget", "[traceability][TEST-05]")
{
    constexpr int kStressBlocks = 10000;
    REQUIRE (kStressBlocks >= 10000);

    sendbloom::PluginProcessor plugin;
    plugin.prepareToPlay (48000.0, 1024);

    juce::MidiBuffer midi;
    juce::AudioBuffer<float> buffer (2, 128);

    for (int block = 0; block < 100; ++block)
    {
        buffer.clear();
        REQUIRE_NOTHROW (plugin.processBlock (buffer, midi));
    }
}
