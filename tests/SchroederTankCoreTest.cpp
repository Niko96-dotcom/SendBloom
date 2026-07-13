#include "ReverbTestHelpers.h"
#include <Authentic32Mode.h>
#include <HostRateReverbEngine.h>
#include <IReverbEngine.h>
#include <ParameterCurves.h>
#include <SchroederTank32.h>
#include <SchroederTank32DelayTable.h>
#include <SchroederTankCore.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using sendbloom::test::reverb::maxAbsDiff;
using sendbloom::test::reverb::measureRT60;
using sendbloom::test::reverb::renderCoreImpulse;

namespace
{

constexpr double kCoreRate = 32768.0;
constexpr double kHostRate = 48000.0;

std::vector<float> renderEngineImpulse (sendbloom::IReverbEngine& engine,
                                        float rt60,
                                        float darkMix,
                                        int numSamples)
{
    std::vector<float> out (static_cast<size_t> (numSamples), 0.0f);

    for (int i = 0; i < numSamples; ++i)
    {
        const auto in = i == 0 ? 1.0f : 0.0f;
        out[static_cast<size_t> (i)] = engine.processSample (in, rt60, darkMix);
    }

    return out;
}

std::vector<float> renderTankImpulse (sendbloom::SchroederTank32& tank,
                                      float rt60,
                                      float darkMix,
                                      int numSamples)
{
    std::vector<float> out (static_cast<size_t> (numSamples), 0.0f);

    for (int i = 0; i < numSamples; ++i)
    {
        const auto in = i == 0 ? 1.0f : 0.0f;
        out[static_cast<size_t> (i)] = tank.processSample (in, rt60, darkMix);
    }

    return out;
}

std::string readSourceFile (const char* relativePath)
{
    const auto path = std::string (SENDBLOOM_SOURCE_DIR) + "/" + relativePath;
    std::ifstream in (path);
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

TEST_CASE ("Shipping fixed-rate adapter updates reverb coefficients once per control span",
           "[verb][SchroederTankCore][performance][regression]")
{
    const auto coreSource = readSourceFile ("source/SchroederTankCore.h");
    const auto adapterSource = readSourceFile ("source/FixedRateAdapter.h");
    REQUIRE_FALSE (coreSource.empty());
    REQUIRE_FALSE (adapterSource.empty());

    const auto processStart = coreSource.find ("float processSample (float input)");
    const auto processEnd = coreSource.find ("void reset", processStart);
    REQUIRE (processStart != std::string::npos);
    REQUIRE (processEnd != std::string::npos);
    const auto processBody = coreSource.substr (processStart, processEnd - processStart);
    REQUIRE (processBody.find ("updateCoeffs") == std::string::npos);

    const auto adapterSet = adapterSource.find ("core.setParameters (rt60, darkMix)");
    const auto adapterLoop = adapterSource.find ("for (int i = 0; i < nInternal; ++i)");
    REQUIRE (adapterSet != std::string::npos);
    REQUIRE (adapterLoop != std::string::npos);
    REQUIRE (adapterSet < adapterLoop);
    REQUIRE (adapterSource.find ("core.processSample (", adapterLoop) != std::string::npos);
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

TEST_CASE ("fixed-rate SchroederTank32 differs from diagnostic host-rate engine", "[verb][HostRate][diagnostics]")
{
    sendbloom::HostRateReverbEngine engine;
    sendbloom::SchroederTank32 tank;
    engine.prepare (kHostRate, 512);
    tank.prepare (kHostRate, 512);

    const auto rt60 = sendbloom::ParameterCurves::sizeToRT60 (0.5f);
    constexpr int numSamples = 48000;

    const auto engineIr = renderEngineImpulse (engine, rt60, 0.0f, numSamples);
    const auto tankIr = renderTankImpulse (tank, rt60, 0.0f, numSamples);

    REQUIRE (maxAbsDiff (engineIr, tankIr) > 1.0e-4f);
}

TEST_CASE ("HostRateReverbEngine RT60 within tolerance at size 0.25", "[verb][HostRate][rt60]")
{
    sendbloom::HostRateReverbEngine engine;
    engine.prepare (kHostRate, 512);

    constexpr float sizeNorm = 0.25f;
    const auto target = sendbloom::ParameterCurves::sizeToRT60 (sizeNorm);
    const auto ir = renderEngineImpulse (engine, target, 0.0f,
                                         static_cast<int> (kHostRate * target * 3.0));
    const auto measured = measureRT60 (ir, kHostRate);

    REQUIRE (measured == Catch::Approx (target).margin (target * 0.15f));
}

TEST_CASE ("HostRateReverbEngine RT60 within tolerance at size 0.5", "[verb][HostRate][rt60]")
{
    sendbloom::HostRateReverbEngine engine;
    engine.prepare (kHostRate, 512);

    constexpr float sizeNorm = 0.5f;
    const auto target = sendbloom::ParameterCurves::sizeToRT60 (sizeNorm);
    const auto ir = renderEngineImpulse (engine, target, 0.0f,
                                         static_cast<int> (kHostRate * target * 3.0));
    const auto measured = measureRT60 (ir, kHostRate);

    REQUIRE (measured == Catch::Approx (target).margin (target * 0.15f));
}

TEST_CASE ("SchroederTankCore reset produces repeatable impulse response", "[verb][SchroederTankCore][reset]")
{
    sendbloom::SchroederTankCore core;
    core.prepare (kCoreRate, 512);

    constexpr float rt60 = 1.5f;
    constexpr int numSamples = static_cast<int> (kCoreRate * 0.5);

    const auto ir1 = renderCoreImpulse (core, rt60, 0.0f, numSamples);
    core.reset();
    const auto ir2 = renderCoreImpulse (core, rt60, 0.0f, numSamples);

    REQUIRE (maxAbsDiff (ir1, ir2) < 1.0e-5f);
}

TEST_CASE ("Authentic32Mode exposes diagnostics-only enum values", "[verb][Authentic32Mode]")
{
    using sendbloom::Authentic32Mode;

    REQUIRE (static_cast<uint8_t> (Authentic32Mode::Off) == 0);
    REQUIRE (static_cast<uint8_t> (Authentic32Mode::LegacyAccumulator) == 1);
    REQUIRE (static_cast<uint8_t> (Authentic32Mode::ProperSRC) == 2);

    const auto roundTrip = static_cast<Authentic32Mode> (static_cast<uint8_t> (Authentic32Mode::ProperSRC));
    REQUIRE (roundTrip == Authentic32Mode::ProperSRC);

    Authentic32Mode defaultMode {};
    REQUIRE (defaultMode == Authentic32Mode::Off);
}

TEST_CASE ("HostRateReverbEngine RT60 within tolerance at size 1.0", "[verb][HostRate][rt60]")
{
    sendbloom::HostRateReverbEngine engine;
    engine.prepare (kHostRate, 512);

    constexpr float sizeNorm = 1.0f;
    const auto target = sendbloom::ParameterCurves::sizeToRT60 (sizeNorm);
    const auto ir = renderEngineImpulse (engine, target, 0.0f,
                                         static_cast<int> (kHostRate * target * 2.5));
    const auto measured = measureRT60 (ir, kHostRate);

    REQUIRE (measured == Catch::Approx (target).margin (target * 0.15f));
}
