#include "helpers/test_helpers.h"
#include <ParameterIDs.h>
#include <PluginProcessor.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <cmath>
#include <vector>

namespace
{

float bufferRms (const juce::AudioBuffer<float>& buffer, int channel = 0, int start = 0, int count = -1)
{
    if (count < 0)
        count = buffer.getNumSamples() - start;

    double sum = 0.0;

    for (int i = start; i < start + count; ++i)
    {
        const auto s = buffer.getSample (channel, i);
        sum += static_cast<double> (s) * static_cast<double> (s);
    }

    return static_cast<float> (std::sqrt (sum / static_cast<double> (count)));
}

void fillSine (juce::AudioBuffer<float>& buffer, float amplitude, float phaseInc)
{
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            buffer.setSample (ch, i, amplitude * std::sin (phaseInc * static_cast<float> (i)));
}

void renderPlugin (sendbloom::PluginProcessor& plugin,
                   juce::AudioBuffer<float>& buffer,
                   int blocks,
                   bool refillEachBlock = false,
                   float amplitude = 0.5f,
                   float phaseInc = 0.03f)
{
    juce::MidiBuffer midi;

    for (int block = 0; block < blocks; ++block)
    {
        if (refillEachBlock)
            fillSine (buffer, amplitude, phaseInc);

        plugin.processBlock (buffer, midi);
    }
}

} // namespace

TEST_CASE ("Passthrough preserves audio at unity gain", "[dsp][passthrough]")
{
    using namespace sendbloom::ParameterIDs;

    sendbloom::PluginProcessor plugin;
    auto& apvts = plugin.getAPVTS();
    *apvts.getRawParameterValue (inputGain) = 1.0f;
    *apvts.getRawParameterValue (outputGain) = 0.0f;
    *apvts.getRawParameterValue (bypass) = 1.0f;
    plugin.prepareToPlay (48000.0, 512);

    juce::AudioBuffer<float> buffer (2, 512);

    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 512; ++i)
            buffer.setSample (ch, i, std::sin (0.01f * static_cast<float> (i)));

    const auto expected = buffer;
    juce::MidiBuffer midi;

    for (int block = 0; block < 4; ++block)
        plugin.processBlock (buffer, midi);

    for (int ch = 0; ch < 2; ++ch)
        for (int i = 400; i < 512; ++i)
            REQUIRE (buffer.getSample (ch, i) == Catch::Approx (expected.getSample (ch, i)).margin (0.02f));
}

TEST_CASE ("APVTS state round-trip", "[parm][state]")
{
    using namespace sendbloom::ParameterIDs;

    sendbloom::PluginProcessor source;
    auto& apvts = source.getAPVTS();

    apvts.getParameter (inputGain)->setValueNotifyingHost (0.25f);
    apvts.getParameter (distn)->setValueNotifyingHost (0.75f);
    apvts.getParameter (bypass)->setValueNotifyingHost (1.0f);

    juce::MemoryBlock state;
    source.getStateInformation (state);

    sendbloom::PluginProcessor restored;
    restored.setStateInformation (state.getData(), static_cast<int> (state.getSize()));

    auto& restoredApvts = restored.getAPVTS();
    REQUIRE (restoredApvts.getRawParameterValue (inputGain)->load()
             == Catch::Approx (apvts.getRawParameterValue (inputGain)->load()).margin (1e-4f));
    REQUIRE (restoredApvts.getRawParameterValue (distn)->load()
             == Catch::Approx (apvts.getRawParameterValue (distn)->load()).margin (1e-4f));
    REQUIRE (restoredApvts.getRawParameterValue (bypass)->load()
             == Catch::Approx (apvts.getRawParameterValue (bypass)->load()).margin (1e-4f));
}

TEST_CASE ("Discrete switch state restores after non-discrete host input", "[parm][state][pluginval]")
{
    using namespace sendbloom::ParameterIDs;

    for (const auto* id : { darkMode, sendConnected, authenticColor, extendedStereo, bypass })
    {
        sendbloom::PluginProcessor plugin;
        auto* parameter = plugin.getAPVTS().getParameter (id);
        REQUIRE (parameter != nullptr);

        parameter->setValueNotifyingHost (0.0f);
        juce::MemoryBlock state;
        plugin.getStateInformation (state);

        // pluginval deliberately fuzzes discrete parameters with arbitrary
        // normalised values before asking the processor to restore its state.
        parameter->setValueNotifyingHost (0.346531f);
        plugin.setStateInformation (state.getData(), static_cast<int> (state.getSize()));

        INFO ("parameter " << id);
        REQUIRE (parameter->getValue() == Catch::Approx (0.0f).margin (1.0e-6f));
    }
}

