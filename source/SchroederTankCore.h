#pragma once

#include "DampedComb.h"
#include "SchroederAllpass.h"
#include "SchroederTank32DelayTable.h"
#include <array>
#include <cmath>
#include <juce_dsp/juce_dsp.h>

namespace sendbloom
{

class SchroederTankCore
{
public:
    void prepare (double processingRate, int /*maxBlockSize*/) noexcept
    {
        processingRate_ = processingRate;

        const auto maxDelay = static_cast<int> (std::ceil (processingRate * 1.2));

        predelayLine.setMaximumDelayInSamples (maxDelay);
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = processingRate;
        spec.maximumBlockSize = 512;
        spec.numChannels = 1;
        predelayLine.prepare (spec);
        predelayLine.reset();
        predelayLine.setDelay (static_cast<float> (SchroederTank32DelayTable::kDarkPredelaySeconds
                                                    * processingRate_));

        for (size_t i = 0; i < seriesApfs.size(); ++i)
        {
            seriesApfs[i].prepare (processingRate, maxDelay);
            seriesApfs[i].setFeedback (SchroederTank32DelayTable::kApfFeedback);
        }

        for (size_t i = 0; i < parallelCombs.size(); ++i)
            parallelCombs[i].prepare (processingRate, maxDelay);

        tankAp.prepare (processingRate, maxDelay);
        tankAp.setFeedback (SchroederTank32DelayTable::kTankApFeedback);

        resetDelayLengths();
        syncCombProcessingRate();
        lfoPhase = 0.0f;
    }

    void setParameters (float rt60Seconds, float darkMix) noexcept
    {
        updateCoeffs (rt60Seconds, darkMix);
    }

    float processSample (float input) noexcept
    {
        return processTank (input);
    }

    void reset() noexcept
    {
        predelayLine.reset();

        for (auto& apf : seriesApfs)
            apf.reset();

        for (auto& comb : parallelCombs)
            comb.reset();

        tankAp.reset();
        lfoPhase = 0.0f;
    }

private:
    float scaleDelay (int delayAt32k) const noexcept
    {
        return static_cast<float> (delayAt32k)
             * static_cast<float> (processingRate_ / SchroederTank32DelayTable::kInternalRate);
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
            comb.setProcessingSampleRate (processingRate_);
    }

    void updateCoeffs (float rt60Seconds, float darkMix) noexcept
    {
        darkMix_ = juce::jlimit (0.0f, 1.0f, darkMix);

        const auto dampingHz = juce::jmap (darkMix_,
                                           SchroederTank32DelayTable::kBrightDampingHz,
                                           SchroederTank32DelayTable::kDarkDampingHz);
        const auto rt60 = juce::jmax (rt60Seconds, 0.05f);

        for (size_t i = 0; i < parallelCombs.size(); ++i)
        {
            const auto combDelay = scaleDelay (SchroederTank32DelayTable::kParallelCombDelays[i]);
            parallelCombs[i].setDampingCutoff (dampingHz);
            parallelCombs[i].setFeedbackForRT60 (rt60, combDelay);
        }
    }

    float processTank (float input) noexcept
    {
        predelayLine.pushSample (0, input);
        const auto delayed = predelayLine.popSample (0);
        float x = input + darkMix_ * (delayed - input);

        for (auto& apf : seriesApfs)
            x = apf.processSample (x);

        float combSum = 0.0f;

        for (auto& comb : parallelCombs)
            combSum += comb.processSample (x);

        combSum *= 0.25f;

        lfoPhase += static_cast<float> (2.0 * juce::MathConstants<double>::pi
                                        * SchroederTank32DelayTable::kTankLfoHz / processingRate_);

        if (lfoPhase > juce::MathConstants<float>::twoPi)
            lfoPhase -= juce::MathConstants<float>::twoPi;

        const auto mod = std::sin (lfoPhase)
                       * SchroederTank32DelayTable::tankLfoDepthSamplesForRate (processingRate_);
        tankAp.setDelay (scaleDelay (SchroederTank32DelayTable::kTankApDelay) + mod);

        return tankAp.processSample (combSum) * 0.85f;
    }

    std::array<SchroederAllpass, 4> seriesApfs;
    std::array<DampedComb, 4> parallelCombs;
    SchroederAllpass tankAp;

    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None> predelayLine;
    double processingRate_ { SchroederTank32DelayTable::kInternalRate };
    float darkMix_ { 0.0f };
    float lfoPhase { 0.0f };
};

} // namespace sendbloom
