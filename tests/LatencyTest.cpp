#include <ParameterCurves.h>
#include <ParameterIDs.h>
#include <PluginProcessor.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE ("Plugin reports zero latency samples", "[chain][latency]")
{
    sendbloom::PluginProcessor plugin;
    REQUIRE (plugin.getLatencySamples() == 0);
}

TEST_CASE ("Plugin latency unchanged after prepare", "[chain][latency]")
{
    sendbloom::PluginProcessor plugin;
    plugin.prepareToPlay (48000.0, 512);
    REQUIRE (plugin.getLatencySamples() == 0);
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
