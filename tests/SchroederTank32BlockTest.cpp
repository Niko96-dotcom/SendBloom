#include "ReverbTestHelpers.h"
#include <ParameterCurves.h>
#include <SchroederTank32.h>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <vector>

namespace
{

constexpr auto kSampleRate = 48000.0;
constexpr int kMaxBlock = 512;
constexpr int kBlockLen = 256;

std::vector<float> makeImpulseBlock (int numSamples)
{
    std::vector<float> block (static_cast<size_t> (numSamples), 0.0f);
    if (numSamples > 0)
        block[0] = 1.0f;
    return block;
}

std::vector<float> makeNoiseBlock (int numSamples)
{
    std::vector<float> block (static_cast<size_t> (numSamples), 0.0f);
    for (int i = 0; i < numSamples; ++i)
        block[static_cast<size_t> (i)] = 0.1f * std::sin (static_cast<float> (i) * 0.017f);
    return block;
}

std::vector<float> renderSampleLoop (sendbloom::SchroederTank32& tank,
                                     const std::vector<float>& input,
                                     float rt60,
                                     float darkMix)
{
    std::vector<float> out (input.size(), 0.0f);
    for (size_t i = 0; i < input.size(); ++i)
        out[i] = tank.processSample (input[i], rt60, darkMix);
    return out;
}

} // namespace

TEST_CASE ("SchroederTank32 fixed-rate processBlock matches sample loop", "[verb][SchroederTank32][INTEG-01][processBlock]")
{
    sendbloom::SchroederTank32 blockTank;
    sendbloom::SchroederTank32 sampleTank;
    blockTank.prepare (kSampleRate, kMaxBlock);
    sampleTank.prepare (kSampleRate, kMaxBlock);

    const auto rt60 = sendbloom::ParameterCurves::sizeToRT60 (0.5f);
    const auto input = makeImpulseBlock (kBlockLen);

    std::vector<float> blockOut (static_cast<size_t> (kBlockLen), 0.0f);
    blockTank.processBlock (input.data(), blockOut.data(), kBlockLen, rt60, 0.0f);

    const auto sampleOut = renderSampleLoop (sampleTank, input, rt60, 0.0f);

    REQUIRE (sendbloom::test::reverb::maxAbsDiff (blockOut, sampleOut) < 1.0e-5f);
}

TEST_CASE ("SchroederTank32 processBlock ProperSRC stays finite", "[verb][SchroederTank32][INTEG-01][processBlock]")
{
    sendbloom::SchroederTank32 tank;
    tank.prepare (kSampleRate, kMaxBlock);

    const auto rt60 = sendbloom::ParameterCurves::sizeToRT60 (0.5f);
    const auto input = makeNoiseBlock (kBlockLen);

    std::vector<float> blockOut (static_cast<size_t> (kBlockLen), 0.0f);
    tank.processBlock (input.data(), blockOut.data(), kBlockLen, rt60, 0.0f);

    auto peak = 0.0f;
    for (const auto sample : blockOut)
    {
        REQUIRE (std::isfinite (sample));
        peak = std::max (peak, std::abs (sample));
    }

    REQUIRE (peak < 4.0f);
}

TEST_CASE ("SchroederTank32 processBlock diagnostics mode changes output", "[verb][SchroederTank32][INTEG-01][processBlock]")
{
    sendbloom::SchroederTank32 properTank;
    sendbloom::SchroederTank32 legacyTank;
    properTank.prepare (kSampleRate, kMaxBlock);
    legacyTank.prepare (kSampleRate, kMaxBlock);
    legacyTank.setAuthentic32ModeForDiagnostics (sendbloom::Authentic32Mode::LegacyAccumulator);

    constexpr int numSamples = 8192;
    const auto rt60 = sendbloom::ParameterCurves::sizeToRT60 (0.5f);

    std::vector<float> inBlock (static_cast<size_t> (kMaxBlock), 0.0f);
    std::vector<float> properOut (static_cast<size_t> (numSamples), 0.0f);
    std::vector<float> legacyOut (static_cast<size_t> (numSamples), 0.0f);

    for (int offset = 0; offset < numSamples; offset += kMaxBlock)
    {
        const int n = std::min (kMaxBlock, numSamples - offset);

        for (int i = 0; i < n; ++i)
            inBlock[static_cast<size_t> (i)] = (offset + i) < 480 ? 1.0f : 0.0f;

        properTank.processBlock (inBlock.data(),
                                 properOut.data() + offset,
                                 n,
                                 rt60,
                                 0.0f);
        legacyTank.processBlock (inBlock.data(),
                                 legacyOut.data() + offset,
                                 n,
                                 rt60,
                                 0.0f);
    }

    auto properPeak = 0.0f;
    auto legacyPeak = 0.0f;

    for (int i = 64; i < numSamples; ++i)
    {
        properPeak = std::max (properPeak, std::abs (properOut[static_cast<size_t> (i)]));
        legacyPeak = std::max (legacyPeak, std::abs (legacyOut[static_cast<size_t> (i)]));
    }

    REQUIRE (properPeak > 1.0e-4f);
    REQUIRE (legacyPeak > 1.0e-4f);
    REQUIRE (sendbloom::test::reverb::maxAbsDiff (properOut, legacyOut) > 1.0e-4f);
}
