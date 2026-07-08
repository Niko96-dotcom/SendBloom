#pragma once

#include "DampedComb.h"
#include "SchroederAllpass.h"
#include "SchroederTank32DelayTable.h"
#include <array>
#include <cmath>
#include <juce_dsp/juce_dsp.h>

namespace sendbloom
{

class LegacyAccumulatorPath
{
public:
    void prepare (double hostRate, int maxBlockSize) noexcept
    {
        juce::ignoreUnused (maxBlockSize);
        hostRate_ = hostRate;

        const auto maxDelay = static_cast<int> (std::ceil (hostRate * 1.2));

        predelayLine.setMaximumDelayInSamples (maxDelay);
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = hostRate;
        spec.maximumBlockSize = 512;
        spec.numChannels = 1;
        predelayLine.prepare (spec);
        predelayLine.reset();

        for (size_t i = 0; i < seriesApfs.size(); ++i)
        {
            seriesApfs[i].prepare (hostRate, maxDelay);
            seriesApfs[i].setFeedback (SchroederTank32DelayTable::kApfFeedback);
        }

        for (size_t i = 0; i < parallelCombs.size(); ++i)
            parallelCombs[i].prepare (hostRate, maxDelay);

        tankAp.prepare (hostRate, maxDelay);
        tankAp.setFeedback (SchroederTank32DelayTable::kTankApFeedback);

        resetDelayLengths();
        syncCombProcessingRate();

        juce::dsp::ProcessSpec antiImageSpec;
        antiImageSpec.sampleRate = hostRate;
        antiImageSpec.maximumBlockSize = 512;
        antiImageSpec.numChannels = 1;
        antiImageFilter.prepare (antiImageSpec);
        antiImageFilter.reset();
        antiImageFilter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
        antiImageFilter.setCutoffFrequency (SchroederTank32DelayTable::kAuthenticAntiImageLpHz);

        reset();
    }

    void processBlock (const float* in,
                       float* out,
                       int n,
                       float rt60,
                       float darkMix) noexcept
    {
        for (int i = 0; i < n; ++i)
        {
            updateCoeffs (rt60, darkMix);
            out[i] = processAuthentic (in[i]);
        }
    }

    void reset() noexcept
    {
        predelayLine.reset();

        for (auto& apf : seriesApfs)
            apf.reset();

        for (auto& comb : parallelCombs)
            comb.reset();

        tankAp.reset();
        antiImageFilter.reset();

        lfoPhase = 0.0f;
        inputAccumulator = 0.0;
        outputHold = 0.0f;
        lastInternalOut = 0.0f;
        predelaySamples = 0.0f;
    }

private:
    static float quantize9bit (float value) noexcept
    {
        constexpr auto steps = 511.0f;
        return std::round (value * steps) / steps;
    }

    float scaleDelay (int delayAt32k) const noexcept
    {
        return static_cast<float> (delayAt32k);
    }

    void resetDelayLengths() noexcept
    {
        for (size_t i = 0; i < seriesApfs.size(); ++i)
            seriesApfs[i].setDelay (scaleDelay (SchroederTank32DelayTable::kSeriesApfDelays[i]));

        for (size_t i = 0; i < parallelCombs.size(); ++i)
            parallelCombs[i].setDelay (scaleDelay (SchroederTank32DelayTable::kParallelCombDelays[i]));

        tankAp.setDelay (scaleDelay (SchroederTank32DelayTable::kTankApDelay));
    }

    void syncCombProcessingRate() noexcept
    {
        for (auto& comb : parallelCombs)
            comb.setProcessingSampleRate (SchroederTank32DelayTable::kInternalRate);
    }

    void updateCoeffs (float rt60Seconds, float darkMix) noexcept
    {
        const auto mix = juce::jlimit (0.0f, 1.0f, darkMix);
        const auto predelaySec = mix * SchroederTank32DelayTable::kDarkPredelaySeconds;
        predelaySamples = predelaySec * static_cast<float> (SchroederTank32DelayTable::kInternalRate);
        predelayLine.setDelay (predelaySamples);

        auto dampingHz = juce::jmap (mix,
                                     SchroederTank32DelayTable::kAuthenticBrightDampingHz,
                                     SchroederTank32DelayTable::kDarkDampingHz);

        auto rt60 = juce::jmax (rt60Seconds, 0.05f);

        const auto brightRef = SchroederTank32DelayTable::kAuthenticBrightDampingHz;
        dampingHz = quantize9bit (dampingHz / brightRef) * brightRef;
        const auto rt60Norm = quantize9bit (rt60 / 6.25f) * 6.25f;
        rt60 = juce::jmax (rt60Norm, 0.05f);

        for (size_t i = 0; i < parallelCombs.size(); ++i)
        {
            const auto combDelay = scaleDelay (SchroederTank32DelayTable::kParallelCombDelays[i]);
            parallelCombs[i].setDampingCutoff (dampingHz);
            parallelCombs[i].setFeedbackForRT60 (rt60, combDelay);
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

        lfoPhase += static_cast<float> (2.0 * juce::MathConstants<double>::pi
                                        * SchroederTank32DelayTable::kTankLfoHz
                                        / SchroederTank32DelayTable::kInternalRate);

        if (lfoPhase > juce::MathConstants<float>::twoPi)
            lfoPhase -= juce::MathConstants<float>::twoPi;

        const auto mod = std::sin (lfoPhase) * SchroederTank32DelayTable::kTankLfoDepthSamples;
        tankAp.setDelay (scaleDelay (SchroederTank32DelayTable::kTankApDelay) + mod);

        return tankAp.processSample (combSum) * 0.85f;
    }

    float processAuthentic (float input) noexcept
    {
        const auto ratio = SchroederTank32DelayTable::kInternalRate / hostRate_;
        inputAccumulator += ratio;

        while (inputAccumulator >= 1.0)
        {
            inputAccumulator -= 1.0;
            lastInternalOut = processTank (input);
        }

        const auto frac = static_cast<float> (inputAccumulator);
        const auto held = lastInternalOut * (1.0f - frac) + outputHold * frac;
        outputHold = lastInternalOut;
        const auto filtered = antiImageFilter.processSample (0, held);
        return juce::jlimit (-4.0f, 4.0f, filtered);
    }

    std::array<SchroederAllpass, 4> seriesApfs;
    std::array<DampedComb, 4> parallelCombs;
    SchroederAllpass tankAp;

    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None> predelayLine;
    juce::dsp::StateVariableTPTFilter<float> antiImageFilter;

    double hostRate_ { 48000.0 };
    float predelaySamples { 0.0f };
    float lfoPhase { 0.0f };
    double inputAccumulator { 0.0 };
    float outputHold { 0.0f };
    float lastInternalOut { 0.0f };
};

} // namespace sendbloom