TEST_CASE ("getBypassParameter returns bypass param", "[parm][layout]")
{
    sendbloom::PluginProcessor plugin;
    REQUIRE (plugin.getBypassParameter() != nullptr);
    REQUIRE (plugin.getBypassParameter() == plugin.getAPVTS().getParameter (sendbloom::ParameterIDs::bypass));
}

TEST_CASE ("Smoothed gain automation avoids block-constant zipper", "[parm][zipper]")
{
    using namespace sendbloom::ParameterIDs;

    sendbloom::PluginProcessor plugin;
    auto& apvts = plugin.getAPVTS();
    *apvts.getRawParameterValue (inputGain) = 0.0f;
    *apvts.getRawParameterValue (outputGain) = 0.0f;
    *apvts.getRawParameterValue (bypass) = 0.0f;
    *apvts.getRawParameterValue (distn) = 0.0f;
    *apvts.getRawParameterValue (level) = 0.5f;
    plugin.prepareToPlay (48000.0, 512);

    juce::AudioBuffer<float> buffer (2, 512);
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 512; ++i)
            buffer.setSample (ch, i, 0.5f);

    juce::MidiBuffer midi;
    for (int block = 0; block < 3; ++block)
        plugin.processBlock (buffer, midi);

    *apvts.getRawParameterValue (inputGain) = 1.0f;
    plugin.processBlock (buffer, midi);

    float maxDelta = 0.0f;
    for (int i = 200; i < 512; ++i)
        maxDelta = std::max (maxDelta, std::abs (buffer.getSample (0, i) - buffer.getSample (0, i - 1)));

    const auto instantStep = std::abs (0.5f * juce::Decibels::decibelsToGain (-3.0f)
                                       - 0.5f * juce::Decibels::decibelsToGain (9.0f));
    REQUIRE (maxDelta < instantStep * 0.5f);
}

TEST_CASE ("Bypass toggle avoids full-scale clicks", "[parm][bypass]")
{
    using namespace sendbloom::ParameterIDs;

    sendbloom::PluginProcessor plugin;
    auto& apvts = plugin.getAPVTS();
    *apvts.getRawParameterValue (inputGain) = 1.0f;
    *apvts.getRawParameterValue (outputGain) = 3.0f;
    *apvts.getRawParameterValue (bypass) = 0.0f;
    plugin.prepareToPlay (48000.0, 512);

    juce::AudioBuffer<float> buffer (2, 512);
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 512; ++i)
            buffer.setSample (ch, i, std::sin (0.02f * static_cast<float> (i)));

    juce::MidiBuffer midi;
    plugin.processBlock (buffer, midi);

    *apvts.getRawParameterValue (bypass) = 1.0f;
    plugin.processBlock (buffer, midi);

    float maxDelta = 0.0f;
    for (int i = 1; i < 512; ++i)
        maxDelta = std::max (maxDelta, std::abs (buffer.getSample (0, i) - buffer.getSample (0, i - 1)));

    REQUIRE (maxDelta < 1.0f);
}

TEST_CASE ("distn automation changes processor output", "[parm][distn audibility]")
{
    using namespace sendbloom::ParameterIDs;

    auto runWithDistn = [] (float distnValue)
    {
        sendbloom::PluginProcessor plugin;
        auto& apvts = plugin.getAPVTS();
        *apvts.getRawParameterValue (inputGain) = 1.0f;
        *apvts.getRawParameterValue (outputGain) = 0.0f;
        *apvts.getRawParameterValue (bypass) = 0.0f;
        *apvts.getRawParameterValue (distn) = distnValue;
        *apvts.getRawParameterValue (level) = 1.0f;
        *apvts.getRawParameterValue (size) = 0.5f;
        *apvts.getRawParameterValue (sendConnected) = 1.0f;
        *apvts.getRawParameterValue (sendAmount) = 1.0f;
        plugin.prepareToPlay (48000.0, 512);

        juce::AudioBuffer<float> buffer (2, 512);
        fillSine (buffer, 0.5f, 0.03f);
        renderPlugin (plugin, buffer, 40, true);

        return buffer.getSample (0, 400);
    };

    REQUIRE (runWithDistn (1.0f) != Catch::Approx (runWithDistn (0.0f)).margin (1e-4f));
}

TEST_CASE ("Plugin instance", "[instance]")
{
    sendbloom::PluginProcessor testPlugin;

    SECTION ("name")
    {
        CHECK_THAT (testPlugin.getName().toStdString(),
                    Catch::Matchers::Equals ("SendBloom"));
    }
}

