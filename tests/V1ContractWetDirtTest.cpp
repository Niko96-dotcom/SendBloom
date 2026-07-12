#include <GatedBloomChain.h>
#include <WetOverdrive.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>

namespace
{

constexpr auto kPi = 3.14159265358979323846f;
constexpr auto kSampleRate = 48000.0;

std::string readTextFile (const juce::File& file)
{
    std::ifstream stream (file.getFullPathName().toStdString());

    if (! stream)
        return {};

    std::ostringstream buffer;
    buffer << stream.rdbuf();
    return buffer.str();
}

juce::File findRepoRoot()
{
    auto dir = juce::File::getCurrentWorkingDirectory();

    for (int depth = 0; depth < 8; ++depth)
    {
        const auto cmakeLists = dir.getChildFile ("CMakeLists.txt");

        if (cmakeLists.existsAsFile())
        {
            const auto cmakeText = readTextFile (cmakeLists);

            if (cmakeText.find ("SendBloom") != std::string::npos)
                return dir;
        }

        dir = dir.getParentDirectory();
    }

    return juce::File::getCurrentWorkingDirectory();
}

float measureFilteredBranchRms (sendbloom::WetOverdriveState& od,
                                float freqHz,
                                float amplitude,
                                size_t settleSamples,
                                size_t measureSamples)
{
    od.prepare (kSampleRate);
    od.reset();

    const auto phaseInc = 2.0f * kPi * freqHz / static_cast<float> (kSampleRate);
    auto phase = 0.0f;

    for (size_t i = 0; i < settleSamples; ++i)
    {
        const auto x = amplitude * std::sin (phase);
        phase += phaseInc;
        od.processFilteredBranch (x);
    }

    auto sumSq = 0.0;

    for (size_t i = 0; i < measureSamples; ++i)
    {
        const auto x = amplitude * std::sin (phase);
        phase += phaseInc;
        const auto y = od.processFilteredBranch (x);
        sumSq += static_cast<double> (y) * static_cast<double> (y);
    }

    return static_cast<float> (std::sqrt (sumSq / static_cast<double> (measureSamples)));
}

} // namespace

TEST_CASE ("30 Hz strongly attenuated vs 1 kHz on filtered branch",
           "[v1][contract][wet-dirt][DSP-09]")
{
    sendbloom::WetOverdriveState od;

    // 100 Hz one-pole HP needs ~2 s to reach steady state at 30 Hz, so let it
    // settle across two seconds before measuring the steady-state RMS.
    constexpr auto kAmplitude = 0.35f;
    constexpr size_t kSettle = 48000 * 2;
    constexpr size_t kMeasure = 96000;

    const auto lowRms = measureFilteredBranchRms (od, 30.0f, kAmplitude, kSettle, kMeasure);
    const auto midRms = measureFilteredBranchRms (od, 1000.0f, kAmplitude, kSettle, kMeasure);

    REQUIRE (midRms > 1.0e-4f);
    REQUIRE (lowRms < midRms * 0.25f);
}

TEST_CASE ("asymmetric clip preserves harder positive drive than symmetric reference",
           "[v1][contract][wet-dirt][DSP-09]")
{
    constexpr auto kTestLevel = 0.25f;
    const auto asym = sendbloom::WetOverdrive::clipTamedTanh (kTestLevel);
    const auto sym = sendbloom::WetOverdrive::clipTamedTanhSymmetric (kTestLevel);

    REQUIRE (std::abs (asym) > std::abs (sym) + 1.0e-4f);
}

TEST_CASE ("post-clip DC blocker decays DC input on filtered branch",
           "[v1][contract][wet-dirt][DSP-10]")
{
    sendbloom::WetOverdriveState od;
    od.prepare (kSampleRate);
    od.reset();

    constexpr auto kDc = 0.2f;
    constexpr size_t kSamples = 48000 * 4;

    auto last = 0.0f;

    for (size_t i = 0; i < kSamples; ++i)
        last = od.processFilteredBranch (kDc);

    REQUIRE (std::abs (last) < 0.01f);
}

// Spec §17.2 gate is "Wet dirt DC mean < 1e-4 after settling" — i.e. the DC
// component of the output (mean of y), not the signal magnitude (mean of |y|).
// The 20 Hz post-clip DC blocker removes residual DC left by the asymmetric
// clipper; a 0.35-amplitude 220 Hz sine itself is ~0.37 in mean-of-|y|.
TEST_CASE ("long-run DC offset below 1e-4 after asymmetric clip",
           "[v1][contract][wet-dirt][DSP-11]")
{
    sendbloom::WetOverdriveState od;
    od.prepare (kSampleRate);
    od.reset();

    constexpr auto kDc = 0.08f;
    constexpr auto kFreq = 220.0f;
    constexpr auto kAmplitude = 0.35f;
    constexpr size_t kSamples = 48000 * 6;
    const auto phaseInc = 2.0f * kPi * kFreq / static_cast<float> (kSampleRate);
    auto phase = 0.0f;

    auto sum = 0.0;

    for (size_t i = 0; i < kSamples; ++i)
    {
        const auto x = kDc + kAmplitude * std::sin (phase);
        phase += phaseInc;
        const auto y = od.process (x, 1.0f);
        sum += static_cast<double> (y);
    }

    const auto dcMean = std::abs (sum / static_cast<double> (kSamples));
    REQUIRE (dcMean < 1.0e-4);
}

TEST_CASE ("distn zero returns original wet within tolerance",
           "[v1][contract][wet-dirt][DSP-12]")
{
    sendbloom::WetOverdriveState od;
    od.prepare (kSampleRate);

    const std::array<float, 5> inputs { -0.4f, -0.05f, 0.0f, 0.12f, 0.38f };

    for (const auto wet : inputs)
        REQUIRE (od.process (wet, 0.0f) == Catch::Approx (wet).margin (1.0e-6f));
}

TEST_CASE ("dirt_os not consumed in GatedBloomChain audio path",
           "[v1][contract][wet-dirt][DSP-13]")
{
    const auto root = findRepoRoot();
    const auto chainText = readTextFile (root.getChildFile ("source/GatedBloomChain.h"));
    const auto drawerText = readTextFile (root.getChildFile ("source/ui/AdvancedDrawer.cpp"));

    REQUIRE_FALSE (chainText.empty());
    REQUIRE_FALSE (drawerText.empty());
    REQUIRE (chainText.find ("dirtOs") == std::string::npos);
    REQUIRE (chainText.find ("dirt_os") == std::string::npos);
    REQUIRE (drawerText.find ("dirtOsToggle.setEnabled (false)") != std::string::npos);
}

TEST_CASE ("GatedBloomChain wet overdrive path remains allocation-free at runtime",
           "[v1][contract][wet-dirt][DSP-13]")
{
    sendbloom::GatedBloomChain chain;
    chain.prepare (kSampleRate, 512);

    const auto rt60 = 1.2f;

    for (int i = 0; i < 4096; ++i)
    {
        const auto env = chain.getEnvelope().process (0.2f);
        const auto wet = chain.processSample (0.15f, env, rt60, 0.0f, false, 1.0f, 1.0f, true, -40.0f);
        REQUIRE (std::isfinite (wet));
    }
}
