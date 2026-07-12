#include "ChainTestHelpers.h"
#include <GatedBloomChain.h>
#include <InputStage.h>
#include <ParameterCurves.h>
#include <ParameterSnapshot.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <ParallelWetMixer.h>
#include <PluginProcessor.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <vector>

namespace
{

constexpr auto kSampleRate = 48000.0f;
constexpr auto kBlockSize = 512;
constexpr auto kFundamentalHz = kSampleRate * 0.03f / (2.0f * 3.14159265358979323846f);
constexpr auto kWarmupBlocks = 48;
constexpr auto kMeasureSamples = 4096;

struct RenderCapture
{
    std::vector<float> output;
    std::vector<float> wet;
    std::vector<float> dryExtract;
};

RenderCapture renderWithDistn (float distnValue)
{
    using namespace sendbloom::ParameterIDs;

    sendbloom::PluginProcessor plugin;
    auto& apvts = plugin.getAPVTS();
    *apvts.getRawParameterValue (inputGain) = 0.35f;
    *apvts.getRawParameterValue (outputGain) = 0.0f;
    *apvts.getRawParameterValue (bypass) = 0.0f;
    *apvts.getRawParameterValue (level) = 1.0f;
    *apvts.getRawParameterValue (distn) = distnValue;
    *apvts.getRawParameterValue (size) = 0.5f;
    *apvts.getRawParameterValue (sendConnected) = 1.0f;
    *apvts.getRawParameterValue (sendAmount) = 1.0f;
    *apvts.getRawParameterValue (gatePrePost) = 0.0f;
    *apvts.getRawParameterValue (inputThreshold) = 0.0f;
    plugin.prepareToPlay (kSampleRate, kBlockSize);

    sendbloom::GatedBloomChain chain;
    sendbloom::InputStage inputStage;
    chain.prepare (kSampleRate, kBlockSize);
    inputStage.prepare (kSampleRate);

    const auto snap = sendbloom::ParameterSnapshot::capture (apvts);
    const auto rt60 = sendbloom::ParameterCurves::sizeToRT60 (snap.sizeNorm);
    const auto thresholdDb = sendbloom::ParameterCurves::inputThresholdDb (snap.inputThresholdNorm);
    const auto inputGainLinear = juce::Decibels::decibelsToGain (snap.inputGainDb);
    const auto distnBlend = snap.distnBlend;
    const auto sendGain = snap.sendGain;
    const auto gatePreSoft = snap.gatePreSoft;
    const auto darkModeMix = snap.darkMode ? 1.0f : 0.0f;
    const auto authenticColor = snap.authenticColor;

    RenderCapture capture;
    capture.output.reserve (static_cast<size_t> (kWarmupBlocks * kBlockSize));
    capture.wet.reserve (capture.output.capacity());
    capture.dryExtract.reserve (capture.output.capacity());

    std::vector<float> inputHistory;
    inputHistory.reserve (capture.output.capacity());

    juce::AudioBuffer<float> buffer (1, kBlockSize);
    juce::MidiBuffer midi;
    const auto phaseInc = 0.03f;
    auto sampleIndex = 0;

    for (int block = 0; block < kWarmupBlocks; ++block)
    {
        for (int i = 0; i < kBlockSize; ++i)
        {
            const auto input = 0.22f * std::sin (phaseInc * static_cast<float> (sampleIndex++));
            inputHistory.push_back (input);
            buffer.setSample (0, i, input);
        }

        plugin.processBlock (buffer, midi);

        for (int i = 0; i < kBlockSize; ++i)
        {
            const auto historyIndex = static_cast<size_t> (block * kBlockSize + i);
            const auto dryTap = inputHistory[historyIndex];
            const auto monoIn = inputStage.processSample (dryTap, inputGainLinear);
            const auto env = chain.getEnvelope().process (std::abs (monoIn));
            const auto wet = chain.processSample (monoIn, env, rt60, darkModeMix, authenticColor,
                                                  distnBlend, sendGain, gatePreSoft, thresholdDb);
            const auto mixed = buffer.getSample (0, i);

            capture.output.push_back (mixed);
            capture.wet.push_back (wet);
            capture.dryExtract.push_back (mixed - wet);
        }
    }

    return capture;
}

} // namespace

TEST_CASE ("dry tap extract matches input at level max", "[chain][od][thd][DryPath]")
{
    const auto capture = renderWithDistn (1.0f);
    const auto start = capture.dryExtract.size() - kMeasureSamples;

    float maxDelta = 0.0f;

    for (size_t i = start; i < capture.dryExtract.size(); ++i)
    {
        const auto input = 0.22f * std::sin (0.03f * static_cast<float> (i));
        maxDelta = std::max (maxDelta, std::abs (capture.dryExtract[i] - input));
    }

    REQUIRE (maxDelta < 0.02f);
}

TEST_CASE ("dry path THD unchanged at distn max level max", "[chain][od][thd][DryPath]")
{
    const auto clean = renderWithDistn (0.0f);
    const auto driven = renderWithDistn (1.0f);
    const auto start = clean.dryExtract.size() - kMeasureSamples;

    const auto thdClean = sendbloom::test::measureThd (clean.dryExtract, kSampleRate, kFundamentalHz, start, kMeasureSamples);
    const auto thdDriven = sendbloom::test::measureThd (driven.dryExtract, kSampleRate, kFundamentalHz, start, kMeasureSamples);

    REQUIRE (thdClean == Catch::Approx (thdDriven).margin (0.002f));
    REQUIRE (thdClean < 0.05f);

    float maxDryDelta = 0.0f;

    for (size_t i = start; i < clean.dryExtract.size(); ++i)
        maxDryDelta = std::max (maxDryDelta, std::abs (clean.dryExtract[i] - driven.dryExtract[i]));

    REQUIRE (maxDryDelta < 0.03f);
}

TEST_CASE ("wet grind increases with distn at level max", "[chain][od][DryPath]")
{
    const auto clean = renderWithDistn (0.0f);
    const auto driven = renderWithDistn (1.0f);
    const auto start = clean.wet.size() - kMeasureSamples;

    const auto wetClean = std::vector<float> (clean.wet.begin() + static_cast<std::ptrdiff_t> (start), clean.wet.end());
    const auto wetDriven = std::vector<float> (driven.wet.begin() + static_cast<std::ptrdiff_t> (start), driven.wet.end());

    REQUIRE (sendbloom::test::rms (wetDriven) != Catch::Approx (sendbloom::test::rms (wetClean)).margin (1e-4f));
}
