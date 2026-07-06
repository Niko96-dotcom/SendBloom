#pragma once

#include "DampedComb.h"
#include "IReverbEngine.h"
#include "SchroederAllpass.h"
#include "SchroederTank32DelayTable.h"
#include <array>
#include <cmath>

namespace sendbloom
{

class SchroederTank32 : public IReverbEngine
{
public:
    void prepare (double sampleRate, int /*maxBlockSize*/) noexcept override
    {
        hostRate = sampleRate;
        useAuthenticPath = false;

        const auto maxDelay = static_cast<int> (std::ceil (sampleRate * 1.2));

        predelayLine.setMaximumDelayInSamples (maxDelay);
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = 512;
        spec.numChannels = 1;
        predelayLine.prepare (spec);
        predelayLine.reset();

        for (size_t i = 0; i < seriesApfs.size(); ++i)
        {
            seriesApfs[i].prepare (sampleRate, maxDelay);
            seriesApfs[i].setFeedback (SchroederTank32DelayTable::kApfFeedback);
        }

        for (size_t i = 0; i < parallelCombs.size(); ++i)
        {
            parallelCombs[i].prepare (sampleRate, maxDelay);
        }

        tankAp.prepare (sampleRate, maxDelay);
        tankAp.setFeedback (SchroederTank32DelayTable::kTankApFeedback);

        resetDelayLengths (false);
        lfoPhase = 0.0f;
        inputAccumulator = 0.0;
        outputHold = 0.0f;
        lastInternalOut = 0.0f;
    }

    float processSample (float input,
                         float rt60Seconds,
                         float darkMix,
                         bool authenticColor) noexcept override
    {
        if (authenticColor != useAuthenticPath)
        {
            useAuthenticPath = authenticColor;
            resetDelayLengths (useAuthenticPath);
        }

        updateCoeffs (rt60Seconds, darkMix);

        if (useAuthenticPath)
            return processAuthentic (input);

        return processHostRate (input);
    }

private:
    static float quantize9bit (float value) noexcept
    {
        constexpr auto steps = 511.0f;
        return std::round (value * steps) / steps;
    }

    float scaleDelay (int delayAt32k, bool authentic) const noexcept
    {
        if (authentic)
            return static_cast<float> (delayAt32k);

        return static_cast<float> (delayAt32k) * static_cast<float> (hostRate / SchroederTank32DelayTable::kInternalRate);
    }

    void resetDelayLengths (bool authentic) noexcept
    {
        for (size_t i = 0; i < seriesApfs.size(); ++i)
            seriesApfs[i].setDelay (scaleDelay (SchroederTank32DelayTable::kSeriesApfDelays[i], authentic));

        for (size_t i = 0; i < parallelCombs.size(); ++i)
            parallelCombs[i].setDelay (scaleDelay (SchroederTank32DelayTable::kParallelCombDelays[i], authentic));

        tankAp.setDelay (scaleDelay (SchroederTank32DelayTable::kTankApDelay, authentic));
    }

    void updateCoeffs (float rt60Seconds, float darkMix) noexcept
    {
        const auto mix = juce::jlimit (0.0f, 1.0f, darkMix);
        const auto predelaySec = mix * SchroederTank32DelayTable::kDarkPredelaySeconds;
        predelaySamples = predelaySec * static_cast<float> (useAuthenticPath
                                                               ? SchroederTank32DelayTable::kInternalRate
                                                               : hostRate);
        predelayLine.setDelay (predelaySamples);

        auto dampingHz = juce::jmap (mix,
                                     SchroederTank32DelayTable::kBrightDampingHz,
                                     SchroederTank32DelayTable::kDarkDampingHz);

        auto rt60 = juce::jmax (rt60Seconds, 0.05f);

        float maxCombDelay = 0.0f;
        float sumCombDelay = 0.0f;

        for (const auto d : SchroederTank32DelayTable::kParallelCombDelays)
        {
            const auto scaled = scaleDelay (d, useAuthenticPath);
            maxCombDelay = std::max (maxCombDelay, scaled);
            sumCombDelay += scaled;
        }

        const auto referenceCombDelay = sumCombDelay / static_cast<float> (SchroederTank32DelayTable::kParallelCombDelays.size());
        juce::ignoreUnused (maxCombDelay);

        if (useAuthenticPath)
        {
            dampingHz = quantize9bit (dampingHz / SchroederTank32DelayTable::kBrightDampingHz)
                        * SchroederTank32DelayTable::kBrightDampingHz;
            const auto rt60Norm = quantize9bit (rt60 / 6.25f) * 6.25f;
            rt60 = juce::jmax (rt60Norm, 0.05f);
        }

        for (auto& comb : parallelCombs)
        {
            comb.setDampingCutoff (dampingHz);
            comb.setFeedbackForRT60 (rt60, referenceCombDelay);
        }
    }

    float processTank (float input) noexcept
    {
        float x = input;

        if (predelaySamples > 0.5f)
        {
            predelayLine.pushSample (0, input);
            x = predelayLine.popSample (0);
        }

        for (auto& apf : seriesApfs)
            x = apf.processSample (x);

        float combSum = 0.0f;

        for (auto& comb : parallelCombs)
            combSum += comb.processSample (x);

        combSum *= 0.25f;

        const auto effectiveRate = useAuthenticPath ? SchroederTank32DelayTable::kInternalRate : hostRate;
        lfoPhase += static_cast<float> (2.0 * juce::MathConstants<double>::pi
                                        * SchroederTank32DelayTable::kTankLfoHz / effectiveRate);

        if (lfoPhase > juce::MathConstants<float>::twoPi)
            lfoPhase -= juce::MathConstants<float>::twoPi;

        const auto mod = std::sin (lfoPhase) * SchroederTank32DelayTable::kTankLfoDepthSamples;
        tankAp.setDelay (scaleDelay (SchroederTank32DelayTable::kTankApDelay, useAuthenticPath) + mod);

        return tankAp.processSample (combSum) * 0.85f;
    }

    float processHostRate (float input) noexcept
    {
        return juce::jlimit (-4.0f, 4.0f, processTank (input));
    }

    float processAuthentic (float input) noexcept
    {
        const auto ratio = SchroederTank32DelayTable::kInternalRate / hostRate;
        inputAccumulator += ratio;

        while (inputAccumulator >= 1.0)
        {
            inputAccumulator -= 1.0;
            lastInternalOut = processTank (input);
        }

        const auto frac = static_cast<float> (inputAccumulator);
        const auto out = lastInternalOut * (1.0f - frac) + outputHold * frac;
        outputHold = lastInternalOut;
        return juce::jlimit (-4.0f, 4.0f, out);
    }

    std::array<SchroederAllpass, 4> seriesApfs;
    std::array<DampedComb, 4> parallelCombs;
    SchroederAllpass tankAp;

    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None> predelayLine;
    double hostRate { 48000.0 };
    bool useAuthenticPath { false };
    float predelaySamples { 0.0f };
    float lfoPhase { 0.0f };
    double inputAccumulator { 0.0 };
    float outputHold { 0.0f };
    float lastInternalOut { 0.0f };
};

} // namespace sendbloom
