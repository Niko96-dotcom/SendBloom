#include <PluginProcessor.h>
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