TEST_CASE ("dry unchanged when distn max with level at zero", "[chain][routing][integration]")
{
    using namespace sendbloom::ParameterIDs;

    auto runAtDistn = [] (float distnValue)
    {
        sendbloom::PluginProcessor plugin;
        auto& apvts = plugin.getAPVTS();
        *apvts.getRawParameterValue (inputGain) = 1.0f;
        *apvts.getRawParameterValue (outputGain) = 0.0f;
        *apvts.getRawParameterValue (bypass) = 0.0f;
        *apvts.getRawParameterValue (level) = 0.0f;
        *apvts.getRawParameterValue (distn) = distnValue;
        *apvts.getRawParameterValue (sendConnected) = 1.0f;
        *apvts.getRawParameterValue (sendAmount) = 1.0f;
        plugin.prepareToPlay (48000.0, 512);

        juce::AudioBuffer<float> buffer (2, 512);
        fillSine (buffer, 0.5f, 0.03f);
        renderPlugin (plugin, buffer, 40, true);
        return bufferRms (buffer);
    };

    REQUIRE (runAtDistn (1.0f) == Catch::Approx (runAtDistn (0.0f)).margin (1e-3f));
}

TEST_CASE ("level scales wet return not dry tap", "[chain][routing][integration]")
{
    using namespace sendbloom::ParameterIDs;

    auto runAtLevel = [] (float levelValue)
    {
        sendbloom::PluginProcessor plugin;
        auto& apvts = plugin.getAPVTS();
        *apvts.getRawParameterValue (inputGain) = 1.0f;
        *apvts.getRawParameterValue (outputGain) = 0.0f;
        *apvts.getRawParameterValue (bypass) = 0.0f;
        *apvts.getRawParameterValue (level) = levelValue;
        *apvts.getRawParameterValue (distn) = 0.0f;
        *apvts.getRawParameterValue (size) = 0.5f;
        *apvts.getRawParameterValue (sendConnected) = 1.0f;
        *apvts.getRawParameterValue (sendAmount) = 1.0f;
        plugin.prepareToPlay (48000.0, 512);

        juce::AudioBuffer<float> buffer (2, 512);
        fillSine (buffer, 0.5f, 0.03f);
        renderPlugin (plugin, buffer, 40, true);
        return bufferRms (buffer);
    };

    const auto lowLevel = runAtLevel (0.0f);
    const auto highLevel = runAtLevel (1.0f);
    REQUIRE (std::abs (highLevel - lowLevel) > 1e-3f);
}

TEST_CASE ("post gate chops wet after input stops", "[chain][routing][integration]")
{
    using namespace sendbloom::ParameterIDs;

    sendbloom::PluginProcessor plugin;
    auto& apvts = plugin.getAPVTS();
    *apvts.getRawParameterValue (inputGain) = 1.0f;
    *apvts.getRawParameterValue (outputGain) = 0.0f;
    *apvts.getRawParameterValue (bypass) = 0.0f;
    *apvts.getRawParameterValue (level) = 0.75f;
    *apvts.getRawParameterValue (distn) = 0.3f;
    *apvts.getRawParameterValue (size) = 0.5f;
    *apvts.getRawParameterValue (sendConnected) = 1.0f;
    *apvts.getRawParameterValue (sendAmount) = 1.0f;
    *apvts.getRawParameterValue (gatePrePost) = 1.0f;
    *apvts.getRawParameterValue (inputThreshold) = 0.35f;
    plugin.prepareToPlay (48000.0, 512);

    juce::AudioBuffer<float> burst (2, 512);
    fillSine (burst, 0.5f, 0.04f);

    juce::AudioBuffer<float> silence (2, 512);
    silence.clear();

    juce::MidiBuffer midi;

    for (int block = 0; block < 40; ++block)
    {
        fillSine (burst, 0.5f, 0.04f);
        plugin.processBlock (burst, midi);
    }

    const auto burstRms = bufferRms (burst, 0, 256, 256);

    for (int block = 0; block < 12; ++block)
    {
        silence.clear();
        plugin.processBlock (silence, midi);
    }

    const auto silenceRms = bufferRms (silence, 0, 256, 256);
    REQUIRE (silenceRms < 0.02f * burstRms);
}

TEST_CASE ("processBlock completes without throw", "[chain][routing][integration]")
{
    sendbloom::PluginProcessor plugin;
    plugin.prepareToPlay (48000.0, 512);

    juce::AudioBuffer<float> buffer (2, 512);
    fillSine (buffer, 0.25f, 0.02f);
    juce::MidiBuffer midi;

    REQUIRE_NOTHROW (plugin.processBlock (buffer, midi));
}
