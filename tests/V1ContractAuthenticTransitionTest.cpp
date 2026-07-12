#include <ParameterIDs.h>
#include <PluginProcessor.h>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

namespace
{

constexpr double kSampleRate = 48000.0;
constexpr int kPreparedBlock = 512;
/** Small host blocks so a 15 ms authentic smoother cannot cross 0.5 in one call. */
constexpr int kSmallBlock = 32;

void configureWetPlugin (sendbloom::PluginProcessor& plugin)
{
    using namespace sendbloom::ParameterIDs;

    auto& apvts = plugin.getAPVTS();
    *apvts.getRawParameterValue (inputGain) = 1.0f;
    *apvts.getRawParameterValue (outputGain) = 0.0f;
    *apvts.getRawParameterValue (bypass) = 0.0f;
    *apvts.getRawParameterValue (level) = 0.75f;
    *apvts.getRawParameterValue (distn) = 0.0f;
    *apvts.getRawParameterValue (size) = 0.55f;
    *apvts.getRawParameterValue (gatePrePost) = 0.0f;
    *apvts.getRawParameterValue (sendConnected) = 1.0f;
    *apvts.getRawParameterValue (sendAmount) = 0.85f;
    *apvts.getRawParameterValue (authenticColor) = 0.0f;
}

void fillImpulse (juce::AudioBuffer<float>& buffer, float amplitude = 0.4f)
{
    buffer.clear();
    if (buffer.getNumSamples() > 0)
    {
        buffer.setSample (0, 0, amplitude);
        if (buffer.getNumChannels() > 1)
            buffer.setSample (1, 0, amplitude);
    }
}

void processQuietBlocks (sendbloom::PluginProcessor& plugin, int count, int blockSize)
{
    juce::AudioBuffer<float> buffer (2, blockSize);
    juce::MidiBuffer midi;

    for (int i = 0; i < count; ++i)
    {
        buffer.clear();
        plugin.processBlock (buffer, midi);
    }
}

} // namespace

TEST_CASE ("v1 authentic toggle requests crossfade in first subsequent block (RT-08/RT-10)",
           "[v1][contract][authentic][RT-08][RT-10]")
{
    // ADR-V1-07 / D-09: one snapshot-edge request; must not wait for 15 ms smoother 0.5.
    using namespace sendbloom::ParameterIDs;

    sendbloom::PluginProcessor plugin;
    configureWetPlugin (plugin);
    plugin.prepareToPlay (kSampleRate, kPreparedBlock);

    processQuietBlocks (plugin, 4, kSmallBlock);
    REQUIRE_FALSE (plugin.chain.isCrossfading());
    REQUIRE (plugin.getLatencySamples() == 0);

    auto& apvts = plugin.getAPVTS();
    *apvts.getRawParameterValue (authenticColor) = 1.0f;

    juce::AudioBuffer<float> buffer (2, kSmallBlock);
    juce::MidiBuffer midi;
    fillImpulse (buffer);
    plugin.processBlock (buffer, midi);

    // Snapshot edge starts the engine crossfade inside this first processBlock.
    REQUIRE (plugin.chain.isCrossfading());
    REQUIRE (plugin.getLatencySamples() == 0);
}

TEST_CASE ("v1 authentic latency stays zero across transition (RT-09)",
           "[v1][contract][authentic][RT-09]")
{
    using namespace sendbloom::ParameterIDs;

    sendbloom::PluginProcessor plugin;
    configureWetPlugin (plugin);
    plugin.prepareToPlay (kSampleRate, kPreparedBlock);
    REQUIRE (plugin.getLatencySamples() == 0);

    auto& apvts = plugin.getAPVTS();
    juce::AudioBuffer<float> buffer (2, kPreparedBlock);
    juce::MidiBuffer midi;

    *apvts.getRawParameterValue (authenticColor) = 1.0f;
    fillImpulse (buffer);
    plugin.processBlock (buffer, midi);
    REQUIRE (plugin.getLatencySamples() == 0);

    // Drain crossfade (~35 ms @ 48 kHz).
    processQuietBlocks (plugin, 16, kPreparedBlock);
    REQUIRE (plugin.getLatencySamples() == 0);

    *apvts.getRawParameterValue (authenticColor) = 0.0f;
    fillImpulse (buffer);
    plugin.processBlock (buffer, midi);
    REQUIRE (plugin.getLatencySamples() == 0);

    processQuietBlocks (plugin, 16, kPreparedBlock);
    REQUIRE (plugin.getLatencySamples() == 0);
    REQUIRE_FALSE (plugin.chain.isCrossfading());
}

TEST_CASE ("v1 rapid authentic toggles converge to final snapshot (RT-11)",
           "[v1][contract][authentic][RT-11]")
{
    using namespace sendbloom::ParameterIDs;

    sendbloom::PluginProcessor plugin;
    configureWetPlugin (plugin);
    plugin.prepareToPlay (kSampleRate, kPreparedBlock);

    auto& apvts = plugin.getAPVTS();
    juce::AudioBuffer<float> buffer (2, kSmallBlock);
    juce::MidiBuffer midi;

    // Rapid block-to-block toggles; final snapshot is authentic ON.
    for (int block = 0; block < 40; ++block)
    {
        *apvts.getRawParameterValue (authenticColor) = (block % 2 == 0) ? 1.0f : 0.0f;
        fillImpulse (buffer, 0.2f);
        plugin.processBlock (buffer, midi);
        REQUIRE (plugin.getLatencySamples() == 0);
    }

    *apvts.getRawParameterValue (authenticColor) = 1.0f;
    processQuietBlocks (plugin, 24, kPreparedBlock);
    REQUIRE_FALSE (plugin.chain.isCrossfading());
    REQUIRE (plugin.getLatencySamples() == 0);

    // Settled on authentic ON: next OFF edge must start a fresh crossfade immediately.
    *apvts.getRawParameterValue (authenticColor) = 0.0f;
    fillImpulse (buffer);
    plugin.processBlock (buffer, midi);
    REQUIRE (plugin.chain.isCrossfading());
}
