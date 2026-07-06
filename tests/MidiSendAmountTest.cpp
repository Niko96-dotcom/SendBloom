#include "helpers/test_helpers.h"
#include <ParameterIDs.h>
#include <ParameterSnapshot.h>
#include <PluginProcessor.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

namespace
{

void configureSendPlugin (sendbloom::PluginProcessor& plugin, bool connected)
{
    using namespace sendbloom::ParameterIDs;

    auto& apvts = plugin.getAPVTS();
    *apvts.getRawParameterValue (inputGain) = 1.0f;
    *apvts.getRawParameterValue (outputGain) = 0.0f;
    *apvts.getRawParameterValue (bypass) = 0.0f;
    *apvts.getRawParameterValue (level) = 1.0f;
    *apvts.getRawParameterValue (distn) = 0.0f;
    *apvts.getRawParameterValue (size) = 0.5f;
    *apvts.getRawParameterValue (gatePrePost) = 0.0f;
    *apvts.getRawParameterValue (sendConnected) = connected ? 1.0f : 0.0f;
    *apvts.getRawParameterValue (sendAmount) = 0.0f;
    plugin.prepareToPlay (48000.0, 512);
}

} // namespace

TEST_CASE ("MIDI CC1 updates send_amount when connected", "[midi][send]")
{
    using namespace sendbloom::ParameterIDs;

    sendbloom::PluginProcessor plugin;
    configureSendPlugin (plugin, true);

    juce::AudioBuffer<float> buffer (2, 512);
    juce::MidiBuffer midi;
    buffer.clear();
    midi.addEvent (juce::MidiMessage::controllerEvent (1, 1, 100), 0);
    plugin.processBlock (buffer, midi);

    REQUIRE (*plugin.getAPVTS().getRawParameterValue (sendAmount)
             == Catch::Approx (100.0f / 127.0f).margin (0.02f));
}

TEST_CASE ("MIDI CC1 maps to sendGain when connected", "[midi][send]")
{
    sendbloom::PluginProcessor plugin;
    configureSendPlugin (plugin, true);

    juce::AudioBuffer<float> buffer (2, 64);
    juce::MidiBuffer midi;
    buffer.clear();

    midi.addEvent (juce::MidiMessage::controllerEvent (1, 1, 127), 0);

    for (int i = 0; i < 2000; ++i)
        plugin.processBlock (buffer, midi);

    const auto snapHigh = sendbloom::ParameterSnapshot::capture (plugin.getAPVTS());

    midi.addEvent (juce::MidiMessage::controllerEvent (1, 1, 0), 0);

    for (int i = 0; i < 2000; ++i)
        plugin.processBlock (buffer, midi);

    const auto snapLow = sendbloom::ParameterSnapshot::capture (plugin.getAPVTS());

    REQUIRE (snapHigh.sendGain > snapLow.sendGain * 2.0f);
    REQUIRE (snapLow.sendGain < 0.01f);
}

TEST_CASE ("MIDI CC1 ignored when send disconnected", "[midi][send]")
{
    using namespace sendbloom::ParameterIDs;

    sendbloom::PluginProcessor plugin;
    configureSendPlugin (plugin, false);

    juce::AudioBuffer<float> buffer (2, 512);
    juce::MidiBuffer midi;
    buffer.clear();
    midi.addEvent (juce::MidiMessage::controllerEvent (1, 1, 127), 0);
    plugin.processBlock (buffer, midi);

    REQUIRE (*plugin.getAPVTS().getRawParameterValue (sendAmount) == Catch::Approx (0.0f));
}

TEST_CASE ("Plugin accepts MIDI input", "[midi][send]")
{
    sendbloom::PluginProcessor plugin;
    REQUIRE (plugin.acceptsMidi());
}
