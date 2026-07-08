#include "ReverbTestHelpers.h"
#include <ParameterCurves.h>
#include <SchroederTank32DelayTable.h>
#include <SchroederTankCore.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using sendbloom::test::reverb::measureRT60;
using sendbloom::test::reverb::renderCoreImpulse;

namespace
{

constexpr double kCoreRate = 32768.0;

std::string readSourceFile (const char* relativePath)
{
    std::ifstream in (relativePath);
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

} // namespace

TEST_CASE ("SchroederTankCore RT60 within tolerance at size 0.25", "[verb][SchroederTankCore][rt60]")
{
    sendbloom::SchroederTankCore core;
    core.prepare (kCoreRate, 512);

    constexpr float sizeNorm = 0.25f;
    const auto target = sendbloom::ParameterCurves::sizeToRT60 (sizeNorm);
    const auto ir = renderCoreImpulse (core, target, 0.0f, static_cast<int> (kCoreRate * target * 3.0));
    const auto measured = measureRT60 (ir, kCoreRate);

    REQUIRE (measured == Catch::Approx (target).margin (target * 0.15f));
}

TEST_CASE ("SchroederTankCore RT60 within tolerance at size 0.5", "[verb][SchroederTankCore][rt60]")
{
    sendbloom::SchroederTankCore core;
    core.prepare (kCoreRate, 512);

    constexpr float sizeNorm = 0.5f;
    const auto target = sendbloom::ParameterCurves::sizeToRT60 (sizeNorm);
    const auto ir = renderCoreImpulse (core, target, 0.0f, static_cast<int> (kCoreRate * target * 3.0));
    const auto measured = measureRT60 (ir, kCoreRate);

    REQUIRE (measured == Catch::Approx (target).margin (target * 0.15f));
}

TEST_CASE ("SchroederTankCore RT60 within tolerance at size 1.0", "[verb][SchroederTankCore][rt60]")
{
    sendbloom::SchroederTankCore core;
    core.prepare (kCoreRate, 512);

    constexpr float sizeNorm = 1.0f;
    const auto target = sendbloom::ParameterCurves::sizeToRT60 (sizeNorm);
    const auto ir = renderCoreImpulse (core, target, 0.0f, static_cast<int> (kCoreRate * target * 2.5));
    const auto measured = measureRT60 (ir, kCoreRate);

    REQUIRE (measured == Catch::Approx (target).margin (target * 0.15f));
}

TEST_CASE ("SchroederTankCore has no host-path mode identifiers", "[verb][SchroederTankCore][CORE-01]")
{
    const auto source = readSourceFile ("source/SchroederTankCore.h");

    REQUIRE (source.find ("hostRate") == std::string::npos);
    REQUIRE (source.find ("useAuthenticPath") == std::string::npos);
    REQUIRE (source.find ("authenticColor") == std::string::npos);
}

TEST_CASE ("SchroederTankCore uses unscaled delay table at 32768 Hz", "[verb][SchroederTankCore][CORE-02]")
{
    sendbloom::SchroederTankCore core;
    core.prepare (kCoreRate, 512);

    constexpr float sizeNorm = 0.5f;
    const auto target = sendbloom::ParameterCurves::sizeToRT60 (sizeNorm);
    const auto ir = renderCoreImpulse (core, target, 0.0f, static_cast<int> (kCoreRate * target * 3.0));
    const auto measured = measureRT60 (ir, kCoreRate);

    // At 32768 Hz scale factor is 1.0 — RT60 within tolerance confirms unscaled table behavior.
    REQUIRE (measured == Catch::Approx (target).margin (target * 0.15f));
    REQUIRE (sendbloom::SchroederTank32DelayTable::kSeriesApfDelays[0] == 167);
}
