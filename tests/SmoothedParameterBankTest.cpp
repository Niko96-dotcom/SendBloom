#include <SmoothedParameterBank.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE ("SmoothedParameterBank ramps toward target within 20 ms", "[parm][smooth]")
{
    sendbloom::SmoothedParameterBank bank;
    bank.prepare (48000.0);

    sendbloom::ParameterSnapshot snap;
    snap.inputGainDb = 0.0f;
    snap.outputGainLinear = 1.0f;
    bank.setTargets (snap);

    snap.inputGainDb = 6.0f;
    bank.setTargets (snap);

    const auto targetLinear = juce::Decibels::decibelsToGain (6.0f);
    float lastValue = 0.0f;

    for (int i = 0; i < 960; ++i)
        lastValue = bank.getNextInputGainLinear();

    REQUIRE (lastValue == Catch::Approx (targetLinear).margin (targetLinear * 0.01f));
}
