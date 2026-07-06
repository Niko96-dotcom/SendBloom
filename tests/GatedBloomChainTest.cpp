#include "ChainTestHelpers.h"
#include <GatedBloomChain.h>
#include <ParallelWetMixer.h>
#include <ParameterCurves.h>
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

    const std::vector<float> burst (static_cast<size_t> (kWarmupSamples), 0.5f);
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

    for (int i = 0; i < 240; ++i)
        wet.push_back (processChainSample (chain, 0.0f, rt60, 0.0f, 0.0f, true));

    const auto tailSlice = std::vector<float> (wet.end() - 240, wet.end());
    REQUIRE (sendbloom::test::rms (tailSlice) > 1e-4f);
    REQUIRE (sendbloom::test::rms (tailSlice) > 1e-6f);
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
