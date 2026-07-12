#include "ChainTestHelpers.h"
#include <GatedBloomChain.h>
#include <ParallelWetMixer.h>
#include <ParameterCurves.h>
#include <algorithm>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <vector>

namespace
{

constexpr auto kSampleRate = 48000.0;
constexpr auto kBlockSize = 512;
constexpr auto kThresholdDb = -40.0f;
constexpr int kWarmupSamples = 15000;
constexpr auto kPi = 3.14159265358979323846f;

float processChainSample (sendbloom::GatedBloomChain& chain,
                          float input,
                          float rt60,
                          float distnBlend,
                          float sendGain,
                          bool gatePreSoft,
                          float darkModeMix = 0.0f,
                          bool authenticColor = false)
{
    const auto env = chain.getEnvelope().process (std::abs (input));
    return chain.processSample (input, env, rt60, darkModeMix, authenticColor,
                                distnBlend, sendGain, gatePreSoft, kThresholdDb);
}

std::vector<float> renderChain (sendbloom::GatedBloomChain& chain,
                                const std::vector<float>& input,
                                float rt60,
                                float distnBlend,
                                float sendGain,
                                bool gatePreSoft)
{
    std::vector<float> wet (input.size());

    for (size_t i = 0; i < input.size(); ++i)
        wet[i] = processChainSample (chain, input[i], rt60, distnBlend, sendGain, gatePreSoft);

    return wet;
}

} // namespace

TEST_CASE ("GatedBloomChain dry tap unchanged when wet gain zero", "[chain][routing][GatedBloomChain]")
{
    sendbloom::GatedBloomChain chain;
    chain.prepare (kSampleRate, kBlockSize);

    const auto input = 0.35f;
    const auto rt60 = sendbloom::ParameterCurves::sizeToRT60 (0.5f);
    const auto wet = processChainSample (chain, input, rt60, 1.0f, 1.0f, false);
    const auto mixed = sendbloom::ParallelWetMixer::mix (input, wet, 0.0f);

    REQUIRE (mixed == Catch::Approx (input));
}

TEST_CASE ("GatedBloomChain wet-only dirt increases wet magnitude", "[chain][routing][GatedBloomChain]")
{
    sendbloom::GatedBloomChain chain;
    chain.prepare (kSampleRate, kBlockSize);

    // ADR-V1-15 added a 100 Hz pre-clip high-pass to the dirty branch (DSP-09),
    // so a DC burst is no longer a valid stimulus here: the HP removes the DC
    // energy that previously made dirty louder than clean. A 220 Hz tone keeps
    // content above the HP and exercises the clipper's added harmonics, which is
    // the behaviour under test. See milestone spec §13.7 / §17.2 (DC gate).
    constexpr auto kProbeFreqHz = 220.0f;
    const auto phaseInc = 2.0f * kPi * kProbeFreqHz / static_cast<float> (kSampleRate);
    std::vector<float> burst (static_cast<size_t> (kWarmupSamples));

    for (size_t i = 0; i < burst.size(); ++i)
        burst[i] = 0.5f * std::sin (phaseInc * static_cast<float> (i));

    const auto rt60 = sendbloom::ParameterCurves::sizeToRT60 (0.5f);

    const auto cleanWet = renderChain (chain, burst, rt60, 0.0f, 1.0f, true);
    chain.getEnvelope().reset();
    chain.prepare (kSampleRate, kBlockSize);

    const auto dirtyWet = renderChain (chain, burst, rt60, 1.0f, 1.0f, true);

    REQUIRE (sendbloom::test::rms (dirtyWet) > sendbloom::test::rms (cleanWet));

    const auto dryTap = 0.5f;
    const auto mixedClean = sendbloom::ParallelWetMixer::mix (dryTap, cleanWet.back(), 0.5f);
    const auto mixedDirty = sendbloom::ParallelWetMixer::mix (dryTap, dirtyWet.back(), 0.5f);
    REQUIRE (mixedClean == Catch::Approx (dryTap + cleanWet.back() * 0.5f).margin (1e-4f));
    REQUIRE (mixedDirty == Catch::Approx (dryTap + dirtyWet.back() * 0.5f).margin (1e-4f));
}

