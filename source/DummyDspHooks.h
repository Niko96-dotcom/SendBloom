#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>

namespace sendbloom
{

struct DummyDspState
{
    float lowpassState[2] { 0.0f, 0.0f };
    float darkCoeff { 0.0f };
    double sampleRate { 48000.0 };

    void prepare (double rate) noexcept
    {
        sampleRate = rate;
        lowpassState[0] = 0.0f;
        lowpassState[1] = 0.0f;
        darkCoeff = 0.0f;
    }
};

struct DummyDspHooks
{
    static float processSample (float input,
                                float sizeNorm,
                                float distnBlend,
                                float wetLevel,
                                float sendGain,
                                float darkTarget,
                                DummyDspState& state,
                                int channel) noexcept
    {
        state.darkCoeff += 0.001f * (darkTarget - state.darkCoeff);

        const auto cutoff = 200.0f + sizeNorm * 7800.0f;
        const auto coeff = std::exp (-2.0f * juce::MathConstants<float>::pi * cutoff
                                     / static_cast<float> (state.sampleRate));

        auto& lp = state.lowpassState[channel];
        lp = coeff * lp + (1.0f - coeff) * input;
        const auto toned = lp;

        const auto driven = std::tanh (toned * 3.0f);
        const auto distorted = toned + distnBlend * (driven - toned);

        return distorted * wetLevel * sendGain * (1.0f - state.darkCoeff * 0.35f);
    }
};

} // namespace sendbloom
