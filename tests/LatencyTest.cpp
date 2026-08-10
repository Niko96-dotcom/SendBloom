#include <ParameterCurves.h>
#include <ParameterIDs.h>
#include <PluginProcessor.h>
#include <RateConverterPair.h>
#include <SrcLatencyTable.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cmath>
#include <utility>
#include <vector>

namespace
{

constexpr int kBlockSize = 512;

int liveProperSrcLatency (double sampleRate, int maxBlockSize)
{
    sendbloom::RateConverterPair converters;
    converters.prepare (sampleRate, maxBlockSize);
    return converters.getRoundTripLatencySamples();
}

enum class DirectRoute
{
    engaged,
    apvtsBypass,
    hostBypass,
};

std::vector<float> renderDirectImpulse (sendbloom::PluginProcessor& plugin,
                                        DirectRoute route)
{
    constexpr int totalSamples = kBlockSize * 3;
    std::vector<float> rendered;
    rendered.reserve (totalSamples);
    juce::MidiBuffer midi;

    for (int offset = 0; offset < totalSamples; offset += kBlockSize)
    {
        juce::AudioBuffer<float> block (1, kBlockSize);
        block.clear();

        if (offset == 0)
            block.setSample (0, 0, 1.0f);

        if (route == DirectRoute::hostBypass)
            plugin.processBlockBypassed (block, midi);
        else
            plugin.processBlock (block, midi);

        for (int sample = 0; sample < kBlockSize; ++sample)
            rendered.push_back (block.getSample (0, sample));
    }

    return rendered;
}

void requireImpulseAtReportedLatency (const std::vector<float>& rendered, int latencySamples)
{
    REQUIRE (static_cast<int> (rendered.size()) > latencySamples);

    for (int sample = 0; sample < latencySamples; ++sample)
        REQUIRE (rendered[static_cast<size_t> (sample)] == Catch::Approx (0.0f).margin (1.0e-6f));

    REQUIRE (rendered[static_cast<size_t> (latencySamples)] == Catch::Approx (1.0f).margin (1.0e-6f));
}

void configureDirectOnly (sendbloom::PluginProcessor& plugin, bool apvtsBypass)
{
    using namespace sendbloom::ParameterIDs;

    auto& apvts = plugin.getAPVTS();
    *apvts.getRawParameterValue (level) = 0.0f;
    *apvts.getRawParameterValue (outputGain) = 0.0f;
    *apvts.getRawParameterValue (bypass) = apvtsBypass ? 1.0f : 0.0f;
}

} // namespace

TEST_CASE ("Plugin has no PDC before it is prepared", "[chain][latency]")
{
    sendbloom::PluginProcessor plugin;
    REQUIRE (plugin.getLatencySamples() == 0);
}

TEST_CASE ("Plugin reports the live ProperSRC latency after prepare", "[chain][latency][PDC-01]")
{
    sendbloom::PluginProcessor plugin;
    plugin.prepareToPlay (48000.0, kBlockSize);

    REQUIRE (plugin.getLatencySamples() == liveProperSrcLatency (48000.0, kBlockSize));
    REQUIRE (plugin.getLatencySamples() == sendbloom::lookupRoundTripLatencySamples (48000.0));
}

TEST_CASE ("Plugin reports canonical ProperSRC PDC across supported host rates",
           "[chain][latency][PDC-01]")
{
    sendbloom::PluginProcessor plugin;

    for (const auto& row : sendbloom::kMeasuredLatencyTable)
    {
        plugin.prepareToPlay (row.hostRateHz, sendbloom::kMaxHostBlock);
        REQUIRE (plugin.getLatencySamples() == row.roundTripSamples);
        REQUIRE (plugin.getLatencySamples()
                 == liveProperSrcLatency (row.hostRateHz, sendbloom::kMaxHostBlock));
    }
}

TEST_CASE ("Plugin PDC follows the prepared live SRC topology, not a fixed table",
           "[chain][latency][PDC-01]")
{
    constexpr std::array<std::pair<double, int>, 4> preparations { {
        { 44100.0, 64 },
        { 48000.0, 127 },
        { 88200.0, 256 },
        { 96000.0, 1024 },
    } };

    sendbloom::PluginProcessor plugin;

    for (const auto& [sampleRate, maxBlock] : preparations)
    {
        plugin.prepareToPlay (sampleRate, maxBlock);
        REQUIRE (plugin.getLatencySamples() == liveProperSrcLatency (sampleRate, maxBlock));
    }
}

TEST_CASE ("Engaged and bypass direct routes emerge at the reported PDC latency",
           "[chain][latency][PDC-02]")
{
    constexpr double kSampleRate = 48000.0;

    for (const auto route : { DirectRoute::engaged, DirectRoute::apvtsBypass, DirectRoute::hostBypass })
    {
        sendbloom::PluginProcessor plugin;
        configureDirectOnly (plugin, route == DirectRoute::apvtsBypass);
        plugin.prepareToPlay (kSampleRate, kBlockSize);

        const auto reportedLatency = plugin.getLatencySamples();
        const auto rendered = renderDirectImpulse (plugin, route);
        requireImpulseAtReportedLatency (rendered, reportedLatency);
    }
}

TEST_CASE ("Plugin tail length tracks size RT60", "[chain][latency][tail]")
{
    using namespace sendbloom::ParameterIDs;

    sendbloom::PluginProcessor plugin;
    auto& apvts = plugin.getAPVTS();

    *apvts.getRawParameterValue (size) = 0.0f;
    *apvts.getRawParameterValue (darkMode) = 0.0f;
    REQUIRE (plugin.getTailLengthSeconds()
             == Catch::Approx (static_cast<double> (sendbloom::ParameterCurves::sizeToRT60 (0.0f))));

    *apvts.getRawParameterValue (size) = 1.0f;
    REQUIRE (plugin.getTailLengthSeconds()
             == Catch::Approx (static_cast<double> (sendbloom::ParameterCurves::sizeToRT60 (1.0f))));
}

TEST_CASE ("Plugin tail length includes dark predelay", "[chain][latency][tail]")
{
    using namespace sendbloom::ParameterIDs;

    sendbloom::PluginProcessor plugin;
    auto& apvts = plugin.getAPVTS();
    *apvts.getRawParameterValue (size) = 0.5f;
    *apvts.getRawParameterValue (darkMode) = 1.0f;

    const auto expected = static_cast<double> (sendbloom::ParameterCurves::sizeToRT60 (0.5f)) + 0.055;
    REQUIRE (plugin.getTailLengthSeconds() == Catch::Approx (expected));
}
