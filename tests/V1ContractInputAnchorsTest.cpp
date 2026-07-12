#include <ParameterCurves.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE ("v1 Input anchors are -9 / 0 / +9 dB",
           "[v1][contract][input-anchors][CORE-01]")
{
    // CORE-01 / ADR-V1-08: inputGainDb(0/0.5/1) == −9 / 0 / +9 within 1e-4.
    // Current ParameterCurves maps +9…−3 (documented by ParameterCurvesTest — leave untouched).
    using sendbloom::ParameterCurves::inputGainDb;

    REQUIRE (inputGainDb (0.0f) == Catch::Approx (-9.0f).margin (1.0e-4f));
    REQUIRE (inputGainDb (0.5f) == Catch::Approx (0.0f).margin (1.0e-4f));
    REQUIRE (inputGainDb (1.0f) == Catch::Approx (9.0f).margin (1.0e-4f));
}