TEST_CASE ("EnvelopeDetector decays during silence", "[chain][routing][envelope]")
{
    sendbloom::EnvelopeDetector env;
    env.prepare (kSampleRate, 5.0f, 10.0f);

    for (int i = 0; i < kWarmupSamples; ++i)
        env.process (0.5f);

    REQUIRE (env.getEnvelope() > 0.4f);

    for (int i = 0; i < 2400; ++i)
        env.process (0.0f);

    REQUIRE (env.getEnvelope() < 0.01f);
}

TEST_CASE ("GatedBloomChain post gate keyed from input envelope", "[chain][routing][GatedBloomChain][postGate]")
{
    sendbloom::GatedBloomChain chain;
    chain.prepare (kSampleRate, kBlockSize);

    const auto rt60 = sendbloom::ParameterCurves::sizeToRT60 (0.5f);
    std::vector<float> wet;

    for (int i = 0; i < kWarmupSamples; ++i)
        wet.push_back (processChainSample (chain, 0.5f, rt60, 0.3f, 1.0f, false));

    for (int i = 0; i < 4800; ++i)
        wet.push_back (processChainSample (chain, 0.0f, rt60, 0.3f, 1.0f, false));

    const auto silenceSlice = std::vector<float> (wet.end() - 512, wet.end());
    REQUIRE (sendbloom::test::rms (silenceSlice) < 0.01f);
}

TEST_CASE ("GatedBloomChain send release preserves tank energy", "[chain][routing][GatedBloomChain][send][tank]")
{
    sendbloom::GatedBloomChain chain;
    chain.prepare (kSampleRate, kBlockSize);

    const auto rt60 = sendbloom::ParameterCurves::sizeToRT60 (0.5f);
    std::vector<float> wet;

    for (int i = 0; i < kWarmupSamples; ++i)
        wet.push_back (processChainSample (chain, 0.5f, rt60, 0.0f, 1.0f, true));

    constexpr auto kTrailSamples = 24000;
    for (int i = 0; i < kTrailSamples; ++i)
        wet.push_back (processChainSample (chain, 0.0f, rt60, 0.0f, 0.0f, true));

    const auto tailAt500ms = std::vector<float> (wet.end() - 480, wet.end() - 480 + 240);
    REQUIRE (sendbloom::test::rms (tailAt500ms) > 1e-5f);

    const auto finalSlice = std::vector<float> (wet.end() - 240, wet.end());
    REQUIRE (sendbloom::test::rms (finalSlice) > 1e-6f);
}

TEST_CASE ("GatedBloomChain topology smoke with reverb stub", "[chain][routing][GatedBloomChain][tank]")
{
    sendbloom::GatedBloomChain chain;
    chain.prepare (kSampleRate, kBlockSize);

    const auto rt60 = sendbloom::ParameterCurves::sizeToRT60 (0.5f);
    const std::vector<float> burst (static_cast<size_t> (kWarmupSamples), 0.4f);
    const auto wet = renderChain (chain, burst, rt60, 0.0f, 1.0f, true);

    REQUIRE (sendbloom::test::rms (wet) > 1e-4f);
}

