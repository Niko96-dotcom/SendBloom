#pragma once

#include <ParameterCurves.h>
#include <SchroederTank32.h>
#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

namespace sendbloom::test
{

inline constexpr auto kSampleRate = 48000.0;
inline constexpr auto kBlockSize = 512;
inline constexpr auto kRenderSamples = 24000uz;
inline constexpr auto kTailStart = kRenderSamples - 4800;
inline constexpr auto kTailCount = 2400uz;

inline std::vector<float> makeGuitarPluck (size_t totalSamples) noexcept
{
    std::vector<float> signal (totalSamples, 0.0f);
    constexpr auto kBurstLen = 240;

    for (int i = 0; i < kBurstLen; ++i)
    {
        const auto t = static_cast<float> (i) / static_cast<float> (kSampleRate);
        const auto env = std::exp (-t * 18.0f);
        signal[static_cast<size_t> (i)] = env * (0.6f * std::sin (2.0f * 3.14159265358979323846f * 330.0f * t)
                                                 + 0.25f * std::sin (2.0f * 3.14159265358979323846f * 660.0f * t)
                                                 + 0.15f * std::sin (2.0f * 3.14159265358979323846f * 990.0f * t));
    }

    return signal;
}

inline std::vector<float> makeDecayingSine (float hz, size_t totalSamples, int burstLen = 480) noexcept
{
    std::vector<float> signal (totalSamples, 0.0f);
    const auto phaseInc = 2.0f * 3.14159265358979323846f * hz / static_cast<float> (kSampleRate);

    for (int i = 0; i < burstLen; ++i)
    {
        const auto env = std::exp (-static_cast<float> (i) / 120.0f);
        signal[static_cast<size_t> (i)] = 0.35f * env * std::sin (phaseInc * static_cast<float> (i));
    }

    return signal;
}

inline std::vector<float> makeImpulseBurst (size_t totalSamples) noexcept
{
    std::vector<float> signal (totalSamples, 0.0f);
    signal[0] = 1.0f;
    return signal;
}

inline std::vector<float> makeSweptSine (size_t totalSamples) noexcept
{
    std::vector<float> signal (totalSamples, 0.0f);
    constexpr auto kBurstLen = 4800;
    constexpr auto f0 = 200.0f;
    constexpr auto f1 = 8000.0f;
    auto phase = 0.0f;

    for (int i = 0; i < kBurstLen; ++i)
    {
        const auto t = static_cast<float> (i) / static_cast<float> (kBurstLen - 1);
        const auto env = std::exp (-static_cast<float> (i) / 800.0f);
        const auto freq = f0 + t * (f1 - f0);
        phase += 2.0f * 3.14159265358979323846f * freq / static_cast<float> (kSampleRate);
        signal[static_cast<size_t> (i)] = 0.3f * env * std::sin (phase);
    }

    return signal;
}

inline std::vector<std::pair<std::string, std::vector<float>>> allFixtures()
{
    return {
        { "guitar_pluck", makeGuitarPluck (kRenderSamples) },
        { "sine220_decay", makeDecayingSine (220.0f, kRenderSamples) },
        { "sine880_decay", makeDecayingSine (880.0f, kRenderSamples) },
        { "impulse", makeImpulseBurst (kRenderSamples) },
        { "swept_sine", makeSweptSine (kRenderSamples) },
    };
}

enum class ReverbPath
{
    HostRate,
    LegacyAccumulator,
    ProperSRC
};

inline void applyPathDiagnostics (sendbloom::SchroederTank32& tank, ReverbPath path) noexcept
{
    switch (path)
    {
        case ReverbPath::LegacyAccumulator:
            tank.setAuthentic32ModeForDiagnostics (sendbloom::Authentic32Mode::LegacyAccumulator);
            break;
        case ReverbPath::ProperSRC:
        case ReverbPath::HostRate:
            tank.clearAuthentic32ModeForDiagnostics();
            break;
    }
}

inline std::vector<float> renderTankPath (sendbloom::SchroederTank32& tank,
                                          const std::vector<float>& input,
                                          ReverbPath path,
                                          double sampleRate,
                                          int blockSize)
{
    tank.prepare (sampleRate, blockSize);
    applyPathDiagnostics (tank, path);

    const auto rt60 = sendbloom::ParameterCurves::sizeToRT60 (0.5f);
    const auto authentic = path != ReverbPath::HostRate;

    std::vector<float> out (input.size(), 0.0f);
    std::vector<float> inBlock (static_cast<size_t> (blockSize), 0.0f);
    std::vector<float> outBlock (static_cast<size_t> (blockSize), 0.0f);

    for (size_t offset = 0; offset < input.size(); offset += static_cast<size_t> (blockSize))
    {
        const int n = static_cast<int> (std::min (static_cast<size_t> (blockSize), input.size() - offset));
        std::copy_n (input.begin() + static_cast<std::ptrdiff_t> (offset), n, inBlock.begin());
        tank.processBlock (inBlock.data(), outBlock.data(), n, rt60, 0.0f, authentic);
        std::copy_n (outBlock.begin(), n, out.begin() + static_cast<std::ptrdiff_t> (offset));
    }

    return out;
}

} // namespace sendbloom::test
