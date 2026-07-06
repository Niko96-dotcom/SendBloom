#include <FactoryPresets.h>
#include <ParameterIDs.h>
#include <PluginProcessor.h>
#include <BinaryData.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <juce_audio_processors/juce_audio_processors.h>
#include <set>

namespace
{

void expectParamNear (const sendbloom::PluginProcessor& plugin,
                     const char* paramId,
                     float expected,
                     float tolerance = 0.02f)
{
    const auto actual = plugin.getAPVTS().getRawParameterValue (paramId)->load();
    INFO ("param " << paramId << " expected " << expected << " got " << actual);
    REQUIRE (actual == Catch::Approx (expected).margin (tolerance));
}

} // namespace

TEST_CASE ("Factory presets exposes eight programs", "[preset][PRST-01]")
{
    sendbloom::PluginProcessor plugin;
    REQUIRE (plugin.getNumPrograms() == sendbloom::FactoryPresets::kNumPresets);
    REQUIRE (plugin.getProgramName (0) == "Sparkle Verb");
    REQUIRE (plugin.getProgramName (7) == "Hot Clip");
}

TEST_CASE ("Factory preset XML resources embedded in bundle", "[preset][PRST-01]")
{
    REQUIRE (BinaryData::Sparkle_Verb_xml != nullptr);
    REQUIRE (BinaryData::Sparkle_Verb_xmlSize > 0);
    REQUIRE (BinaryData::Hot_Clip_xmlSize > 0);
}

TEST_CASE ("Each factory preset loads distinct parameter values", "[preset][PRST-01]")
{
    sendbloom::PluginProcessor plugin;

    std::array<float, sendbloom::FactoryPresets::kNumPresets> sizes {};

    for (int i = 0; i < sendbloom::FactoryPresets::kNumPresets; ++i)
    {
        plugin.setCurrentProgram (i);
        sizes[static_cast<size_t> (i)] = plugin.getAPVTS().getRawParameterValue (sendbloom::ParameterIDs::size)->load();
        REQUIRE (sizes[static_cast<size_t> (i)] >= 0.0f);
        REQUIRE (sizes[static_cast<size_t> (i)] <= 1.0f);
    }

    REQUIRE (sizes[0] != Catch::Approx (sizes[7]).margin (1e-4f));
    const auto uniqueCount = std::set<float> (sizes.begin(), sizes.end()).size();
    REQUIRE (uniqueCount >= 6);
}

TEST_CASE ("Factory preset state round-trip preserves all parameters", "[preset][PRST-02]")
{
    using namespace sendbloom::ParameterIDs;

    sendbloom::PluginProcessor source;
    source.setCurrentProgram (3);

    juce::MemoryBlock state;
    source.getStateInformation (state);

    sendbloom::PluginProcessor restored;
    restored.setStateInformation (state.getData(), static_cast<int> (state.getSize()));

    const auto& src = source.getAPVTS();
    const auto& dst = restored.getAPVTS();

    for (const auto* id : { inputGain, inputThreshold, size, level, distn, outputGain,
                            darkMode, gatePrePost, sendConnected, sendAmount, sendFeel,
                            authenticColor, extendedStereo, dirtOs, bypass })
    {
        REQUIRE (dst.getRawParameterValue (id)->load()
                 == Catch::Approx (src.getRawParameterValue (id)->load()).margin (1e-4f));
    }
}

TEST_CASE ("Preset XML matches programmatic factory state", "[preset][PRST-02]")
{
    sendbloom::PluginProcessor pluginFromCode;
    pluginFromCode.setCurrentProgram (0);

    const auto xml = juce::parseXML (juce::String (BinaryData::Sparkle_Verb_xml,
                                                   static_cast<int> (BinaryData::Sparkle_Verb_xmlSize)));
    REQUIRE (xml != nullptr);

    sendbloom::PluginProcessor pluginFromXml;
    juce::MemoryBlock block;
    juce::AudioProcessor::copyXmlToBinary (*xml, block);
    pluginFromXml.setStateInformation (block.getData(), static_cast<int> (block.getSize()));

    using namespace sendbloom::ParameterIDs;
    const auto& codeApvts = pluginFromCode.getAPVTS();
    const auto& xmlApvts = pluginFromXml.getAPVTS();

    REQUIRE (xmlApvts.getRawParameterValue (size)->load()
             == Catch::Approx (codeApvts.getRawParameterValue (size)->load()).margin (1e-4f));
    REQUIRE (xmlApvts.getRawParameterValue (inputGain)->load()
             == Catch::Approx (codeApvts.getRawParameterValue (inputGain)->load()).margin (1e-4f));
}

TEST_CASE ("All factory presets round-trip through get/setStateInformation", "[preset][PRST-02]")
{
    using namespace sendbloom::ParameterIDs;

    for (int preset = 0; preset < sendbloom::FactoryPresets::kNumPresets; ++preset)
    {
        sendbloom::PluginProcessor source;
        source.setCurrentProgram (preset);

        juce::MemoryBlock state;
        source.getStateInformation (state);

        sendbloom::PluginProcessor restored;
        restored.setStateInformation (state.getData(), static_cast<int> (state.getSize()));

        const auto& src = source.getAPVTS();
        const auto& dst = restored.getAPVTS();

        INFO ("preset index " << preset);
        REQUIRE (dst.getRawParameterValue (level)->load()
                 == Catch::Approx (src.getRawParameterValue (level)->load()).margin (1e-4f));
        REQUIRE (dst.getRawParameterValue (distn)->load()
                 == Catch::Approx (src.getRawParameterValue (distn)->load()).margin (1e-4f));
    }
}
