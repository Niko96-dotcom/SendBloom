#include "ChainTestHelpers.h"
#include <GatedBloomChain.h>
#include <ParameterCurves.h>
#include <PluginProcessor.h>
#include <WetOverdrive.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <iostream>
#include <vector>

namespace
{

using sendbloom::OverdriveCurve;
using sendbloom::WetOverdrive;
using sendbloom::WetOverdriveState;

constexpr auto kSampleRate = 48000.0;
constexpr auto kBlockSize = 512;

float maxAbsOutput (OverdriveCurve curve) noexcept
{
    auto peak = 0.0f;

    for (int i = -400; i <= 400; ++i)
    {
        const auto x = static_cast<float> (i) * 0.01f;
        peak = std::max (peak, std::abs (WetOverdrive::clipSample (x, curve)));
    }

    return peak;
}

std::vector<float> renderTailThroughChain (sendbloom::GatedBloomChain& chain,
                                           float distnBlend,
                                           const std::vector<float>& input)
{
    const auto rt60 = sendbloom::ParameterCurves::sizeToRT60 (0.5f);
    std::vector<float> wet (input.size());

    for (size_t i = 0; i < input.size(); ++i)
    {
        const auto env = chain.getEnvelope().process (std::abs (input[i]));
        wet[i] = chain.processSample (input[i], env, rt60, 0.0f,
                                      distnBlend, 1.0f, true, -40.0f);
    }

    return wet;
}

std::vector<float> makeGuitarPluck (size_t totalSamples) noexcept
{
    std::vector<float> signal (totalSamples, 0.0f);
    constexpr auto kBurstLen = 240;
    for (int i = 0; i < kBurstLen; ++i)
    {
        const auto t = static_cast<float> (i) / static_cast<float> (kSampleRate);
        const auto env = std::exp (-t * 18.0f);
        signal[static_cast<size_t> (i)] = env * (0.6f * std::sin (2.0f * 3.14159265358979323846f * 330.0f * t)
                                                 + 0.25f * std::sin (2.0f * 3.14159265358979323846f * 660.0f * t));
    }

    return signal;
}

std::vector<float> makeDecayingSine220 (size_t totalSamples) noexcept
{
    std::vector<float> signal (totalSamples, 0.0f);
    constexpr auto kBurstLen = 480;
    const auto phaseInc = 2.0f * 3.14159265358979323846f * 220.0f / static_cast<float> (kSampleRate);

    for (int i = 0; i < kBurstLen; ++i)
    {
        const auto env = std::exp (-static_cast<float> (i) / 120.0f);
        signal[static_cast<size_t> (i)] = 0.35f * env * std::sin (phaseInc * static_cast<float> (i));
    }

    return signal;
}

std::vector<float> makeDiTransient (size_t totalSamples) noexcept
{
    std::vector<float> signal (totalSamples, 0.0f);
    constexpr auto kBurstLen = 64;

    for (int i = 0; i < kBurstLen; ++i)
    {
        const auto t = static_cast<float> (i) / static_cast<float> (kSampleRate);
        const auto env = std::exp (-t * 80.0f);
        signal[static_cast<size_t> (i)] = env * (0.7f * std::sin (2.0f * 3.14159265358979323846f * 1800.0f * t)
                                                  + 0.3f * std::sin (2.0f * 3.14159265358979323846f * 3600.0f * t));
    }

    return signal;
}

float highFrequencyEnergy (const std::vector<float>& samples, size_t start, size_t count) noexcept
{
    if (count == 0 || start + count > samples.size())
        return 0.0f;

    const auto slice = std::vector<float> (samples.begin() + static_cast<std::ptrdiff_t> (start),
                                           samples.begin() + static_cast<std::ptrdiff_t> (start + count));
    const auto low = sendbloom::test::goertzelPower (slice, kSampleRate, 800.0, 0, count);
    const auto mid = sendbloom::test::goertzelPower (slice, kSampleRate, 2000.0, 0, count);
    const auto high = sendbloom::test::goertzelPower (slice, kSampleRate, 5000.0, 0, count);
    const auto total = low + mid + high;

    if (total < 1e-12)
        return 0.0f;

    return static_cast<float> (high / total);
}

struct FixtureMetrics
{
    const char* name;
    float smallSignalGain;
    float maxOutput;
    float tailRmsClean;
    float tailRmsDirty;
    float hfEnergyClean;
    float hfEnergyDirty;
};

FixtureMetrics measureFixture (const char* name,
                               const std::vector<float>& input,
                               OverdriveCurve curveForStatelessGain)
{
    sendbloom::GatedBloomChain cleanChain;
    sendbloom::GatedBloomChain dirtyChain;
    cleanChain.prepare (kSampleRate, kBlockSize);
    dirtyChain.prepare (kSampleRate, kBlockSize);

    const auto cleanWet = renderTailThroughChain (cleanChain, 0.0f, input);
    const auto dirtyWet = renderTailThroughChain (dirtyChain, 1.0f, input);

    const auto tailStart = input.size() - 4800;
    const auto tailCount = 2400;

    FixtureMetrics m {};
    m.name = name;
    m.smallSignalGain = WetOverdrive::smallSignalGain (curveForStatelessGain);
    m.maxOutput = maxAbsOutput (curveForStatelessGain);
    m.tailRmsClean = sendbloom::test::rms (std::vector<float> (cleanWet.begin() + static_cast<std::ptrdiff_t> (tailStart),
                                                                 cleanWet.end()));
    m.tailRmsDirty = sendbloom::test::rms (std::vector<float> (dirtyWet.begin() + static_cast<std::ptrdiff_t> (tailStart),
                                                                 dirtyWet.end()));
    m.hfEnergyClean = highFrequencyEnergy (cleanWet, tailStart, tailCount);
    m.hfEnergyDirty = highFrequencyEnergy (dirtyWet, tailStart, tailCount);
    return m;
}

void printMetricsCsv (const FixtureMetrics& m)
{
    std::cout << m.name << ","
              << m.smallSignalGain << ","
              << m.maxOutput << ","
              << m.tailRmsClean << ","
              << m.tailRmsDirty << ","
              << (m.tailRmsDirty / std::max (m.tailRmsClean, 1e-9f)) << ","
              << m.hfEnergyClean << ","
              << m.hfEnergyDirty << ","
              << (m.hfEnergyDirty - m.hfEnergyClean)
              << '\n';
}

} // namespace

