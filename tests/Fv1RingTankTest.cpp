#include <Fv1RingTank.h>
#include <ParameterCurves.h>
#include <catch2/catch_test_macros.hpp>

TEST_CASE ("Shipping ring decay coefficient responds monotonically to Size and stays stable",
           "[verb][Fv1RingTank][regression]")
{
    sendbloom::Fv1RingTank tank;
    tank.prepare (32768.0, 512);
    float previous = 0.0f;
    for (const auto size : { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f })
    {
        tank.setParameters (sendbloom::ParameterCurves::sizeToRT60 (size), 0.0f);
        const auto coefficient = tank.getKrt();
        REQUIRE (coefficient > previous);
        REQUIRE (coefficient < 1.0f);
        previous = coefficient;
    }
}
