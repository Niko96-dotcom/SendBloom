#include <ParameterIDs.h>
#include <PluginProcessor.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <fstream>
#include <sstream>
#include <string>

namespace
{

void configureSendPlugin (sendbloom::PluginProcessor& plugin, bool connected, float amount)
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
    *apvts.getRawParameterValue (sendAmount) = amount;
    plugin.prepareToPlay (48000.0, 512);
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

} // namespace

TEST_CASE ("v1 MIDI CC1 must not mutate APVTS send_amount",
           "[v1][contract][midi-apvts][MIDI-03]")
{
    // MIDI-03: with send_connected true, processBlock CC1 must leave APVTS send_amount unchanged.
    // Current processBlock stores CC1 into sendParam (MidiSendAmountTest documents that — leave untouched).
    // DSP-effect half of §8.4 (pressure still changes without APVTS write) awaits Phase 22 RT path (research A1).
    using namespace sendbloom::ParameterIDs;

    constexpr float kKnownAmount = 0.42f;

    sendbloom::PluginProcessor plugin;
    configureSendPlugin (plugin, true, kKnownAmount);

    const float before = plugin.getAPVTS().getRawParameterValue (sendAmount)->load();
    REQUIRE (before == Catch::Approx (kKnownAmount).margin (1.0e-4f));

    juce::AudioBuffer<float> buffer (2, 512);
    juce::MidiBuffer midi;
    buffer.clear();
    midi.addEvent (juce::MidiMessage::controllerEvent (1, 1, 100), 0);
    plugin.processBlock (buffer, midi);

    const float after = plugin.getAPVTS().getRawParameterValue (sendAmount)->load();
    // Intended failure: processBlock mutates APVTS via sendParam->store.
    REQUIRE (after == Catch::Approx (before).margin (1.0e-6f));
}

TEST_CASE ("v1 processBlock must not store MIDI into send_amount APVTS",
           "[v1][contract][midi-apvts][MIDI-03][source-scan]")
{
    const auto root = findRepoRoot();
    const auto source = readTextFile (root.getChildFile ("source/PluginProcessor.cpp"));
    REQUIRE_FALSE (source.empty());

    const auto body = extractProcessBlockBody (source);
    REQUIRE_FALSE (body.empty());

    // Intended failure while raw APVTS write remains in the realtime path.
    REQUIRE (body.find ("sendParam->store") == std::string::npos);
}
