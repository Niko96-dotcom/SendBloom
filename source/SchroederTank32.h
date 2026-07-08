#pragma once

#include "Authentic32Mode.h"
#include "DampedComb.h"
#include "EngineCrossfade.h"
#include "FixedRateAdapter.h"
#include "HostRateReverbEngine.h"
#include "IReverbEngine.h"
#include "SchroederAllpass.h"
#include "SchroederTank32DelayTable.h"
#include <array>
#include <cmath>
#include <optional>

namespace sendbloom
{

class SchroederTank32 : public IReverbEngine
{
public:
    void prepare (double sampleRate, int maxBlockSize) noexcept override
    {
        hostRate = sampleRate;
        maxBlockSize_ = maxBlockSize;
        useAuthenticPath = false;

        hostEngine.prepare (sampleRate, maxBlockSize);
        fixedRate_.prepare (sampleRate, maxBlockSize);
        engineCrossfade_.prepare (sampleRate);
        hostCrossfadeScratch_.assign (static_cast<size_t> (maxBlockSize), 0.0f);
        fixedCrossfadeScratch_.assign (static_cast<size_t> (maxBlockSize), 0.0f);

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

        resetDelayLengths();
        syncCombProcessingRate();
        lfoPhase = 0.0f;
    }

    float processSample (float input,
                         float rt60Seconds,
                         float darkMix,
                         bool authenticColor) noexcept override
    {
        if (engineCrossfade_.isCrossfading())
        {
            float out = 0.0f;
            processBlock (&input, &out, 1, rt60Seconds, darkMix, authenticColor);
            return out;
        }

        if (! authenticColor)
            return hostEngine.processSample (input, rt60Seconds, darkMix, false);

        const auto mode = diagnosticsMode_.value_or (Authentic32Mode::ProperSRC);
        float out = 0.0f;
        fixedRate_.processBlock (&input, &out, 1, rt60Seconds, darkMix, mode);
        return out;
    }

    void processBlock (const float* input,
                       float* output,
                       int numSamples,
                       float rt60Seconds,
                       float darkMix,
                       bool authenticColor) noexcept override
    {
        if (numSamples > maxBlockSize_)
            return;

        if (engineCrossfade_.isCrossfading())
        {
            const auto mode = diagnosticsMode_.value_or (Authentic32Mode::ProperSRC);
            hostEngine.processBlock (input,
                                     hostCrossfadeScratch_.data(),
                                     numSamples,
                                     rt60Seconds,
                                     darkMix,
                                     false);
            fixedRate_.processBlock (input,
                                     fixedCrossfadeScratch_.data(),
                                     numSamples,
                                     rt60Seconds,
                                     darkMix,
                                     mode);
            engineCrossfade_.mixWetBlock (hostCrossfadeScratch_.data(),
                                          fixedCrossfadeScratch_.data(),
                                          output,
                                          numSamples);

            if (! engineCrossfade_.isCrossfading())
            {
                if (engineCrossfade_.targetIsFixedEngine())
                    hostEngine.reset();
                else
                    fixedRate_.reset();
            }

            return;
        }

        if (! authenticColor)
        {
            IReverbEngine::processBlock (input, output, numSamples, rt60Seconds, darkMix, authenticColor);
            return;
        }

        const auto mode = diagnosticsMode_.value_or (Authentic32Mode::ProperSRC);
        fixedRate_.processBlock (input, output, numSamples, rt60Seconds, darkMix, mode);
    }

    bool isCrossfading() const noexcept override
    {
        return engineCrossfade_.isCrossfading();
    }

    void requestEngineCrossfade (bool targetAuthentic) noexcept override
    {
        if (engineCrossfade_.isCrossfading()
            && engineCrossfade_.targetIsFixedEngine() == targetAuthentic)
            return;

        if (targetAuthentic)
            engineCrossfade_.beginCrossfadeTowardFixed();
        else
            engineCrossfade_.beginCrossfadeTowardHost();
    }

    void setAuthentic32ModeForDiagnostics (Authentic32Mode mode) noexcept
    {
        diagnosticsMode_ = mode;
    }

    void clearAuthentic32ModeForDiagnostics() noexcept
    {
        diagnosticsMode_ = std::nullopt;
    }

    int getSrcRoundTripLatencySamples() const noexcept
    {
        return fixedRate_.getRoundTripLatencySamples();
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

    HostRateReverbEngine hostEngine;
    FixedRateAdapter fixedRate_;
    EngineCrossfade engineCrossfade_;
    std::vector<float> hostCrossfadeScratch_;
    std::vector<float> fixedCrossfadeScratch_;
    std::optional<Authentic32Mode> diagnosticsMode_;

    std::array<SchroederAllpass, 4> seriesApfs;
    std::array<DampedComb, 4> parallelCombs;
    SchroederAllpass tankAp;

    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None> predelayLine;
    double hostRate { 48000.0 };
    int maxBlockSize_ { 512 };
    bool useAuthenticPath { false };
    float predelaySamples { 0.0f };
    float lfoPhase { 0.0f };
};

} // namespace sendbloom
