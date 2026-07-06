#include <ParameterCurves.h>
#include <ParameterIDs.h>
#include <PluginProcessor.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

namespace
{

float bufferRms (const juce::AudioBuffer<float>& buffer, int channel, int start, int count)
{
    double sum = 0.0;

    for (int i = start; i < start + count; ++i)
    {
        const auto sample = buffer.getSample (channel, i);
        sum += static_cast<double> (sample) * static_cast<double> (sample);
    }

    return static_cast<float> (std::sqrt (sum / static_cast<double> (count)));
}

void fillStereoSine (juce::AudioBuffer<float>& buffer, float leftAmp, float rightAmp, float phaseInc)
{
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        const auto sample = std::sin (phaseInc * static_cast<float> (i));
        buffer.setSample (0, i, leftAmp * sample);
        buffer.setSample (1, i, rightAmp * sample);
    }
}

float renderStereoPluginRms (float leftAmp, float rightAmp)
{
    using namespace sendbloom::ParameterIDs;

    sendbloom::PluginProcessor plugin;
    auto& apvts = plugin.getAPVTS();
    *apvts.getRawParameterValue (inputGain) = 1.0f;
    *apvts.getRawParameterValue (outputGain) = 0.0f;
    *apvts.getRawParameterValue (bypass) = 0.0f;
    *apvts.getRawParameterValue (level) = 1.0f;
    *apvts.getRawParameterValue (distn) = 0.0f;
    *apvts.getRawParameterValue (size) = 0.75f;
    *apvts.getRawParameterValue (sendConnected) = 1.0f;
    *apvts.getRawParameterValue (sendAmount) = 1.0f;
    *apvts.getRawParameterValue (gatePrePost) = 0.0f;
    plugin.prepareToPlay (48000.0, 512);

    juce::AudioBuffer<float> buffer (2, 512);
    juce::MidiBuffer midi;

    for (int block = 0; block < 48; ++block)
    {
        fillStereoSine (buffer, leftAmp, rightAmp, 0.03f);
        plugin.processBlock (buffer, midi);
    }

    return bufferRms (buffer, 0, 256, 256);
}

} // namespace

TEST_CASE ("PluginProcessor dual-mono output matches for identical stereo input", "[io][mono][integration]")
{
    using namespace sendbloom::ParameterIDs;

    sendbloom::PluginProcessor plugin;
    auto& apvts = plugin.getAPVTS();
    *apvts.getRawParameterValue (inputGain) = 1.0f;
    *apvts.getRawParameterValue (outputGain) = 0.0f;
    *apvts.getRawParameterValue (bypass) = 0.0f;
    *apvts.getRawParameterValue (level) = 0.75f;
    *apvts.getRawParameterValue (size) = 0.5f;
    *apvts.getRawParameterValue (sendConnected) = 1.0f;
    *apvts.getRawParameterValue (sendAmount) = 1.0f;
    plugin.prepareToPlay (48000.0, 512);

    juce::AudioBuffer<float> buffer (2, 512);
    fillStereoSine (buffer, 0.5f, 0.5f, 0.03f);

    juce::MidiBuffer midi;

    for (int block = 0; block < 32; ++block)
    {
        fillStereoSine (buffer, 0.5f, 0.5f, 0.03f);
        plugin.processBlock (buffer, midi);
    }

    for (int i = 256; i < 512; ++i)
        REQUIRE (buffer.getSample (0, i) == Catch::Approx (buffer.getSample (1, i)).margin (1e-5f));
}

TEST_CASE ("Wet path uses summed mono bus from stereo input", "[io][mono][integration]")
{
    const auto inPhaseRms = renderStereoPluginRms (0.8f, 0.2f);
    const auto outOfPhaseRms = renderStereoPluginRms (0.5f, -0.5f);

    REQUIRE (inPhaseRms > outOfPhaseRms * 1.5f);
}
