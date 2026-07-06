#pragma once

#include <ParameterCurves.h>
#include <ParameterIDs.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace sendbloom
{

struct ParameterSnapshot
{
    float inputGainNorm {};
    float inputGainDb {};
    float inputThresholdNorm {};
    float inputThresholdDb {};
    float sizeNorm {};
    float rt60Seconds {};
    float levelNorm {};
    float wetGain {};
    float dryGain {};
    float distnNorm {};
    float distnBlend {};
    float outputGainDb {};
    float outputGainLinear {};
    float sendAmountNorm {};
    float sendGain {};
    bool darkMode {};
    bool gatePreSoft {};
    bool sendConnected {};
    bool sendFirmFeel {};
    bool authenticColor {};
    bool extendedStereo {};
    bool dirtOs {};
    bool bypassed {};

    static ParameterSnapshot capture (const juce::AudioProcessorValueTreeState& apvts) noexcept
    {
        ParameterSnapshot s;

        s.inputGainNorm = apvts.getRawParameterValue (ParameterIDs::inputGain)->load();
        s.inputGainDb = ParameterCurves::inputGainDb (s.inputGainNorm);

        s.inputThresholdNorm = apvts.getRawParameterValue (ParameterIDs::inputThreshold)->load();
        s.inputThresholdDb = ParameterCurves::inputThresholdDb (s.inputThresholdNorm);

        s.sizeNorm = apvts.getRawParameterValue (ParameterIDs::size)->load();
        s.rt60Seconds = ParameterCurves::sizeToRT60 (s.sizeNorm);

        s.levelNorm = apvts.getRawParameterValue (ParameterIDs::level)->load();
        ParameterCurves::levelEqualPower (s.levelNorm, s.dryGain, s.wetGain);

        s.distnNorm = apvts.getRawParameterValue (ParameterIDs::distn)->load();
        s.distnBlend = ParameterCurves::distnBlend (s.distnNorm);

        s.outputGainDb = apvts.getRawParameterValue (ParameterIDs::outputGain)->load();
        s.outputGainLinear = ParameterCurves::outputGainLinear (s.outputGainDb);

        s.darkMode = apvts.getRawParameterValue (ParameterIDs::darkMode)->load() > 0.5f;
        s.gatePreSoft = static_cast<int> (apvts.getRawParameterValue (ParameterIDs::gatePrePost)->load()) == 0;
        s.sendConnected = apvts.getRawParameterValue (ParameterIDs::sendConnected)->load() > 0.5f;

        s.sendAmountNorm = apvts.getRawParameterValue (ParameterIDs::sendAmount)->load();
        s.sendFirmFeel = static_cast<int> (apvts.getRawParameterValue (ParameterIDs::sendFeel)->load()) == 0;
        s.sendGain = s.sendConnected
                       ? ParameterCurves::sendGain (s.sendAmountNorm, s.sendFirmFeel)
                       : 1.0f;

        s.authenticColor = apvts.getRawParameterValue (ParameterIDs::authenticColor)->load() > 0.5f;
        s.extendedStereo = apvts.getRawParameterValue (ParameterIDs::extendedStereo)->load() > 0.5f;
        s.dirtOs = apvts.getRawParameterValue (ParameterIDs::dirtOs)->load() > 0.5f;
        s.bypassed = apvts.getRawParameterValue (ParameterIDs::bypass)->load() > 0.5f;

        return s;
    }
};

} // namespace sendbloom
