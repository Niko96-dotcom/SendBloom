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

inline void fillConstantSine (std::vector<float>& buffer, float amplitude, float phaseInc)
{
    buffer.resize (buffer.capacity() > 0 ? buffer.size() : buffer.capacity());

    for (size_t i = 0; i < buffer.size(); ++i)
        buffer[i] = amplitude * std::sin (phaseInc * static_cast<float> (i));
}

inline std::vector<float> renderParallelMix (const std::vector<float>& dryTap,
                                             const std::vector<float>& wet,
                                             const std::vector<float>& wetGain)
{
    const auto n = dryTap.size();
    std::vector<float> out (n);

    for (size_t i = 0; i < n; ++i)
    {
        const auto g = i < wetGain.size() ? wetGain[i] : wetGain.back();
        const auto w = i < wet.size() ? wet[i] : wet.back();
        const auto d = i < dryTap.size() ? dryTap[i] : dryTap.back();
        out[i] = d + w * g;
    }

    return out;
}

} // namespace sendbloom::test
