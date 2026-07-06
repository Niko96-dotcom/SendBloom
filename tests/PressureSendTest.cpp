#include <ParameterCurves.h>
#include <PressureSend.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

using namespace sendbloom::ParameterCurves;

TEST_CASE ("PressureSend returns unity gain when disconnected", "[send][PressureSend]")
{
    REQUIRE (sendbloom::PressureSend::computeGain (0.0f, false, true) == Catch::Approx (1.0f));
    REQUIRE (sendbloom::PressureSend::computeGain (0.5f, false, true) == Catch::Approx (1.0f));
    REQUIRE (sendbloom::PressureSend::computeGain (1.0f, false, false) == Catch::Approx (1.0f));
}

TEST_CASE ("PressureSend Firm curve matches ParameterCurves", "[send][PressureSend]")
{
    const auto gain = sendbloom::PressureSend::computeGain (0.5f, true, true);
    REQUIRE (gain == Catch::Approx (std::pow (smoothstep (0.5f), 1.85f)));
}

TEST_CASE ("PressureSend Soft curve matches ParameterCurves", "[send][PressureSend]")
{
    const auto gain = sendbloom::PressureSend::computeGain (0.5f, true, false);
    REQUIRE (gain == Catch::Approx (std::pow (smoothstep (0.5f), 1.2f)));
}

TEST_CASE ("PressureSend Firm and Soft differ at mid amount", "[send][PressureSend]")
{
    const auto firm = sendbloom::PressureSend::computeGain (0.5f, true, true);
    const auto soft = sendbloom::PressureSend::computeGain (0.5f, true, false);
    REQUIRE (firm != Catch::Approx (soft).margin (1e-6f));
}

TEST_CASE ("PressureSend process multiplies input by gain", "[send][PressureSend]")
{
    const auto input = 0.42f;
    const auto gain = 0.75f;
    REQUIRE (sendbloom::PressureSend::process (input, gain) == Catch::Approx (input * gain));
}
