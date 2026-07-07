#include <DampedComb.h>
#include <FactoryPresets.h>
#include <ParameterIDs.h>
#include <PluginProcessor.h>
#include <SchroederTank32.h>
#include <BinaryData.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <array>
#include <cstdlib>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace
{

constexpr auto kSampleRate = 48000.0;

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

std::string readTextFile (const juce::File& file)
{
    std::ifstream stream (file.getFullPathName().toStdString());

    if (! stream)
        return {};

    std::ostringstream buffer;
    buffer << stream.rdbuf();
    return buffer.str();
}

juce::File findRepoRoot()
{
    auto dir = juce::File::getCurrentWorkingDirectory();

    for (int depth = 0; depth < 8; ++depth)
    {
        const auto cmakeLists = dir.getChildFile ("CMakeLists.txt");

        if (cmakeLists.existsAsFile())
        {
            const auto cmakeText = readTextFile (cmakeLists);

            if (cmakeText.find ("SendBloom") != std::string::npos)
                return dir;
        }

        dir = dir.getParentDirectory();
    }

    return juce::File::getCurrentWorkingDirectory();
}

std::string extractProcessBlockBody (const std::string& source)
{
    const auto start = source.find ("void PluginProcessor::processBlock");

    if (start == std::string::npos)
        return {};

    const auto openBrace = source.find ('{', start);

    if (openBrace == std::string::npos)
        return {};

    int depth = 0;

    for (size_t i = openBrace; i < source.size(); ++i)
    {
        if (source[i] == '{')
            ++depth;
        else if (source[i] == '}')
        {
            --depth;

            if (depth == 0)
                return source.substr (openBrace, i - openBrace + 1);
        }
    }

    return {};
}

float renderMonoDryRms (float inputGainNorm, float levelNorm)
{
    using namespace sendbloom::ParameterIDs;

    sendbloom::PluginProcessor plugin;
    auto& apvts = plugin.getAPVTS();
    *apvts.getRawParameterValue (inputGain) = inputGainNorm;
    *apvts.getRawParameterValue (outputGain) = 0.0f;
    *apvts.getRawParameterValue (bypass) = 0.0f;
    *apvts.getRawParameterValue (level) = levelNorm;
    *apvts.getRawParameterValue (distn) = 0.0f;
    *apvts.getRawParameterValue (size) = 0.0f;
    *apvts.getRawParameterValue (sendConnected) = 0.0f;
    *apvts.getRawParameterValue (gatePrePost) = 0.0f;
    plugin.prepareToPlay (kSampleRate, 512);

    juce::AudioBuffer<float> buffer (1, 512);
    juce::MidiBuffer midi;

    for (int block = 0; block < 24; ++block)
    {
        for (int i = 0; i < 512; ++i)
            buffer.setSample (0, i, 0.2f * std::sin (0.03f * static_cast<float> (block * 512 + i)));

        plugin.processBlock (buffer, midi);
    }

    return bufferRms (buffer, 0, 256, 256);
}

std::vector<float> renderImpulse (sendbloom::SchroederTank32& tank, bool authentic, int numSamples)
{
    std::vector<float> out (static_cast<size_t> (numSamples), 0.0f);

    for (int i = 0; i < numSamples; ++i)
    {
        const auto in = i == 0 ? 1.0f : 0.0f;
        out[static_cast<size_t> (i)] = tank.processSample (in, 1.5f, 0.0f, authentic);
    }

    return out;
}

} // namespace

TEST_CASE ("legal metadata audit script passes on product-facing files", "[release][legal]")
{
    const auto script = findRepoRoot().getChildFile ("scripts/check-legal-metadata.sh");
    REQUIRE (script.existsAsFile());

    const auto command = "bash \"" + script.getFullPathName() + "\"";
    REQUIRE (std::system (command.toRawUTF8()) == 0);
}

TEST_CASE ("processBlock does not call setValueNotifyingHost", "[release][realtime]")
{
    const auto sourcePath = findRepoRoot().getChildFile ("source/PluginProcessor.cpp");
    const auto source = readTextFile (sourcePath);
    const auto body = extractProcessBlockBody (source);

    REQUIRE_FALSE (body.empty());
    REQUIRE (body.find ("setValueNotifyingHost") == std::string::npos);
}

