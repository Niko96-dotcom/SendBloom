#include <ParameterSnapshot.h>
#include <ParameterIDs.h>
#include <PluginProcessor.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

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

    REQUIRE (snap.rt60Seconds == Catch::Approx (sendbloom::ParameterCurves::sizeToRT60 (0.5f)));
    REQUIRE (snap.distnBlend == Catch::Approx (1.0f));
    REQUIRE (snap.wetGain == Catch::Approx (snap.dryGain).margin (1e-5f));
    REQUIRE (snap.sendGain == Catch::Approx (1.0f));
}

TEST_CASE ("ParameterSnapshot send gain when connected", "[parm][snapshot]")
{
    using namespace sendbloom::ParameterIDs;

    sendbloom::PluginProcessor plugin;
    auto& apvts = plugin.getAPVTS();

    apvts.getParameter (sendConnected)->setValueNotifyingHost (1.0f);
    *apvts.getRawParameterValue (sendAmount) = 0.5f;
    *apvts.getRawParameterValue (sendFeel) = 0.0f;

    const auto snap = sendbloom::ParameterSnapshot::capture (apvts);
    REQUIRE (snap.sendGain == Catch::Approx (sendbloom::ParameterCurves::sendGain (0.5f, true)));
}
