#include "helpers/test_helpers.h"
#include <ParameterIDs.h>
#include <PluginProcessor.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <cmath>

TEST_CASE ("Passthrough preserves audio at unity gain", "[dsp][passthrough]")
{
    using namespace sendbloom::ParameterIDs;

    sendbloom::PluginProcessor plugin;
    auto& apvts = plugin.getAPVTS();
    *apvts.getRawParameterValue (inputGain) = 1.0f;
    *apvts.getRawParameterValue (outputGain) = 3.0f;
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
    plugin.prepareToPlay (48000.0, 512);

    juce::AudioBuffer<float> buffer (2, 512);
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 512; ++i)
            buffer.setSample (ch, i, 0.5f);

    auto& apvts = plugin.getAPVTS();
    *apvts.getRawParameterValue (inputGain) = 0.0f;

    juce::MidiBuffer midi;
    plugin.processBlock (buffer, midi);

    *apvts.getRawParameterValue (inputGain) = 1.0f;
    plugin.processBlock (buffer, midi);

    float maxDelta = 0.0f;
    for (int i = 1; i < 512; ++i)
        maxDelta = std::max (maxDelta, std::abs (buffer.getSample (0, i) - buffer.getSample (0, i - 1)));

    const auto blockStep = std::abs (buffer.getSample (0, 400) - buffer.getSample (0, 200));
    REQUIRE (blockStep > 0.0f);
    REQUIRE (maxDelta < blockStep);
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
