#include "Authentic32Mode.h"
#include "FixedRateAdapter.h"
#include "ParameterCurves.h"
#include "RateConverterPair.h"
#include "SchroederTank32DelayTable.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
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
