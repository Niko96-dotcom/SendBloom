#include "helpers/test_helpers.h"
#include <PluginProcessor.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <cmath>

TEST_CASE ("Passthrough preserves audio", "[dsp][passthrough]")
{
    sendbloom::PluginProcessor plugin;
    plugin.prepareToPlay (48000.0, 512);

    juce::AudioBuffer<float> buffer (2, 512);

    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 512; ++i)
            buffer.setSample (ch, i, std::sin (0.01f * static_cast<float> (i)));

    const auto expected = buffer;
    juce::MidiBuffer midi;
    plugin.processBlock (buffer, midi);

    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 512; ++i)
            REQUIRE (buffer.getSample (ch, i) == expected.getSample (ch, i));
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
