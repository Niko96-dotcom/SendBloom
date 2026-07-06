#pragma once

#include "DampedComb.h"
#include "IReverbEngine.h"
#include "SchroederAllpass.h"
#include <array>
#include <cmath>

namespace sendbloom
{

class Fdn8Reverb : public IReverbEngine
{
public:
    void prepare (double sampleRate, int /*maxBlockSize*/) noexcept override
    {
        hostRate = sampleRate;
        const auto maxDelay = static_cast<int> (std::ceil (sampleRate * 0.12));

        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = 512;
        spec.numChannels = 1;

        predelayLine.setMaximumDelayInSamples (maxDelay);
        predelayLine.prepare (spec);
        predelayLine.reset();

        for (size_t i = 0; i < diffusionApfs.size(); ++i)
        {
            diffusionApfs[i].prepare (sampleRate, maxDelay);
            diffusionApfs[i].setFeedback (0.7f);
            const auto delayMs = 5.0f + static_cast<float> (i) * 1.5f;
            diffusionApfs[i].setDelay (delayMs * 0.001f * static_cast<float> (sampleRate));
        }

        for (size_t i = 0; i < lines.size(); ++i)
        {
            lines[i].prepare (sampleRate, maxDelay);
            const auto delaySamples = kLineDelaysMs[i] * 0.001f * static_cast<float> (sampleRate);
            lines[i].setDelay (delaySamples);
        }
    }

    float processSample (float input,
                         float rt60Seconds,
                         float darkMix,
                         bool /*authenticColor*/) noexcept override
    {
        const auto mix = juce::jlimit (0.0f, 1.0f, darkMix);
        const auto predelaySec = mix * kDarkPredelaySeconds;
        predelayLine.setDelay (predelaySec * static_cast<float> (hostRate));

        auto x = input;

        if (predelaySec > 0.0005f)
        {
            predelayLine.pushSample (0, x);
            x = predelayLine.popSample (0);
        }

        for (auto& apf : diffusionApfs)
            x = apf.processSample (x);

        const auto dampingHz = juce::jmap (mix, kBrightDampingHz, kDarkDampingHz);
        const auto rt60 = juce::jmax (rt60Seconds, 0.05f);

        float sumDelay = 0.0f;

        for (const auto ms : kLineDelaysMs)
            sumDelay += ms * 0.001f * static_cast<float> (hostRate);

        const auto referenceDelay = sumDelay / static_cast<float> (lines.size());

        for (auto& line : lines)
        {
            line.setDampingCutoff (dampingHz);
            line.setFeedbackForRT60 (rt60, referenceDelay);
        }

        float sum = 0.0f;
        const auto feed = x / static_cast<float> (lines.size());

        for (auto& line : lines)
            sum += line.processSample (feed);

        return juce::jlimit (-4.0f, 4.0f, sum * 0.85f);
    }

private:
    static constexpr std::array<float, 8> kLineDelaysMs { 37.0f, 41.0f, 43.0f, 47.0f,
                                                          53.0f, 59.0f, 61.0f, 67.0f };
    static constexpr float kDarkPredelaySeconds = 0.055f;
    static constexpr float kBrightDampingHz = 8000.0f;
    static constexpr float kDarkDampingHz = 3200.0f;

    std::array<DampedComb, 8> lines;
    std::array<SchroederAllpass, 2> diffusionApfs;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None> predelayLine;
    double hostRate { 48000.0 };
};

} // namespace sendbloom
