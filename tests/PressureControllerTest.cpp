#include <ParameterCurves.h>
#include <PressureController.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

namespace
{

constexpr double kSampleRate = 48000.0;

float settleGain (sendbloom::PressureController& c, int samples)
{
    float g = 0.0f;
    for (int i = 0; i < samples; ++i)
        g = c.processSample();
    return g;
}

} // namespace

TEST_CASE ("PressureController disconnected returns unity regardless of host pressure",
           "[send][PressureController]")
{
    sendbloom::PressureController c;
    c.prepare (kSampleRate);
    c.setConnected (false);
    c.setHostPressureTarget (0.0f);
    c.setMidiPressureTarget (0.0f);
    c.setFirmFeel (true);

    REQUIRE (c.processSample() == Catch::Approx (1.0f));

    c.setHostPressureTarget (1.0f);
    REQUIRE (c.processSample() == Catch::Approx (1.0f));
}

TEST_CASE ("PressureController connected at host 0 settles to zero gain",
           "[send][PressureController]")
{
    sendbloom::PressureController c;
    c.prepare (kSampleRate);
    c.setConnected (true);
    c.setHostPressureTarget (0.0f);
    c.setMidiPressureTarget (0.0f);
    c.setFirmFeel (true);

    // Seed nonzero state then release to 0 so we prove settle, not initial zero.
    c.setHostPressureTarget (1.0f);
    settleGain (c, static_cast<int> (kSampleRate * 0.2));
    c.setHostPressureTarget (0.0f);
    const auto gain = settleGain (c, static_cast<int> (kSampleRate * 0.5));
    REQUIRE (gain == Catch::Approx (0.0f).margin (1e-4f));
}

TEST_CASE ("PressureController connected settled gain matches ParameterCurves Firm/Soft",
           "[send][PressureController]")
{
    sendbloom::PressureController firm;
    firm.prepare (kSampleRate);
    firm.setConnected (true);
    firm.setMidiPressureTarget (0.0f);
    firm.setFirmFeel (true);
    firm.setHostPressureTarget (0.5f);
    const auto firmGain = settleGain (firm, static_cast<int> (kSampleRate * 0.5));
    REQUIRE (firmGain
             == Catch::Approx (sendbloom::ParameterCurves::sendGain (0.5f, true)).margin (1e-4f));

    sendbloom::PressureController soft;
    soft.prepare (kSampleRate);
    soft.setConnected (true);
    soft.setMidiPressureTarget (0.0f);
    soft.setFirmFeel (false);
    soft.setHostPressureTarget (0.5f);
    const auto softGain = settleGain (soft, static_cast<int> (kSampleRate * 0.5));
    REQUIRE (softGain
             == Catch::Approx (sendbloom::ParameterCurves::sendGain (0.5f, false)).margin (1e-4f));

    REQUIRE (firmGain != Catch::Approx (softGain).margin (1e-6f));
}

TEST_CASE ("PressureController attack reaches ~63% of settled Firm gain near 3 ms",
           "[send][PressureController]")
{
    sendbloom::PressureController c;
    c.prepare (kSampleRate);
    c.reset();
    c.setConnected (true);
    c.setMidiPressureTarget (0.0f);
    c.setFirmFeel (true);
    c.setHostPressureTarget (0.0f);
    settleGain (c, 64);

    c.setHostPressureTarget (1.0f);
    const auto settled = sendbloom::ParameterCurves::sendGain (1.0f, true);
    const int attackSamples = static_cast<int> (std::lround (0.003 * kSampleRate));
    const auto gainAtTau = settleGain (c, attackSamples);

    // Smooth raw pressure to ~63% at τ, then curve — expect post-curve gain at that pressure.
    const auto expected = sendbloom::ParameterCurves::sendGain (1.0f - std::exp (-1.0f), true);
    REQUIRE (gainAtTau == Catch::Approx (expected).margin (expected * 0.25f));
    REQUIRE (gainAtTau < settled);
}

TEST_CASE ("PressureController release reaches ~37% residual near 25 ms",
           "[send][PressureController]")
{
    sendbloom::PressureController c;
    c.prepare (kSampleRate);
    c.setConnected (true);
    c.setMidiPressureTarget (0.0f);
    c.setFirmFeel (true);
    c.setHostPressureTarget (1.0f);
    settleGain (c, static_cast<int> (kSampleRate * 0.5));

    c.setHostPressureTarget (0.0f);
    const int releaseSamples = static_cast<int> (std::lround (0.025 * kSampleRate));
    const auto gainAtTau = settleGain (c, releaseSamples);

    const auto expected = sendbloom::ParameterCurves::sendGain (std::exp (-1.0f), true);
    REQUIRE (gainAtTau == Catch::Approx (expected).margin (std::max (expected * 0.20f, 1e-4f)));
}

TEST_CASE ("PressureController clamps host and midi targets to [0,1]",
           "[send][PressureController]")
{
    sendbloom::PressureController c;
    c.prepare (kSampleRate);
    c.setConnected (true);
    c.setFirmFeel (true);
    c.setHostPressureTarget (2.0f);
    c.setMidiPressureTarget (-1.0f);
    const auto gain = settleGain (c, static_cast<int> (kSampleRate * 0.5));
    REQUIRE (gain == Catch::Approx (sendbloom::ParameterCurves::sendGain (1.0f, true)).margin (1e-4f));
}
