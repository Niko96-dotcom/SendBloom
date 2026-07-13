#include <ParameterIDs.h>
#include <PluginProcessor.h>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>

namespace
{

std::string readTextFile (const juce::File& file)
{
    std::ifstream stream (file.getFullPathName().toStdString());
    std::ostringstream buffer;
    buffer << stream.rdbuf();
    return buffer.str();
}

juce::File findRepoRoot()
{
    return juce::File { SENDBLOOM_SOURCE_DIR };
}

} // namespace

TEST_CASE ("v1 exposes no reverb engine selector", "[v1][contract][fixed-rate]")
{
    sendbloom::PluginProcessor plugin;

    REQUIRE (plugin.getAPVTS().getParameter ("authentic_color") == nullptr);
    REQUIRE (plugin.getParameters().size() == 13);
}

TEST_CASE ("v1 production reverb is fixed ProperSRC without crossfade machinery",
           "[v1][contract][fixed-rate][source-scan]")
{
    const auto root = findRepoRoot();
    const auto tank = readTextFile (root.getChildFile ("source/SchroederTank32.h"));
    const auto adapter = readTextFile (root.getChildFile ("source/FixedRateAdapter.h"));
    const auto processor = readTextFile (root.getChildFile ("source/PluginProcessor.cpp"));

    REQUIRE (tank.find ("FixedRateAdapter") != std::string::npos);
    REQUIRE (tank.find ("fixedRate_.processBlock") != std::string::npos);
    REQUIRE (tank.find ("HostRateReverbEngine") == std::string::npos);
    REQUIRE (tank.find ("EngineCrossfade") == std::string::npos);
    REQUIRE (adapter.find ("processProperSrc") != std::string::npos);
    REQUIRE (adapter.find ("quantize9bit") == std::string::npos);
    REQUIRE (adapter.find ("SENDBLOOM_ENABLE_DIAGNOSTICS") != std::string::npos);
    REQUIRE (processor.find ("requestedAuthenticColor") == std::string::npos);
    REQUIRE (processor.find ("requestEngineCrossfade") == std::string::npos);
}

TEST_CASE ("v1 fixed ProperSRC remains finite at supported host rates",
           "[v1][contract][fixed-rate][multi-rate]")
{
    for (const auto sampleRate : { 44100.0, 48000.0, 88200.0, 96000.0 })
    {
        sendbloom::PluginProcessor plugin;
        plugin.prepareToPlay (sampleRate, 256);

        juce::AudioBuffer<float> buffer (2, 256);
        juce::MidiBuffer midi;

        for (int block = 0; block < 12; ++block)
        {
            for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
                for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
                    buffer.setSample (channel, sample,
                                      0.25f * std::sin (0.03f * static_cast<float> (block * 256 + sample)));

            plugin.processBlock (buffer, midi);
        }

        INFO ("sample rate " << sampleRate);
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
                REQUIRE (std::isfinite (buffer.getSample (channel, sample)));
    }
}
