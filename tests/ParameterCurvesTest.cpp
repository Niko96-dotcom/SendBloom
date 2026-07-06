#include <ParameterCurves.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

using namespace sendbloom::ParameterCurves;

TEST_CASE ("sizeToRT60 matches architecture curve", "[curves][parm]")
{
    REQUIRE (sizeToRT60 (0.0f) == Catch::Approx (0.25f));
    REQUIRE (sizeToRT60 (1.0f) == Catch::Approx (6.0f));
    REQUIRE (sizeToRT60 (0.5f) == Catch::Approx (0.25f + 5.75f * std::pow (0.5f, 2.4f)));
}

TEST_CASE ("distnBlend uses pow 2.8 curve", "[curves][parm]")
{
    REQUIRE (distnBlend (0.0f) == Catch::Approx (0.0f));
    REQUIRE (distnBlend (1.0f) == Catch::Approx (1.0f));
    REQUIRE (distnBlend (0.25f) == Catch::Approx (std::pow (0.25f, 2.8f)));
}

TEST_CASE ("level equal-power at 0.5", "[curves][parm]")
{
    float dry {}, wet {};
    levelEqualPower (0.5f, dry, wet);
    REQUIRE (dry == Catch::Approx (wet).margin (1e-5f));
    REQUIRE (dry == Catch::Approx (std::sin (juce::MathConstants<float>::halfPi * 0.5f)).margin (1e-5f));
}

TEST_CASE ("inputGainDb smoothstep mapping", "[curves][parm]")
{
    REQUIRE (inputGainDb (0.0f) == Catch::Approx (9.0f));
    REQUIRE (inputGainDb (1.0f) == Catch::Approx (-3.0f));
}

TEST_CASE ("inputThresholdDb pow skew mapping", "[curves][parm]")
{
    REQUIRE (inputThresholdDb (0.0f) == Catch::Approx (-52.0f));
    REQUIRE (inputThresholdDb (1.0f) == Catch::Approx (-18.0f));
}

TEST_CASE ("sendGain Firm vs Soft differ at 0.5", "[curves][parm]")
{
    const auto firm = sendGain (0.5f, true);
    const auto soft = sendGain (0.5f, false);
    REQUIRE (firm != Catch::Approx (soft).margin (1e-6f));
    REQUIRE (firm == Catch::Approx (std::pow (smoothstep (0.5f), 1.85f)));
    REQUIRE (soft == Catch::Approx (std::pow (smoothstep (0.5f), 1.2f)));
}