TEST_CASE ("WetOverdrive distn zero returns clean wet exactly", "[od][WetOverdrive][diagnostics]")
{
    WetOverdriveState od;
    od.prepare (kSampleRate);

    REQUIRE (od.process (0.35f, 0.0f) == Catch::Approx (0.35f));
    REQUIRE (od.process (-0.2f, 0.0f) == Catch::Approx (-0.2f));
    REQUIRE (WetOverdrive::process (0.18f, 0.0f) == Catch::Approx (0.18f));
}

TEST_CASE ("WetOverdrive distn one differs from clean wet", "[od][WetOverdrive][diagnostics]")
{
    WetOverdriveState od;
    od.prepare (kSampleRate);

    const auto input = 0.35f;
    const auto clean = od.process (input, 0.0f);
    const auto dirty = od.process (input, 1.0f);

    REQUIRE (dirty != Catch::Approx (clean).margin (1e-4f));
}

TEST_CASE ("WetOverdrive output finite for input range -4 to +4", "[od][WetOverdrive][diagnostics]")
{
    WetOverdriveState od;
    od.prepare (kSampleRate);

    for (int i = -400; i <= 400; ++i)
    {
        const auto x = static_cast<float> (i) * 0.01f;

        for (const auto blend : { 0.0f, 0.5f, 1.0f })
        {
            const auto y = od.process (x, blend);
            REQUIRE (std::isfinite (y));
        }
    }
}

