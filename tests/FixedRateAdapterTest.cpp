#include "Authentic32Mode.h"
#include "ChainTestHelpers.h"
#include "FixedRateAdapter.h"
#include "LegacyAccumulatorPath.h"
#include "ParameterCurves.h"
#include "RateConverterPair.h"
#include "SrcLatencyTable.h"
#include "ReverbTestHelpers.h"
#include "SchroederTank32.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <random>
#include <sstream>
#include <string>
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

std::vector<float> renderAdapterImpulseOnPrepared (sendbloom::FixedRateAdapter& adapter,
                                                   sendbloom::Authentic32Mode mode,
                                                   int maxBlock,
                                                   int blockSize,
                                                   int totalSamples)
{
    const auto rt60 = sendbloom::ParameterCurves::sizeToRT60 (0.5f);

    std::vector<float> ir;
    ir.reserve (static_cast<size_t> (totalSamples));

    std::vector<float> inBlock (static_cast<size_t> (maxBlock), 0.0f);
    std::vector<float> outBlock (static_cast<size_t> (maxBlock), 0.0f);

    int globalSample = 0;

    while (static_cast<int> (ir.size()) < totalSamples)
    {
        const int remaining = totalSamples - static_cast<int> (ir.size());
        const int n = std::min (blockSize, remaining);

        std::fill (inBlock.begin(), inBlock.begin() + n, 0.0f);
        if (globalSample == 0)
            inBlock[0] = 1.0f;

        adapter.processBlock (inBlock.data(), outBlock.data(), n, rt60, 0.0f, mode);

        for (int i = 0; i < n && static_cast<int> (ir.size()) < totalSamples; ++i)
            ir.push_back (outBlock[static_cast<size_t> (i)]);

        globalSample += n;
    }

    return ir;
}

std::vector<float> renderAdapterImpulse (sendbloom::FixedRateAdapter& adapter,
                                         sendbloom::Authentic32Mode mode,
                                         double hostRate,
                                         int maxBlock,
                                         int blockSize,
                                         int totalSamples)
{
    adapter.prepare (hostRate, maxBlock);
    return renderAdapterImpulseOnPrepared (adapter, mode, maxBlock, blockSize, totalSamples);
}

std::vector<float> renderAdapterImpulse (sendbloom::Authentic32Mode mode,
                                         double hostRate = 48000.0,
                                         int maxBlock = 512,
                                         int blockSize = 512,
                                         int totalSamples = 24000)
{
    sendbloom::FixedRateAdapter adapter;
    return renderAdapterImpulse (adapter, mode, hostRate, maxBlock, blockSize, totalSamples);
}

std::vector<float> makeGuitarPluck (size_t totalSamples, double sampleRate = 48000.0)
{
    std::vector<float> signal (totalSamples, 0.0f);
    constexpr auto kBurstLen = 240;

    for (int i = 0; i < kBurstLen; ++i)
    {
        const auto t = static_cast<float> (i) / static_cast<float> (sampleRate);
        const auto env = std::exp (-t * 18.0f);
        signal[static_cast<size_t> (i)] = env * (0.6f * std::sin (2.0f * 3.14159265358979323846f * 330.0f * t)
                                                 + 0.25f * std::sin (2.0f * 3.14159265358979323846f * 660.0f * t)
                                                 + 0.15f * std::sin (2.0f * 3.14159265358979323846f * 990.0f * t));
    }

    return signal;
}

