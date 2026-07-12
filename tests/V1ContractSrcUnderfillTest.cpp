#include "Authentic32Mode.h"
#include "FixedRateAdapter.h"
#include "ParameterCurves.h"
#include "RateConverterPair.h"
#include "SchroederTank32DelayTable.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace
{

constexpr float kSentinel = 123456.0f;

int measureDownsampleWritten (sendbloom::RateConverterPair& converters, int nHost)
{
    std::vector<float> hostIn (static_cast<size_t> (nHost), 0.0f);
    std::vector<double> internal (static_cast<size_t> (converters.getMaxUpsampledLen (nHost)), 0.0);

    const int nInternal = converters.upsample (hostIn.data(), nHost, internal.data());

    std::vector<float> out (static_cast<size_t> (nHost), kSentinel);
    std::fill (out.begin(), out.end(), 0.0f);
    return converters.downsample (internal.data(), nInternal, out.data(), nHost);
}

std::vector<float> renderRoundTrip (const std::vector<int>& blockPattern)
{
    constexpr int kTotalSamples = 24000;
    constexpr int kMaxBlock = 512;
    constexpr double kSampleRate = 48000.0;

    sendbloom::RateConverterPair converters;
    converters.prepare (kSampleRate, kMaxBlock);

    std::vector<float> input (kMaxBlock, 0.0f);
    std::vector<float> output (kMaxBlock, 0.0f);
    std::vector<double> internal (
        static_cast<size_t> (converters.getMaxUpsampledLen (kMaxBlock)), 0.0);
    std::vector<float> rendered (kTotalSamples, 0.0f);

    int position = 0;
    size_t patternIndex = 0;

    while (position < kTotalSamples)
    {
        const auto requested = blockPattern[patternIndex++ % blockPattern.size()];
        const auto blockSize = std::min (requested, kTotalSamples - position);

        for (int i = 0; i < blockSize; ++i)
            input[static_cast<size_t> (i)] = static_cast<float> (
                std::sin (2.0 * juce::MathConstants<double>::pi * 440.0
                          * static_cast<double> (position + i) / kSampleRate));

        std::fill (output.begin(), output.end(), 0.0f);
        const auto internalCount = converters.upsample (input.data(), blockSize, internal.data());
        converters.downsample (internal.data(), internalCount, output.data(), blockSize);
        std::copy_n (output.data(), blockSize, rendered.data() + position);
        position += blockSize;
    }

    return rendered;
}

} // namespace

TEST_CASE ("RateConverterPair may underfill host output on early blocks",
           "[v1][contract][src-underfill][DSP-06]")
{
    sendbloom::RateConverterPair converters;
    converters.prepare (48000.0, 512);

    const int written = measureDownsampleWritten (converters, 48);
    REQUIRE (written >= 0);
    REQUIRE (written <= 48);
}

TEST_CASE ("FixedRateAdapter ProperSRC pre-clears output before downsample",
           "[v1][contract][src-underfill][DSP-06]")
{
    sendbloom::FixedRateAdapter adapter;
    adapter.prepare (48000.0, 512);

    const auto rt60 = sendbloom::ParameterCurves::sizeToRT60 (0.5f);
    constexpr int n = 48;

    std::vector<float> in (static_cast<size_t> (n), 0.0f);
    std::vector<float> out (static_cast<size_t> (n), kSentinel);

    adapter.processBlock (in.data(),
                          out.data(),
                          n,
                          rt60,
                          0.0f,
                          sendbloom::Authentic32Mode::ProperSRC);

    for (const auto sample : out)
        REQUIRE (sample != Catch::Approx (kSentinel));
}

TEST_CASE ("ProperSRC unwritten downsample tail remains zero",
           "[v1][contract][src-underfill][DSP-07]")
{
    sendbloom::RateConverterPair converters;
    converters.prepare (48000.0, 512);

    constexpr int n = 48;
    const int written = measureDownsampleWritten (converters, n);

    REQUIRE (written < n);

    sendbloom::FixedRateAdapter adapter;
    adapter.prepare (48000.0, 512);

    const auto rt60 = sendbloom::ParameterCurves::sizeToRT60 (0.5f);
    std::vector<float> in (static_cast<size_t> (n), 0.0f);
    std::vector<float> out (static_cast<size_t> (n), kSentinel);

    adapter.processBlock (in.data(),
                          out.data(),
                          n,
                          rt60,
                          0.0f,
                          sendbloom::Authentic32Mode::ProperSRC);

    for (int i = written; i < n; ++i)
        REQUIRE (out[static_cast<size_t> (i)] == 0.0f);
}

TEST_CASE ("RateConverterPair round trip is invariant to variable host block boundaries",
           "[v1][contract][src][variableBlock][regression]")
{
    const auto fixed = renderRoundTrip ({ 512 });
    const auto variable = renderRoundTrip ({ 1, 7, 64, 512 });

    REQUIRE (fixed.size() == variable.size());

    // Ignore filter priming and compare the steady stream sample-for-sample. Before the
    // regression fix, the variable render inserted zero samples with errors near full scale.
    constexpr size_t kPrimingGuard = 4000;
    float maxDifference = 0.0f;
    double squaredError = 0.0;

    for (size_t i = kPrimingGuard; i < fixed.size(); ++i)
    {
        const auto difference = std::abs (fixed[i] - variable[i]);
        maxDifference = std::max (maxDifference, difference);
        squaredError += static_cast<double> (difference) * difference;
    }

    const auto rmsError = std::sqrt (squaredError
                                     / static_cast<double> (fixed.size() - kPrimingGuard));
    REQUIRE (maxDifference < 0.01f);
    REQUIRE (rmsError < 0.001);
}