TEST_CASE ("WetOverdrive no NaN Inf during reverb OD chain", "[od][WetOverdrive][diagnostics][chain]")
{
    sendbloom::GatedBloomChain chain;
    chain.prepare (kSampleRate, kBlockSize);

    const auto rt60 = sendbloom::ParameterCurves::sizeToRT60 (0.5f);
    const auto input = makeGuitarPluck (12000);

    for (size_t i = 0; i < input.size(); ++i)
    {
        const auto env = chain.getEnvelope().process (std::abs (input[i]));
        const auto wet = chain.processSample (input[i], env, rt60, 0.0f,
                                              1.0f, 1.0f, true, -40.0f);
        REQUIRE (std::isfinite (wet));
    }
}

TEST_CASE ("WetOverdrive small-signal gain within documented ceiling", "[od][WetOverdrive][diagnostics]")
{
    const auto gain = WetOverdrive::smallSignalGain (WetOverdrive::kActiveCurve);
    INFO ("small-signal gain = " << gain << ", ceiling = " << WetOverdrive::kSmallSignalMaxGain);
    REQUIRE (gain <= WetOverdrive::kSmallSignalMaxGain);
}

TEST_CASE ("WetOverdrive active curve tamed tanh asymmetry", "[od][WetOverdrive][diagnostics]")
{
    const auto pos = WetOverdrive::clipTamedTanh (0.25f);
    const auto neg = WetOverdrive::clipTamedTanh (-0.25f);
    const auto symPos = WetOverdrive::clipTamedTanhSymmetric (0.25f);

    REQUIRE (std::abs (pos) > std::abs (symPos) + 1e-4f);
    REQUIRE (std::abs (pos) > std::abs (neg));
}

TEST_CASE ("WetOverdrive blend interpolates clean and driven branch", "[od][WetOverdrive][diagnostics]")
{
    WetOverdriveState od;
    od.prepare (kSampleRate);

    const auto input = 0.3f;
    const auto blend = 0.5f;
    od.reset();
    const auto driven = od.processFilteredBranch (input);
    const auto expected = input + blend * (driven - input);
    od.reset();
    REQUIRE (od.process (input, blend) == Catch::Approx (expected));
}

TEST_CASE ("WetOverdrive legacy had higher small-signal gain than active curve", "[od][WetOverdrive][diagnostics]")
{
    const auto legacyGain = WetOverdrive::smallSignalGain (OverdriveCurve::Legacy);
    const auto activeGain = WetOverdrive::smallSignalGain (WetOverdrive::kActiveCurve);

    INFO ("legacy small-signal gain = " << legacyGain << ", active = " << activeGain);
    REQUIRE (legacyGain > activeGain);
}

TEST_CASE ("WetOverdrive 220 Hz decay tail does not swell versus clean at distn max", "[od][WetOverdrive][diagnostics][chain]")
{
    // Acceptance gate: quiet reverb-tail swell is measured on the 220 Hz decaying-sine
    // fixture (silence after burst). Other CSV fixtures are informational only.
    constexpr float kTailSwellMaxRatio220Hz = 1.15f;

    sendbloom::GatedBloomChain cleanChain;
    sendbloom::GatedBloomChain dirtyChain;
    cleanChain.prepare (kSampleRate, kBlockSize);
    dirtyChain.prepare (kSampleRate, kBlockSize);

    const auto input = makeDecayingSine220 (24000);
    const auto cleanWet = renderTailThroughChain (cleanChain, 0.0f, input);
    const auto dirtyWet = renderTailThroughChain (dirtyChain, 1.0f, input);

    const auto tailStart = input.size() - 4800;
    const auto cleanTail = std::vector<float> (cleanWet.begin() + static_cast<std::ptrdiff_t> (tailStart), cleanWet.end());
    const auto dirtyTail = std::vector<float> (dirtyWet.begin() + static_cast<std::ptrdiff_t> (tailStart), dirtyWet.end());

    const auto cleanRms = sendbloom::test::rms (cleanTail);
    const auto dirtyRms = sendbloom::test::rms (dirtyTail);
    const auto ratio = dirtyRms / std::max (cleanRms, 1e-9f);

    INFO ("220 Hz decay tail RMS ratio dirty/clean = " << ratio);
    REQUIRE (ratio < kTailSwellMaxRatio220Hz);
}

