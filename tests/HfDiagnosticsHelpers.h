#pragma once

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

} // namespace sendbloom::test
