#pragma once

#include "ParameterCurves.h"
#include "ParameterIDs.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace sendbloom
{

struct ParameterSnapshot
{
    float inputGainNorm {};
    float inputGainDb {};
    float inputThresholdNorm {};
    float inputThresholdDb {};
    float inputThresholdLinear {};
    float sizeNorm {};
    float levelNorm {};
    float wetGain {};
    float distnNorm {};
    float distnBlend {};
    float outputGainDb {};
    float outputGainLinear {};
    float sendAmountNorm {};
    bool darkMode {};
    bool gatePre {};
    bool sendConnected {};
    bool sendFirmFeel {};
    bool extendedStereo {};
    bool bypassed {};

    static ParameterSnapshot capture (const juce::AudioProcessorValueTreeState& apvts) noexcept
    {
        ParameterSnapshot s;

        s.inputGainNorm = apvts.getRawParameterValue (ParameterIDs::inputGain)->load();
        s.inputGainDb = ParameterCurves::inputGainDb (s.inputGainNorm);

        s.inputThresholdNorm = apvts.getRawParameterValue (ParameterIDs::inputThreshold)->load();
        s.inputThresholdDb = ParameterCurves::inputThresholdDb (s.inputThresholdNorm);
        s.inputThresholdLinear = juce::Decibels::decibelsToGain (s.inputThresholdDb);

        s.sizeNorm = apvts.getRawParameterValue (ParameterIDs::size)->load();

        s.levelNorm = apvts.getRawParameterValue (ParameterIDs::level)->load();
        s.wetGain = ParameterCurves::levelWetGain (s.levelNorm);

        s.distnNorm = apvts.getRawParameterValue (ParameterIDs::distn)->load();
        s.distnBlend = ParameterCurves::distnBlend (s.distnNorm);

        s.outputGainDb = apvts.getRawParameterValue (ParameterIDs::outputGain)->load();
        s.outputGainLinear = ParameterCurves::outputGainLinear (s.outputGainDb);

        s.darkMode = apvts.getRawParameterValue (ParameterIDs::darkMode)->load() > 0.5f;
        s.gatePre = static_cast<int> (apvts.getRawParameterValue (ParameterIDs::gatePrePost)->load()) == 0;
        s.sendConnected = apvts.getRawParameterValue (ParameterIDs::sendConnected)->load() > 0.5f;

        s.sendAmountNorm = apvts.getRawParameterValue (ParameterIDs::sendAmount)->load();
        s.sendFirmFeel = static_cast<int> (apvts.getRawParameterValue (ParameterIDs::sendFeel)->load()) == 0;

        s.extendedStereo = apvts.getRawParameterValue (ParameterIDs::extendedStereo)->load() > 0.5f;
        s.bypassed = apvts.getRawParameterValue (ParameterIDs::bypass)->load() > 0.5f;

        return s;
    }
};

} // namespace sendbloom