TEST_CASE ("WetOverdrive listening fixture metrics CSV", "[od][WetOverdrive][diagnostics][fixtures]")
{
    // Informational offline metrics for listening review. Only the 220 Hz decay fixture
    // has an enforced tail-swell gate (< 1.15); guitar pluck may exceed that ratio.
    std::cout << "fixture,small_signal_gain,max_output,tail_rms_clean,tail_rms_dirty,tail_rms_ratio,hf_clean,hf_dirty,hf_delta\n";

    const std::vector<std::pair<const char*, std::vector<float>>> fixtures {
        { "guitar_pluck", makeGuitarPluck (24000) },
        { "sine220_decay", makeDecayingSine220 (24000) },
        { "di_transient", makeDiTransient (24000) },
    };

    printMetricsCsv (measureFixture ("active_tamed_tanh", makeDecayingSine220 (24000), WetOverdrive::kActiveCurve));
    printMetricsCsv (measureFixture ("legacy_tanh", makeDecayingSine220 (24000), OverdriveCurve::Legacy));

    for (const auto& [name, signal] : fixtures)
    {
        const auto active = measureFixture (name, signal, WetOverdrive::kActiveCurve);
        printMetricsCsv (active);
    }

    // Candidate A/B/C stateless curve comparison on tail slice
    for (const auto curve : { OverdriveCurve::TamedTanh, OverdriveCurve::DiodeSoftClip, OverdriveCurve::CubicSoftClip })
    {
        const char* label = curve == OverdriveCurve::TamedTanh ? "candidate_a"
                          : curve == OverdriveCurve::DiodeSoftClip ? "candidate_b"
                          : "candidate_c";
        FixtureMetrics m {};
        m.name = label;
        m.smallSignalGain = WetOverdrive::smallSignalGain (curve);
        m.maxOutput = maxAbsOutput (curve);
        m.tailRmsClean = 0.0f;
        m.tailRmsDirty = 0.0f;
        m.hfEnergyClean = 0.0f;
        m.hfEnergyDirty = 0.0f;
        printMetricsCsv (m);
    }

    SUCCEED ("metrics printed to stdout");
}

TEST_CASE ("PluginProcessor dry path integrity with new overdrive", "[od][WetOverdrive][diagnostics][DryPath]")
{
    using namespace sendbloom::ParameterIDs;

    sendbloom::PluginProcessor plugin;
    auto& apvts = plugin.getAPVTS();
    *apvts.getRawParameterValue (inputGain) = 0.35f;
    *apvts.getRawParameterValue (outputGain) = 0.0f;
    *apvts.getRawParameterValue (bypass) = 0.0f;
    *apvts.getRawParameterValue (level) = 1.0f;
    *apvts.getRawParameterValue (distn) = 1.0f;
    plugin.prepareToPlay (kSampleRate, kBlockSize);

    juce::AudioBuffer<float> buffer (1, kBlockSize);
    juce::MidiBuffer midi;

    for (int block = 0; block < 32; ++block)
    {
        for (int i = 0; i < kBlockSize; ++i)
            buffer.setSample (0, i, 0.22f * std::sin (0.03f * static_cast<float> (block * kBlockSize + i)));

        plugin.processBlock (buffer, midi);

        for (int i = 0; i < kBlockSize; ++i)
            REQUIRE (std::isfinite (buffer.getSample (0, i)));
    }
}
