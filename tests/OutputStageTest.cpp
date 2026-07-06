#include <OutputStage.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE ("OutputStage applies gain trim", "[io][OutputStage]")
{
    const auto gain = juce::Decibels::decibelsToGain (-6.0f);
    const auto out = sendbloom::OutputStage::processSample (1.0f, gain);
    REQUIRE (out == Catch::Approx (0.5f).margin (0.002f));
}

TEST_CASE ("OutputStage unity at 0 dB", "[io][OutputStage]")
{
    const auto out = sendbloom::OutputStage::processSample (0.75f, 1.0f);
    REQUIRE (out == Catch::Approx (0.75f));
}
