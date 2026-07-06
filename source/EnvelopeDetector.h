#pragma once

#include <cmath>

namespace sendbloom
{

class EnvelopeDetector
{
public:
    void prepare (double sampleRate, float attackMs, float releaseMs) noexcept
    {
        attackCoeff = coeffForMs (attackMs, sampleRate);
        releaseCoeff = coeffForMs (releaseMs, sampleRate);
        envelope = 0.0f;
    }

    float process (float input) noexcept
    {
        const auto x = std::abs (input);
        const auto coeff = x > envelope ? attackCoeff : releaseCoeff;
        envelope = coeff * envelope + (1.0f - coeff) * x;
        return envelope;
    }

    float getEnvelope() const noexcept { return envelope; }

    void reset() noexcept { envelope = 0.0f; }

private:
    static float coeffForMs (float ms, double sampleRate) noexcept
    {
        return std::exp (-1.0f / (ms * 0.001f * static_cast<float> (sampleRate)));
    }

    float envelope { 0.0f };
    float attackCoeff { 0.0f };
    float releaseCoeff { 0.0f };
};

} // namespace sendbloom
