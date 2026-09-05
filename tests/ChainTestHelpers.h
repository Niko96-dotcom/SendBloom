#pragma once

#include <cmath>
#include <vector>

namespace sendbloom::test
{

inline float rms (const std::vector<float>& samples)
{
    if (samples.empty())
        return 0.0f;

    double sum = 0.0;

    for (const auto s : samples)
        sum += static_cast<double> (s) * static_cast<double> (s);

    return static_cast<float> (std::sqrt (sum / static_cast<double> (samples.size())));
}

inline double goertzelPower (const std::vector<float>& samples,
                             double sampleRate,
                             double frequencyHz,
                             size_t start,
                             size_t count) noexcept
{
    if (count == 0 || start + count > samples.size())
        return 0.0;

    const auto omega = 2.0 * 3.14159265358979323846 * frequencyHz / sampleRate;
    const auto coeff = 2.0 * std::cos (omega);
    auto s1 = 0.0;
    auto s2 = 0.0;

    for (size_t i = start; i < start + count; ++i)
    {
        const auto s0 = static_cast<double> (samples[i]) + coeff * s1 - s2;
        s2 = s1;
        s1 = s0;
    }

    const auto real = s1 - s2 * std::cos (omega);
    const auto imag = s2 * std::sin (omega);
    return real * real + imag * imag;
}

inline float measureThd (const std::vector<float>& samples,
                         double sampleRate,
                         float fundamentalHz,
                         size_t start,
                         size_t count) noexcept
{
    const auto fundamental = goertzelPower (samples, sampleRate, fundamentalHz, start, count);

    if (fundamental < 1e-12)
        return 0.0f;

    auto harmonics = 0.0;

    for (int harmonic = 2; harmonic <= 5; ++harmonic)
        harmonics += goertzelPower (samples, sampleRate, fundamentalHz * static_cast<double> (harmonic), start, count);

    return static_cast<float> (std::sqrt (harmonics / fundamental));
}

} // namespace sendbloom::test
