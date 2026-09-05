#include <ParameterSnapshot.h>
#include <ParameterIDs.h>
#include <PluginProcessor.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

TEST_CASE ("ParameterSnapshot capture applies curve mappings", "[parm][snapshot]")
{
    using namespace sendbloom::ParameterIDs;

    sendbloom::PluginProcessor plugin;
    auto& apvts = plugin.getAPVTS();

    *apvts.getRawParameterValue (size) = 0.5f;
    *apvts.getRawParameterValue (distn) = 1.0f;
    *apvts.getRawParameterValue (level) = 0.5f;
    *apvts.getRawParameterValue (sendConnected) = 0.0f;
    *apvts.getRawParameterValue (sendAmount) = 0.5f;

    const auto snap = sendbloom::ParameterSnapshot::capture (apvts);

    REQUIRE (snap.sizeNorm == Catch::Approx (0.5f));
    REQUIRE (snap.distnBlend == Catch::Approx (1.0f));
    REQUIRE (snap.wetGain == Catch::Approx (std::sin (juce::MathConstants<float>::halfPi * 0.5f)).margin (1e-5f));
    REQUIRE_FALSE (snap.sendConnected);
}

TEST_CASE ("ParameterSnapshot captures pressure controls", "[parm][snapshot]")
{
    using namespace sendbloom::ParameterIDs;

    sendbloom::PluginProcessor plugin;
    auto& apvts = plugin.getAPVTS();

    apvts.getParameter (sendConnected)->setValueNotifyingHost (1.0f);
    *apvts.getRawParameterValue (sendAmount) = 0.5f;
    *apvts.getRawParameterValue (sendFeel) = 0.0f;

    const auto snap = sendbloom::ParameterSnapshot::capture (apvts);
    REQUIRE (snap.sendConnected);
    REQUIRE (snap.sendAmountNorm == Catch::Approx (0.5f));
    REQUIRE (snap.sendFirmFeel);
}
