#include <ParameterCurves.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

using namespace sendbloom::ParameterCurves;

TEST_CASE ("sizeToRT60 spans the ring tank's achievable decay range", "[curves][parm]")
{
    // Requested targets below roughly a second collapse toward the same ring
    // tail, so 1.2 s is the chosen useful floor. The public manual establishes
    // the 6 s maximum, not a hardware minimum.
    REQUIRE (sizeToRT60 (0.0f) == Catch::Approx (kMinRT60Seconds));
    REQUIRE (sizeToRT60 (1.0f) == Catch::Approx (kMaxRT60Seconds));
    REQUIRE (sizeToRT60 (0.5f)
             == Catch::Approx (std::sqrt (kMinRT60Seconds * kMaxRT60Seconds)).epsilon (1e-5));
}

TEST_CASE ("sizeToRT60 is monotonic and clamped", "[curves][parm]")
{
    auto previous = sizeToRT60 (0.0f);

    for (int i = 1; i <= 100; ++i)
    {
        const auto current = sizeToRT60 (static_cast<float> (i) / 100.0f);
        REQUIRE (current > previous);
        previous = current;
    }

    REQUIRE (sizeToRT60 (-0.5f) == Catch::Approx (kMinRT60Seconds));
    REQUIRE (sizeToRT60 (1.5f) == Catch::Approx (kMaxRT60Seconds));
}

TEST_CASE ("distnBlend reaches both extremes across the knob", "[curves][parm]")
{
    REQUIRE (distnBlend (0.0f) == Catch::Approx (0.0f));
    REQUIRE (distnBlend (1.0f) == Catch::Approx (1.0f));
    REQUIRE (distnBlend (0.25f) == Catch::Approx (std::pow (0.25f, 1.6f)));

    // The manual promises "all clean, or all distorted" over the sweep, so the
    // middle of the knob has to be audibly dirty. pow 2.8 put it at 0.14.
    REQUIRE (distnBlend (0.5f) > 0.3f);
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
