#include <GatedBloomChain.h>
#include <NoiseGate.h>
#include <ParameterCurves.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

namespace
{

constexpr double kSampleRate = 48000.0;
constexpr int kFifteenMsSamples = 720;

} // namespace

TEST_CASE ("post gate wet drops within 15 ms of silence onset", "[gate][integration][TEST-02][timing]")
{
    sendbloom::GatedBloomChain chain;
    chain.prepare (kSampleRate, 512);

    const auto rt60 = sendbloom::ParameterCurves::sizeToRT60 (0.5f);
    constexpr auto kThresholdDb = -40.0f;

    float peakWet = 0.0f;

    for (int i = 0; i < 12000; ++i)
    {
        const auto input = 0.5f;
        const auto env = chain.getEnvelope().process (std::abs (input));
        const auto wet = chain.processSample (input, env, rt60, 0.0f, true, 0.3f, 1.0f, false, kThresholdDb);
        peakWet = std::max (peakWet, std::abs (wet));
    }

    REQUIRE (peakWet > 1e-4f);

    int silenceSamples = 0;
    bool choppedWithin15ms = false;

    for (int i = 0; i < 2000; ++i)
    {
        const auto env = chain.getEnvelope().process (0.0f);
        const auto wet = chain.processSample (0.0f, env, rt60, 0.0f, true, 0.3f, 1.0f, false, kThresholdDb);
        ++silenceSamples;

        if (silenceSamples <= kFifteenMsSamples && std::abs (wet) < 0.02f * peakWet)
        {
            choppedWithin15ms = true;
            break;
        }
    }

    REQUIRE (choppedWithin15ms);
}

TEST_CASE ("PostHard gate release budget under 15 ms", "[gate][NoiseGate][TEST-02][timing]")
{
    sendbloom::NoiseGate gate;
    gate.prepare (kSampleRate, sendbloom::GateProfile::PostHard);

    const auto openThresh = juce::Decibels::decibelsToGain (-40.0f);
    gate.process (openThresh * 2.0f, -40.0f);
    REQUIRE (gate.getIsOpen());

    int samplesToClosed = 0;
    bool reachedFloor = false;

    for (int i = 0; i < kFifteenMsSamples; ++i)
    {
        gate.process (0.0f, -40.0f);
        ++samplesToClosed;

        if (! gate.getIsOpen() && gate.getGain() <= 1.0e-4f)
        {
            reachedFloor = true;
            break;
        }
    }

    REQUIRE (samplesToClosed < kFifteenMsSamples);
    REQUIRE (reachedFloor);
    REQUIRE (gate.getGain() == Catch::Approx (0.0f).margin (1e-4f));
}