std::vector<float> renderAdapterInput (sendbloom::Authentic32Mode mode,
                                       const std::vector<float>& input,
                                       double hostRate = 48000.0,
                                       int maxBlock = 512,
                                       int blockSize = 512)
{
    sendbloom::FixedRateAdapter adapter;
    adapter.prepare (hostRate, maxBlock);

    const auto rt60 = sendbloom::ParameterCurves::sizeToRT60 (0.5f);

    std::vector<float> out (input.size(), 0.0f);
    std::vector<float> inBlock (static_cast<size_t> (maxBlock), 0.0f);
    std::vector<float> outBlock (static_cast<size_t> (maxBlock), 0.0f);

    for (size_t offset = 0; offset < input.size(); offset += static_cast<size_t> (blockSize))
    {
        const int n = static_cast<int> (std::min (static_cast<size_t> (blockSize), input.size() - offset));

        for (int i = 0; i < n; ++i)
            inBlock[static_cast<size_t> (i)] = input[offset + static_cast<size_t> (i)];

        adapter.processBlock (inBlock.data(), outBlock.data(), n, rt60, 0.0f, mode);

        for (int i = 0; i < n; ++i)
            out[offset + static_cast<size_t> (i)] = outBlock[static_cast<size_t> (i)];
    }

    return out;
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

TEST_CASE ("RateConverterPair round-trip latency at four host rates",
           "[verb][LAT-01]")
{
    for (const auto& row : sendbloom::kMeasuredLatencyTable)
    {
        sendbloom::RateConverterPair converter;
        converter.prepare (row.hostRateHz, sendbloom::kMaxHostBlock);
        REQUIRE (converter.getRoundTripLatencySamples() == row.roundTripSamples);
    }
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

TEST_CASE ("LegacyAccumulatorPath matches SchroederTank32 authentic impulse render",
           "[verb][FixedRateAdapter][LegacyAccumulator][SRC-05]")
{
    sendbloom::LegacyAccumulatorPath legacy;
    sendbloom::SchroederTank32 tank;
    legacy.prepare (48000.0, 512);
    tank.prepare (48000.0, 512);
    tank.setAuthentic32ModeForDiagnostics (sendbloom::Authentic32Mode::LegacyAccumulator);

    constexpr int numSamples = 24000;
    constexpr int kBlockSize = 512;
    const auto rt60 = sendbloom::ParameterCurves::sizeToRT60 (0.5f);

    std::vector<float> inBlock (static_cast<size_t> (kBlockSize), 0.0f);
    std::vector<float> legacyOutBlock (static_cast<size_t> (kBlockSize), 0.0f);
    std::vector<float> tankOutBlock (static_cast<size_t> (kBlockSize), 0.0f);

    float maxLegacyTail = 0.0f;
    float maxTankTail = 0.0f;

    for (int offset = 0; offset < numSamples; offset += kBlockSize)
    {
        const int n = std::min (kBlockSize, numSamples - offset);

        for (int i = 0; i < n; ++i)
            inBlock[static_cast<size_t> (i)] = (offset + i) == 0 ? 1.0f : 0.0f;

        legacy.processBlock (inBlock.data(), legacyOutBlock.data(), n, rt60, 0.0f);
        tank.processBlock (inBlock.data(), tankOutBlock.data(), n, rt60, 0.0f, true);

        for (int i = 0; i < n; ++i)
        {
            const int globalIdx = offset + i;
            REQUIRE (legacyOutBlock[static_cast<size_t> (i)]
                     == Catch::Approx (tankOutBlock[static_cast<size_t> (i)]).margin (1.0e-5f));

            if (globalIdx >= 64)
            {
                maxLegacyTail = std::max (maxLegacyTail, std::abs (legacyOutBlock[static_cast<size_t> (i)]));
                maxTankTail = std::max (maxTankTail, std::abs (tankOutBlock[static_cast<size_t> (i)]));
            }
        }
    }

    tank.clearAuthentic32ModeForDiagnostics();
    REQUIRE (maxLegacyTail == Catch::Approx (maxTankTail).margin (1.0e-6f));
}

TEST_CASE ("LegacyAccumulatorPath burst input produces tank-matched tail energy",
           "[verb][FixedRateAdapter][LegacyAccumulator][SRC-05][burst]")
{
    sendbloom::LegacyAccumulatorPath legacy;
    sendbloom::SchroederTank32 tank;
    legacy.prepare (48000.0, 512);
    tank.prepare (48000.0, 512);
    tank.setAuthentic32ModeForDiagnostics (sendbloom::Authentic32Mode::LegacyAccumulator);

    constexpr int numSamples = 24000;
    constexpr int kBlockSize = 512;
    const auto rt60 = sendbloom::ParameterCurves::sizeToRT60 (0.5f);

    std::vector<float> inBlock (static_cast<size_t> (kBlockSize), 0.0f);
    std::vector<float> legacyOutBlock (static_cast<size_t> (kBlockSize), 0.0f);
    std::vector<float> tankOutBlock (static_cast<size_t> (kBlockSize), 0.0f);

    float maxLegacyTail = 0.0f;

    for (int offset = 0; offset < numSamples; offset += kBlockSize)
    {
        const int n = std::min (kBlockSize, numSamples - offset);

        for (int i = 0; i < n; ++i)
            inBlock[static_cast<size_t> (i)] = (offset + i) < 480 ? 1.0f : 0.0f;

        legacy.processBlock (inBlock.data(), legacyOutBlock.data(), n, rt60, 0.0f);
        tank.processBlock (inBlock.data(), tankOutBlock.data(), n, rt60, 0.0f, true);

        for (int i = 0; i < n; ++i)
        {
            const int globalIdx = offset + i;
            REQUIRE (legacyOutBlock[static_cast<size_t> (i)]
                     == Catch::Approx (tankOutBlock[static_cast<size_t> (i)]).margin (1.0e-5f));

            if (globalIdx >= 64)
                maxLegacyTail = std::max (maxLegacyTail, std::abs (legacyOutBlock[static_cast<size_t> (i)]));
        }
    }

    tank.clearAuthentic32ModeForDiagnostics();
    REQUIRE (maxLegacyTail > 1.0e-4f);
}

TEST_CASE ("FixedRateAdapter routes Authentic32Mode Off vs LegacyAccumulator",
           "[verb][Authentic32Mode][SRC-04]")
{
    sendbloom::FixedRateAdapter adapter;
    adapter.prepare (48000.0, 512);

    constexpr int numSamples = 8192;
    constexpr int kBlockSize = 512;
    std::vector<float> in (static_cast<size_t> (kBlockSize), 0.0f);

    std::vector<float> offOut (static_cast<size_t> (numSamples), -1.0f);
    std::vector<float> legacyOut (static_cast<size_t> (numSamples), 0.0f);
    std::vector<float> properOut (static_cast<size_t> (numSamples), -1.0f);

    const auto rt60 = sendbloom::ParameterCurves::sizeToRT60 (0.5f);

    for (int offset = 0; offset < numSamples; offset += kBlockSize)
    {
        const int n = std::min (kBlockSize, numSamples - offset);

        for (int i = 0; i < n; ++i)
            in[static_cast<size_t> (i)] = (offset + i) < 480 ? 1.0f : 0.0f;

        adapter.processBlock (in.data(),
                              offOut.data() + offset,
                              n,
                              rt60,
                              0.0f,
                              sendbloom::Authentic32Mode::Off);
    }

    adapter.reset();

    for (int offset = 0; offset < numSamples; offset += kBlockSize)
    {
        const int n = std::min (kBlockSize, numSamples - offset);

        for (int i = 0; i < n; ++i)
            in[static_cast<size_t> (i)] = (offset + i) < 480 ? 1.0f : 0.0f;

        adapter.processBlock (in.data(),
                              legacyOut.data() + offset,
                              n,
                              rt60,
                              0.0f,
                              sendbloom::Authentic32Mode::LegacyAccumulator);
    }

    adapter.reset();

    for (int offset = 0; offset < numSamples; offset += kBlockSize)
    {
        const int n = std::min (kBlockSize, numSamples - offset);

        for (int i = 0; i < n; ++i)
            in[static_cast<size_t> (i)] = (offset + i) < 480 ? 1.0f : 0.0f;

        adapter.processBlock (in.data(),
                              properOut.data() + offset,
                              n,
                              rt60,
                              0.0f,
                              sendbloom::Authentic32Mode::ProperSRC);
    }

    REQUIRE (sendbloom::test::rms (offOut) == 0.0f);

    float maxLegacy = 0.0f;
    float maxProper = 0.0f;

    for (int i = 64; i < numSamples; ++i)
    {
        maxLegacy = std::max (maxLegacy, std::abs (legacyOut[static_cast<size_t> (i)]));
        maxProper = std::max (maxProper, std::abs (properOut[static_cast<size_t> (i)]));
    }

    REQUIRE (maxLegacy > 1.0e-4f);
    REQUIRE (maxProper > 1.0e-4f);
}

TEST_CASE ("FixedRateAdapter ProperSRC impulse at 48 kHz produces finite bright tail",
           "[verb][FixedRateAdapter][ProperSRC][48]")
{
    const auto ir = renderAdapterImpulse (sendbloom::Authentic32Mode::ProperSRC);

    REQUIRE (static_cast<int> (ir.size()) == 24000);

    for (const auto sample : ir)
        REQUIRE (std::isfinite (sample));

    float maxTail = 0.0f;

    for (int i = 64; i < static_cast<int> (ir.size()); ++i)
        maxTail = std::max (maxTail, std::abs (ir[static_cast<size_t> (i)]));

    REQUIRE (maxTail > 1.0e-4f);
}

TEST_CASE ("FixedRateAdapter ProperSRC impulse is finite at four host rates",
           "[verb][FixedRateAdapter][ProperSRC][multiRate]")
{
    constexpr std::array<double, 4> kHostRates { 44100.0, 48000.0, 88200.0, 96000.0 };
    constexpr int kMaxBlock = 512;
    constexpr int kBlockSize = 256;
    constexpr int kTotalSamples = 4096;

    for (const auto hostRate : kHostRates)
    {
        const auto ir = renderAdapterImpulse (sendbloom::Authentic32Mode::ProperSRC,
                                              hostRate,
                                              kMaxBlock,
                                              kBlockSize,
                                              kTotalSamples);

        REQUIRE (static_cast<int> (ir.size()) == kTotalSamples);

        for (const auto sample : ir)
            REQUIRE (std::isfinite (sample));
    }
}

TEST_CASE ("FixedRateAdapter ProperSRC tolerates variable block sizes",
           "[verb][FixedRateAdapter][ProperSRC][variableBlock]")
{
    constexpr double kHostRate = 48000.0;
    constexpr int kMaxBlock = 512;
    constexpr int kTotalSamples = 2000;
    constexpr std::array<int, 4> kBlockSizes { 1, 7, 64, 512 };

    sendbloom::FixedRateAdapter adapter;
    adapter.prepare (kHostRate, kMaxBlock);

    const auto rt60 = sendbloom::ParameterCurves::sizeToRT60 (0.5f);

    std::vector<float> ir;
    ir.reserve (static_cast<size_t> (kTotalSamples));

    std::vector<float> inBlock (static_cast<size_t> (kMaxBlock), 0.0f);
    std::vector<float> outBlock (static_cast<size_t> (kMaxBlock), 0.0f);

    size_t blockIndex = 0;
    int globalSample = 0;

    REQUIRE_NOTHROW ([&]() {
        while (static_cast<int> (ir.size()) < kTotalSamples)
        {
            const int blockSize = kBlockSizes[blockIndex % kBlockSizes.size()];
            ++blockIndex;

            const int remaining = kTotalSamples - static_cast<int> (ir.size());
            const int n = std::min (blockSize, remaining);

            std::fill (inBlock.begin(), inBlock.begin() + n, 0.0f);
            if (globalSample == 0)
                inBlock[0] = 1.0f;

            adapter.processBlock (inBlock.data(),
                                  outBlock.data(),
                                  n,
                                  rt60,
                                  0.0f,
                                  sendbloom::Authentic32Mode::ProperSRC);

            for (int i = 0; i < n && static_cast<int> (ir.size()) < kTotalSamples; ++i)
                ir.push_back (outBlock[static_cast<size_t> (i)]);

            globalSample += n;
        }
    }());

    for (const auto sample : ir)
        REQUIRE (std::isfinite (sample));
}

TEST_CASE ("FixedRateAdapter ProperSRC reduces 14825 Hz imaging vs LegacyAccumulator",
           "[verb][FixedRateAdapter][SRC-06]")
{
    constexpr size_t tailStart = 19200;
    constexpr size_t tailCount = 2400;
    constexpr double imagingHz = 14825.0;
    constexpr double sampleRate = 48000.0;

    // Guitar pluck excites accumulator imaging in the 14–15 kHz band; impulse tail is
    // masked by the legacy anti-image SVF at 14825 Hz (HF diagnostics use same fixture).
    const auto input = makeGuitarPluck (24000, sampleRate);
    const auto legacyIr = renderAdapterInput (sendbloom::Authentic32Mode::LegacyAccumulator, input);
    const auto properIr = renderAdapterInput (sendbloom::Authentic32Mode::ProperSRC, input);

    const auto legacyPower = sendbloom::test::goertzelPower (legacyIr,
                                                              sampleRate,
                                                              imagingHz,
                                                              tailStart,
                                                              tailCount);
    const auto properPower = sendbloom::test::goertzelPower (properIr,
                                                               sampleRate,
                                                               imagingHz,
                                                               tailStart,
                                                               tailCount);

    INFO ("legacy 14825 Hz tail RMS = " << std::sqrt (legacyPower / static_cast<double> (tailCount)));
    INFO ("proper 14825 Hz tail RMS = " << std::sqrt (properPower / static_cast<double> (tailCount)));

    REQUIRE (legacyPower > 0.0);
    REQUIRE (properPower <= legacyPower * 0.30);
}

TEST_CASE ("FixedRateAdapter reset restores impulse parity for legacy and ProperSRC",
           "[verb][FixedRateAdapter][reset][TEST-10]")
{
    constexpr int kMaxBlock = 512;
    constexpr int kTotalSamples = 24000;

    const std::array modes {
        sendbloom::Authentic32Mode::LegacyAccumulator,
        sendbloom::Authentic32Mode::ProperSRC,
    };

    for (const auto mode : modes)
    {
        sendbloom::FixedRateAdapter adapter;
        adapter.prepare (48000.0, kMaxBlock);
        const auto ir1 = renderAdapterImpulseOnPrepared (adapter, mode, kMaxBlock, 512, kTotalSamples);

        adapter.reset();
        const auto ir2 = renderAdapterImpulseOnPrepared (adapter, mode, kMaxBlock, 512, kTotalSamples);

        REQUIRE (sendbloom::test::reverb::maxAbsDiff (ir1, ir2) < 1.0e-4f);
    }
}

TEST_CASE ("FixedRateAdapter ProperSRC realtime stress with random block sizes",
           "[verb][FixedRateAdapter][realtime][SRC-02]")
{
    sendbloom::FixedRateAdapter adapter;
    adapter.prepare (48000.0, 512);

    const auto rt60 = sendbloom::ParameterCurves::sizeToRT60 (0.5f);

    std::mt19937 rng (0x13'04'2026u);
    std::uniform_int_distribution<int> blockDist (1, 512);
    std::uniform_real_distribution<float> noiseDist (-0.1f, 0.1f);

    std::vector<float> inBlock (512, 0.0f);
    std::vector<float> outBlock (512, 0.0f);

    REQUIRE_NOTHROW ([&]() {
        for (int iter = 0; iter < 10000; ++iter)
        {
            const int n = blockDist (rng);

            for (int i = 0; i < n; ++i)
                inBlock[static_cast<size_t> (i)] = noiseDist (rng);

            adapter.processBlock (inBlock.data(),
                                  outBlock.data(),
                                  n,
                                  rt60,
                                  0.0f,
                                  sendbloom::Authentic32Mode::ProperSRC);

            for (int i = 0; i < n; ++i)
                REQUIRE (std::isfinite (outBlock[static_cast<size_t> (i)]));
        }
    }());
}

std::string readAdapterHeaderSource()
{
    constexpr std::array<const char*, 3> kCandidates {
        "source/FixedRateAdapter.h",
        "../source/FixedRateAdapter.h",
        "../../source/FixedRateAdapter.h",
    };

    for (const auto* path : kCandidates)
    {
        std::ifstream header (path);
        if (header.is_open())
        {
            std::ostringstream source;
            source << header.rdbuf();
            return source.str();
        }
    }

    return {};
}

TEST_CASE ("FixedRateAdapter processBlock has no heap allocation tokens",
           "[verb][FixedRateAdapter][SRC-02][static]")
{
    const auto text = readAdapterHeaderSource();
    REQUIRE_FALSE (text.empty());

    const auto processBlockPos = text.find ("void processBlock");
    REQUIRE (processBlockPos != std::string::npos);

    const auto bodyStart = text.find ('{', processBlockPos);
    REQUIRE (bodyStart != std::string::npos);

    const auto nextPublic = text.find ("\npublic:", bodyStart);
    const auto nextPrivate = text.find ("\nprivate:", bodyStart);
    auto bodyEnd = std::min (nextPublic, nextPrivate);
    if (bodyEnd == std::string::npos)
        bodyEnd = text.size();

    const auto body = text.substr (bodyStart, bodyEnd - bodyStart);

    auto stripComments = [] (std::string s) {
        for (;;)
        {
            const auto block = s.find ("/*");
            if (block == std::string::npos)
                break;

            const auto end = s.find ("*/", block + 2);
            if (end == std::string::npos)
                break;

            s.erase (block, end - block + 2);
        }

        for (;;)
        {
            const auto line = s.find ("//");
            if (line == std::string::npos)
                break;

            const auto end = s.find ('\n', line);
            if (end == std::string::npos)
            {
                s.erase (line);
                break;
            }

            s.erase (line, end - line);
        }

        return s;
    };

    const auto stripped = stripComments (body);

    REQUIRE (stripped.find ("make_unique") == std::string::npos);
    REQUIRE (stripped.find (".resize(") == std::string::npos);
}