TEST_CASE ("GatedBloomChain processBlock matches processSample loop",
           "[chain][GatedBloomChain][INTEG-02][processBlock]")
{
    sendbloom::GatedBloomChain chainBlock;
    sendbloom::GatedBloomChain chainSample;
    chainBlock.prepare (kSampleRate, kBlockSize);
    chainSample.prepare (kSampleRate, kBlockSize);

    constexpr int kNumSamples = 128;
    std::vector<float> monoIn (static_cast<size_t> (kNumSamples));
    std::vector<float> envelope (static_cast<size_t> (kNumSamples));

    for (int i = 0; i < kNumSamples; ++i)
    {
        monoIn[static_cast<size_t> (i)] = 0.35f * std::sin (0.03f * static_cast<float> (i));
        envelope[static_cast<size_t> (i)] =
            chainBlock.getEnvelope().process (std::abs (monoIn[static_cast<size_t> (i)]));
    }

    const auto rt60 = sendbloom::ParameterCurves::sizeToRT60 (0.5f);
    constexpr float kDistnBlend = 0.3f;
    constexpr float kSendGain = 1.0f;
    constexpr bool kGatePreSoft = true;

    for (int i = 0; i < 2000; ++i)
    {
        const auto env = chainBlock.getEnvelope().process (0.5f);
        chainBlock.processSample (0.5f, env, rt60, 0.0f, false, kDistnBlend, kSendGain, kGatePreSoft, kThresholdDb);
        chainSample.processSample (0.5f, env, rt60, 0.0f, false, kDistnBlend, kSendGain, kGatePreSoft, kThresholdDb);
    }

    std::vector<float> blockOut (static_cast<size_t> (kNumSamples));
    std::vector<float> sampleOut (static_cast<size_t> (kNumSamples));

    chainBlock.processBlock (monoIn.data(), envelope.data(), blockOut.data(), kNumSamples,
                             rt60, 0.0f, false, kDistnBlend, kSendGain, kGatePreSoft, kThresholdDb);

    for (int i = 0; i < kNumSamples; ++i)
        sampleOut[static_cast<size_t> (i)] = chainSample.processSample (
            monoIn[static_cast<size_t> (i)], envelope[static_cast<size_t> (i)],
            rt60, 0.0f, false, kDistnBlend, kSendGain, kGatePreSoft, kThresholdDb);

    float maxDiff = 0.0f;
    for (int i = 0; i < kNumSamples; ++i)
        maxDiff = std::max (maxDiff,
                            std::abs (blockOut[static_cast<size_t> (i)]
                                      - sampleOut[static_cast<size_t> (i)]));

    REQUIRE (sendbloom::test::rms (sampleOut) > 1e-6f);
    REQUIRE (maxDiff < 1e-5f);
}

TEST_CASE ("GatedBloomChain authentic block produces finite output",
           "[chain][GatedBloomChain][INTEG-02][authentic-block]")
{
    sendbloom::GatedBloomChain chain;
    chain.prepare (kSampleRate, kBlockSize);

    constexpr int kTotalSamples = 8192;
    const auto rt60 = sendbloom::ParameterCurves::sizeToRT60 (0.5f);

    std::vector<float> monoBlock (static_cast<size_t> (kBlockSize), 0.0f);
    std::vector<float> envBlock (static_cast<size_t> (kBlockSize), 0.0f);
    std::vector<float> wetOut (static_cast<size_t> (kTotalSamples), 0.0f);

    for (int offset = 0; offset < kTotalSamples; offset += kBlockSize)
    {
        const int n = std::min (kBlockSize, kTotalSamples - offset);

        for (int i = 0; i < n; ++i)
        {
            monoBlock[static_cast<size_t> (i)] = (offset + i) < 480 ? 1.0f : 0.0f;
            envBlock[static_cast<size_t> (i)] =
                chain.getEnvelope().process (std::abs (monoBlock[static_cast<size_t> (i)]));
        }

        chain.processBlock (monoBlock.data(), envBlock.data(), wetOut.data() + offset, n,
                            rt60, 0.0f, true, 0.0f, 1.0f, true, kThresholdDb);
    }

    auto peak = 0.0f;
    for (int i = 64; i < kTotalSamples; ++i)
    {
        const auto w = wetOut[static_cast<size_t> (i)];
        REQUIRE (std::isfinite (w));
        peak = std::max (peak, std::abs (w));
    }

    REQUIRE (peak > 1e-6f);
}
