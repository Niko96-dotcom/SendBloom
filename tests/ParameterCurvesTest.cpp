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

TEST_CASE ("level equal-power wet-only at 0.5", "[curves][parm]")
{
    float dry {}, wet {};
    levelEqualPower (0.5f, dry, wet);
    REQUIRE (dry == Catch::Approx (1.0f).margin (1e-5f));
    REQUIRE (wet == Catch::Approx (std::sin (juce::MathConstants<float>::halfPi * 0.5f)).margin (1e-5f));
}

TEST_CASE ("inputGainDb ADR-V1-08 anchors", "[curves][parm]")
{
    REQUIRE (inputGainDb (0.0f) == Catch::Approx (-9.0f).margin (1e-4f));
    REQUIRE (inputGainDb (0.5f) == Catch::Approx (0.0f).margin (1e-4f));
    REQUIRE (inputGainDb (1.0f) == Catch::Approx (9.0f).margin (1e-4f));
}

TEST_CASE ("inputThresholdDb is a small gate trim around the reference", "[curves][parm]")
{
    // Demoted from an independent -52..-18 dB threshold to a +/-6 dB trim so that
    // INPT is the dominant gate-sensitivity control (see kGateReferenceDb).
    REQUIRE (inputThresholdDb (0.5f) == Catch::Approx (-45.0f)); // centre = reference
    REQUIRE (inputThresholdDb (0.0f) == Catch::Approx (-51.0f)); // -6 dB trim
    REQUIRE (inputThresholdDb (1.0f) == Catch::Approx (-39.0f)); // +6 dB trim
}

TEST_CASE ("sendGain Firm vs Soft differ at 0.5", "[curves][parm]")
{
    const auto firm = sendGain (0.5f, true);
    const auto soft = sendGain (0.5f, false);
    REQUIRE (firm != Catch::Approx (soft).margin (1e-6f));
    REQUIRE (firm == Catch::Approx (std::pow (smoothstep (0.5f), 1.85f)));
    REQUIRE (soft == Catch::Approx (std::pow (smoothstep (0.5f), 1.2f)));
}
