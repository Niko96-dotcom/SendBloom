#pragma once

#include "ParameterSnapshot.h"
#include <juce_audio_basics/juce_audio_basics.h>

namespace sendbloom
{

class SmoothedParameterBank
{
public:
    void prepare (double sampleRate) noexcept
    {
        inputGainLinear.reset (sampleRate, 0.020);
        outputGainLinear.reset (sampleRate, 0.020);
        sizeNorm.reset (sampleRate, 0.050);
        inputThresholdLinear.reset (sampleRate, 0.050);
        levelWetGain.reset (sampleRate, 0.020);
        distnBlend.reset (sampleRate, 0.020);
        // sendGain smoothing removed — PressureController owns attack/release (ADR-V1-04)
        darkModeTarget.reset (sampleRate, 0.015);
        bypassWetMix.reset (sampleRate, 0.005);
    }

    void setTargets (const ParameterSnapshot& snap) noexcept
    {
        inputGainLinear.setTargetValue (juce::Decibels::decibelsToGain (snap.inputGainDb));
        outputGainLinear.setTargetValue (snap.outputGainLinear);
        sizeNorm.setTargetValue (snap.sizeNorm);
        inputThresholdLinear.setTargetValue (snap.inputThresholdLinear);
        levelWetGain.setTargetValue (snap.wetGain);
        distnBlend.setTargetValue (snap.distnBlend);
        darkModeTarget.setTargetValue (snap.darkMode ? 1.0f : 0.0f);
        bypassWetMix.setTargetValue (snap.bypassed ? 0.0f : 1.0f);
    }

    void snapToTargets() noexcept
    {
        inputGainLinear.setCurrentAndTargetValue (inputGainLinear.getTargetValue());
        outputGainLinear.setCurrentAndTargetValue (outputGainLinear.getTargetValue());
        sizeNorm.setCurrentAndTargetValue (sizeNorm.getTargetValue());
        inputThresholdLinear.setCurrentAndTargetValue (inputThresholdLinear.getTargetValue());
        levelWetGain.setCurrentAndTargetValue (levelWetGain.getTargetValue());
        distnBlend.setCurrentAndTargetValue (distnBlend.getTargetValue());
        darkModeTarget.setCurrentAndTargetValue (darkModeTarget.getTargetValue());
        bypassWetMix.setCurrentAndTargetValue (bypassWetMix.getTargetValue());
    }

    float getNextInputGainLinear() noexcept { return inputGainLinear.getNextValue(); }
    float getNextOutputGainLinear() noexcept { return outputGainLinear.getNextValue(); }
    float getNextSizeNorm() noexcept { return sizeNorm.getNextValue(); }
    float getNextInputThresholdLinear() noexcept { return inputThresholdLinear.getNextValue(); }
    float getNextLevelWetGain() noexcept { return levelWetGain.getNextValue(); }
    float getNextDistnBlend() noexcept { return distnBlend.getNextValue(); }
    float getNextDarkModeTarget() noexcept { return darkModeTarget.getNextValue(); }
    float getNextBypassWetMix() noexcept { return bypassWetMix.getNextValue(); }

    juce::SmoothedValue<float>& getBypassWetMixSmoother() noexcept { return bypassWetMix; }

private:
    juce::SmoothedValue<float> inputGainLinear;
    juce::SmoothedValue<float> outputGainLinear;
    juce::SmoothedValue<float> sizeNorm;
    juce::SmoothedValue<float> inputThresholdLinear;
    juce::SmoothedValue<float> levelWetGain;
    juce::SmoothedValue<float> distnBlend;
    juce::SmoothedValue<float> darkModeTarget;
    juce::SmoothedValue<float> bypassWetMix;
};

} // namespace sendbloom