TEST_CASE ("32k Color docs describe software model not firmware claims", "[release][verb][authentic]")
{
    const auto root = findRepoRoot();
    const auto readme = readTextFile (root.getChildFile ("README.md"));
    const auto checklist = readTextFile (root.getChildFile ("docs/RELEASE_CHECKLIST.md"));
    const auto tankSource = readTextFile (root.getChildFile ("source/SchroederTank32.h"));

    REQUIRE (readme.find ("firmware-derived") != std::string::npos);
    REQUIRE (readme.find ("32,768 Hz") != std::string::npos);
    REQUIRE (readme.find ("EEPROM") == std::string::npos);
    REQUIRE (readme.find ("bytecode") == std::string::npos);
    REQUIRE (checklist.find ("not firmware-derived") != std::string::npos);
    REQUIRE (tankSource.find ("processAuthentic") != std::string::npos);
    REQUIRE (tankSource.find ("kInternalRate") != std::string::npos);
}

TEST_CASE ("input_gain colors wet path only; dry tap stays pre-gain", "[release][io][input_gain]")
{
    const auto dryLowGain = renderMonoDryRms (0.0f, 0.0f);
    const auto dryHighGain = renderMonoDryRms (1.0f, 0.0f);

    REQUIRE (dryLowGain == Catch::Approx (dryHighGain).margin (0.02f));

    using namespace sendbloom::ParameterIDs;

    sendbloom::PluginProcessor plugin;
    auto& apvts = plugin.getAPVTS();
    *apvts.getRawParameterValue (inputGain) = 0.0f;
    *apvts.getRawParameterValue (outputGain) = 0.0f;
    *apvts.getRawParameterValue (bypass) = 0.0f;
    *apvts.getRawParameterValue (level) = 1.0f;
    *apvts.getRawParameterValue (distn) = 0.0f;
    *apvts.getRawParameterValue (size) = 0.75f;
    *apvts.getRawParameterValue (sendConnected) = 0.0f;
    *apvts.getRawParameterValue (gatePrePost) = 0.0f;
    plugin.prepareToPlay (kSampleRate, 512);

    juce::AudioBuffer<float> buffer (1, 512);
    juce::MidiBuffer midi;

    for (int block = 0; block < 32; ++block)
    {
        for (int i = 0; i < 512; ++i)
            buffer.setSample (0, i, 0.25f);

        plugin.processBlock (buffer, midi);
    }

    const auto wetLow = bufferRms (buffer, 0, 256, 256);

    *apvts.getRawParameterValue (inputGain) = 1.0f;

    for (int block = 0; block < 32; ++block)
    {
        for (int i = 0; i < 512; ++i)
            buffer.setSample (0, i, 0.25f);

        plugin.processBlock (buffer, midi);
    }

    const auto wetHigh = bufferRms (buffer, 0, 256, 256);
    REQUIRE (wetHigh > wetLow * 1.2f);
}

TEST_CASE ("Authentic mode outputs dual-mono for asymmetric stereo input", "[release][io][mono]")
{
    using namespace sendbloom::ParameterIDs;

    sendbloom::PluginProcessor plugin;
    auto& apvts = plugin.getAPVTS();
    *apvts.getRawParameterValue (inputGain) = 0.5f;
    *apvts.getRawParameterValue (outputGain) = 0.0f;
    *apvts.getRawParameterValue (bypass) = 0.0f;
    *apvts.getRawParameterValue (level) = 0.6f;
    *apvts.getRawParameterValue (size) = 0.5f;
    *apvts.getRawParameterValue (extendedStereo) = 0.0f;
    *apvts.getRawParameterValue (sendConnected) = 1.0f;
    *apvts.getRawParameterValue (sendAmount) = 1.0f;
    plugin.prepareToPlay (kSampleRate, 512);

    juce::AudioBuffer<float> buffer (2, 512);
    juce::MidiBuffer midi;

    for (int block = 0; block < 24; ++block)
    {
        for (int i = 0; i < 512; ++i)
        {
            const auto sample = std::sin (0.04f * static_cast<float> (block * 512 + i));
            buffer.setSample (0, i, 0.9f * sample);
            buffer.setSample (1, i, 0.1f * sample);
        }

        plugin.processBlock (buffer, midi);
    }

    for (int i = 256; i < 512; ++i)
        REQUIRE (buffer.getSample (0, i) == Catch::Approx (buffer.getSample (1, i)).margin (1e-5f));
}

TEST_CASE ("authentic_color produces distinct response from host-rate path", "[release][verb][authentic]")
{
    sendbloom::SchroederTank32 hostTank;
    sendbloom::SchroederTank32 authTank;
    hostTank.prepare (kSampleRate, 512);
    authTank.prepare (kSampleRate, 512);

    const auto hostIr = renderImpulse (hostTank, false, 16384);
    const auto authIr = renderImpulse (authTank, true, 16384);

    float maxDiff = 0.0f;

    for (size_t i = 512; i < hostIr.size(); ++i)
        maxDiff = std::max (maxDiff, std::abs (hostIr[i] - authIr[i]));

    REQUIRE (maxDiff > 1.0e-4f);
}

