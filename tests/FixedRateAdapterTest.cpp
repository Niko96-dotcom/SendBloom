#include "ChainTestHelpers.h"
#include "RateConverterPair.h"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cmath>
#include <vector>

namespace
{

std::vector<float> renderConverterPassthrough (sendbloom::RateConverterPair& converter,
                                               double hostRate,
                                               int blockSize,
                                               int maxBlock,
                                               int totalSamples)
{
    converter.prepare (hostRate, maxBlock);

    std::vector<float> hostOut;
    hostOut.reserve (static_cast<size_t> (totalSamples));

    std::vector<float> hostBlock (static_cast<size_t> (maxBlock), 0.0f);
    std::vector<double> internalBlock (static_cast<size_t> (converter.getMaxUpsampledLen (maxBlock) + 64), 0.0);

    int globalSample = 0;

    while (static_cast<int> (hostOut.size()) < totalSamples)
    {
        const int remaining = totalSamples - static_cast<int> (hostOut.size());
        const int n = std::min (blockSize, remaining);

        std::fill (hostBlock.begin(), hostBlock.begin() + n, 0.0f);
        if (globalSample == 0)
            hostBlock[0] = 1.0f;

        const int nInternal = converter.upsample (hostBlock.data(), n, internalBlock.data());

        std::vector<float> downBlock (static_cast<size_t> (n), 0.0f);
        converter.downsample (internalBlock.data(), nInternal, downBlock.data(), n);

        for (int i = 0; i < n && static_cast<int> (hostOut.size()) < totalSamples; ++i)
            hostOut.push_back (downBlock[static_cast<size_t> (i)]);

        globalSample += n;
    }

    return hostOut;
}

} // namespace

TEST_CASE ("RateConverterPair passthrough is finite at multiple host rates",
           "[verb][FixedRateAdapter][multiRate][SRC-03]")
{
    constexpr std::array<double, 4> kHostRates { 44100.0, 48000.0, 88200.0, 96000.0 };
    constexpr int kMaxBlock = 512;
    constexpr int kBlockSize = 256;
    constexpr int kTotalSamples = 4096;

    for (const auto hostRate : kHostRates)
    {
        sendbloom::RateConverterPair converter;
        const auto output = renderConverterPassthrough (converter,
                                                        hostRate,
                                                        kBlockSize,
                                                        kMaxBlock,
                                                        kTotalSamples);

        REQUIRE (static_cast<int> (output.size()) == kTotalSamples);

        for (const auto sample : output)
            REQUIRE (std::isfinite (sample));
    }
}

TEST_CASE ("RateConverterPair tolerates variable block sizes",
           "[verb][FixedRateAdapter][variableBlock][SRC-03]")
{
    constexpr double kHostRate = 48000.0;
    constexpr int kMaxBlock = 512;
    constexpr int kTotalSamples = 2000;
    constexpr std::array<int, 4> kBlockSizes { 1, 7, 64, 512 };

    sendbloom::RateConverterPair converter;
    converter.prepare (kHostRate, kMaxBlock);

    std::vector<float> hostOut;
    hostOut.reserve (static_cast<size_t> (kTotalSamples));

    std::vector<float> hostBlock (static_cast<size_t> (kMaxBlock), 0.0f);
    std::vector<double> internalBlock (static_cast<size_t> (converter.getMaxUpsampledLen (kMaxBlock) + 64), 0.0);

    size_t blockIndex = 0;
    int globalSample = 0;

    REQUIRE_NOTHROW ([&]() {
        while (static_cast<int> (hostOut.size()) < kTotalSamples)
        {
            const int blockSize = kBlockSizes[blockIndex % kBlockSizes.size()];
            ++blockIndex;

            const int remaining = kTotalSamples - static_cast<int> (hostOut.size());
            const int n = std::min (blockSize, remaining);

            std::fill (hostBlock.begin(), hostBlock.begin() + n, 0.0f);
            if (globalSample == 0)
                hostBlock[0] = 1.0f;

            const int nInternal = converter.upsample (hostBlock.data(), n, internalBlock.data());

            std::vector<float> downBlock (static_cast<size_t> (n), 0.0f);
            converter.downsample (internalBlock.data(), nInternal, downBlock.data(), n);

            for (int i = 0; i < n && static_cast<int> (hostOut.size()) < kTotalSamples; ++i)
                hostOut.push_back (downBlock[static_cast<size_t> (i)]);

            globalSample += n;
        }
    }());

    for (const auto sample : hostOut)
        REQUIRE (std::isfinite (sample));
}

TEST_CASE ("RateConverterPair round-trip latency is stable after reset",
           "[verb][FixedRateAdapter][latency]")
{
    sendbloom::RateConverterPair converter;
    converter.prepare (48000.0, 512);

    const auto latencyBefore = converter.getRoundTripLatencySamples();
    REQUIRE (latencyBefore > 0);

    converter.reset();
    const auto latencyAfterReset = converter.getRoundTripLatencySamples();
    REQUIRE (latencyAfterReset == latencyBefore);

    converter.prepare (48000.0, 512);
    const auto latencyAfterReprepare = converter.getRoundTripLatencySamples();
    REQUIRE (latencyAfterReprepare == latencyBefore);
}
