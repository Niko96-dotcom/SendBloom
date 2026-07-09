#include "ParameterLayout.h"
#include "ParameterIDs.h"

namespace sendbloom
{

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using namespace ParameterIDs;

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { inputGain, 1 }, "Input Gain",
        juce::NormalisableRange<float> { 0.0f, 1.0f }, 0.5f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { inputThreshold, 1 }, "Input Threshold",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.0f, 0.5f }, 0.35f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { size, 1 }, "Size",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.0f, 0.5f }, 0.5f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { level, 1 }, "Level",
        juce::NormalisableRange<float> { 0.0f, 1.0f }, 0.5f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { distn, 1 }, "Distn",
        juce::NormalisableRange<float> { 0.0f, 1.0f }, 0.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { outputGain, 1 }, "Output Gain",
        juce::NormalisableRange<float> { -12.0f, 12.0f }, 0.0f));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { darkMode, 1 }, "Dark Mode", false));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { gatePrePost, 1 }, "Gate Pre/Post",
        juce::StringArray { "PreSoft", "PostHard" }, 1));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { sendConnected, 1 }, "Send Connected", false));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { sendAmount, 1 }, "Send Amount",
        juce::NormalisableRange<float> { 0.0f, 1.0f }, 1.0f));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { sendFeel, 1 }, "Send Feel",
        juce::StringArray { "Firm", "Soft" }, 0));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { authenticColor, 1 }, "Authentic Color", false));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { extendedStereo, 1 }, "Extended Stereo", false));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { dirtOs, 1 }, "Dirt OS", false));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { bypass, 1 }, "Bypass", false));

    return layout;
}

} // namespace sendbloom