TEST_CASE ("DampedComb RT60 feedback depends on comb delay reference", "[release][verb][comb]")
{
    sendbloom::DampedComb combA;
    sendbloom::DampedComb combB;
    combA.prepare (kSampleRate, 8192);
    combB.prepare (kSampleRate, 8192);

    combA.setDelay (1000.0f);
    combB.setDelay (2000.0f);

    combA.setFeedbackForRT60 (1.5f, 1000.0f);
    combB.setFeedbackForRT60 (1.5f, 2000.0f);

    REQUIRE (combA.getFeedback() != Catch::Approx (combB.getFeedback()).margin (1e-6f));
    REQUIRE (combB.getFeedback() < combA.getFeedback());
}

TEST_CASE ("setCurrentProgram loads embedded XML preset values", "[release][preset][xml]")
{
    using namespace sendbloom::ParameterIDs;

    struct PresetXml
    {
        const char* data;
        int size;
    };

    const PresetXml presetXml[] {
        { BinaryData::Sparkle_Verb_xml, BinaryData::Sparkle_Verb_xmlSize },
        { BinaryData::Cut_Sample_Gate_xml, BinaryData::Cut_Sample_Gate_xmlSize },
        { BinaryData::Spacerock_Burn_xml, BinaryData::Spacerock_Burn_xmlSize },
        { BinaryData::Dry_Dub_Sends_xml, BinaryData::Dry_Dub_Sends_xmlSize },
        { BinaryData::Dark_Bloom_xml, BinaryData::Dark_Bloom_xmlSize },
        { BinaryData::Firm_Pressure_xml, BinaryData::Firm_Pressure_xmlSize },
        { BinaryData::Gated_Room_xml, BinaryData::Gated_Room_xmlSize },
        { BinaryData::Hot_Clip_xml, BinaryData::Hot_Clip_xmlSize },
    };

    for (int preset = 0; preset < sendbloom::FactoryPresets::kNumPresets; ++preset)
    {
        sendbloom::PluginProcessor fromProgram;
        fromProgram.setCurrentProgram (preset);

        sendbloom::PluginProcessor fromXml;
        const auto xml = juce::parseXML (juce::String (presetXml[preset].data, presetXml[preset].size));
        REQUIRE (xml != nullptr);

        juce::MemoryBlock block;
        juce::AudioProcessor::copyXmlToBinary (*xml, block);
        fromXml.setStateInformation (block.getData(), static_cast<int> (block.getSize()));

        const auto& programApvts = fromProgram.getAPVTS();
        const auto& xmlApvts = fromXml.getAPVTS();

        INFO ("preset index " << preset);

        for (const auto* id : { inputGain, inputThreshold, size, level, distn, outputGain,
                                darkMode, gatePrePost, sendConnected, sendAmount, sendFeel,
                                authenticColor, extendedStereo, dirtOs, bypass })
        {
            REQUIRE (programApvts.getRawParameterValue (id)->load()
                     == Catch::Approx (xmlApvts.getRawParameterValue (id)->load()).margin (1e-4f));
        }
    }
}

TEST_CASE ("MIDI CC1 updates send_amount without host notification", "[release][midi][realtime]")
{
    using namespace sendbloom::ParameterIDs;

    sendbloom::PluginProcessor plugin;
    auto& apvts = plugin.getAPVTS();
    *apvts.getRawParameterValue (inputGain) = 1.0f;
    *apvts.getRawParameterValue (outputGain) = 0.0f;
    *apvts.getRawParameterValue (bypass) = 0.0f;
    *apvts.getRawParameterValue (level) = 0.0f;
    *apvts.getRawParameterValue (distn) = 0.0f;
    *apvts.getRawParameterValue (size) = 0.5f;
    *apvts.getRawParameterValue (gatePrePost) = 0.0f;
    *apvts.getRawParameterValue (sendConnected) = 1.0f;
    *apvts.getRawParameterValue (sendAmount) = 0.0f;
    plugin.prepareToPlay (kSampleRate, 512);

    juce::AudioBuffer<float> buffer (2, 128);
    juce::MidiBuffer midi;
    buffer.clear();
    midi.addEvent (juce::MidiMessage::controllerEvent (1, 1, 64), 0);

    plugin.processBlock (buffer, midi);

    REQUIRE (*apvts.getRawParameterValue (sendAmount)
             == Catch::Approx (64.0f / 127.0f).margin (0.02f));
}
