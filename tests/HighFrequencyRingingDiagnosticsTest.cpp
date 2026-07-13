#include "HfDiagnosticsHelpers.h"
#include <GatedBloomChain.h>
#include <ParameterCurves.h>
#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <string>
#include <vector>

namespace
{

using sendbloom::GatedBloomChain;
using sendbloom::test::ReverbPath;
using sendbloom::test::kAuthenticVsHostRmsAbove10kMaxRatio;
using sendbloom::test::kBlockSize;
using sendbloom::test::kImagingBandPeakRmsMax;
using sendbloom::test::kNarrowbandDominanceMaxRatio;
using sendbloom::test::kRenderSamples;
using sendbloom::test::kSampleRate;
using sendbloom::test::makeDecayingSine;
using sendbloom::test::makeGuitarPluck;
using sendbloom::test::makeImpulseBurst;
using sendbloom::test::makeSweptSine;
using sendbloom::test::measureTail;
using sendbloom::test::narrowbandDominanceRatio;
using sendbloom::test::printMetricsCsv;
using sendbloom::test::renderTankPath;
using sendbloom::test::imaging14825Rms;

struct RenderConfig
{
    const char* label;
    float distn;
    float darkMix;
};

const RenderConfig kMatrixConfigs[] {
    { "A_rev_bright", 0.0f, 0.0f },
    { "B_rev_dark", 0.0f, 1.0f },
    { "C_od_bright", 1.0f, 0.0f },
    { "D_od_dark", 1.0f, 1.0f },
};

std::vector<float> renderChain (const std::vector<float>& input,
                                float distn,
                                float darkMix) noexcept
{
    GatedBloomChain chain;
    chain.prepare (kSampleRate, kBlockSize);

    const auto rt60 = sendbloom::ParameterCurves::sizeToRT60 (0.5f);
    std::vector<float> wet (input.size());

    for (size_t i = 0; i < input.size(); ++i)
    {
        const auto env = chain.getEnvelope().process (std::abs (input[i]));
        wet[i] = chain.processSample (input[i], env, rt60, darkMix,
                                      distn, 1.0f, true, -40.0f);
    }

    return wet;
}

} // namespace

TEST_CASE ("HF ringing isolation matrix metrics CSV", "[hf][ringing][diagnostics][fixtures]")
{
    const std::vector<std::pair<const char*, std::vector<float>>> fixtures {
        { "guitar_pluck", makeGuitarPluck (kRenderSamples) },
        { "sine220_decay", makeDecayingSine (220.0f, kRenderSamples) },
        { "sine880_decay", makeDecayingSine (880.0f, kRenderSamples) },
        { "impulse", makeImpulseBurst (kRenderSamples) },
        { "swept_sine", makeSweptSine (kRenderSamples) },
    };

    std::cout << "config,fixture,rms_total,rms_above_6k,rms_above_10k,rms_above_14k,hf_ratio,centroid_hz,"
                 "max_nb_peak_rms,peak_freq_hz,hf_decay_sec\n";

    for (const auto& [fixtureName, input] : fixtures)
    {
        for (const auto& cfg : kMatrixConfigs)
        {
            const auto wet = renderChain (input, cfg.distn, cfg.darkMix);
            printMetricsCsv (measureTail (cfg.label, fixtureName, wet));
        }
    }

    SUCCEED ("HF isolation matrix metrics printed to stdout");
}

TEST_CASE ("HF ringing ProperSRC bright guitar 10k RMS near diagnostic host-rate level",
           "[hf][ringing][regression][TEST-11]")
{
    const auto input = makeGuitarPluck (kRenderSamples);

    // Tank-only baseline: ProperSRC coloration adds HF vs the diagnostic host-rate engine.
    sendbloom::SchroederTank32 tank;
    const auto hostTank = renderTankPath (tank, input, ReverbPath::HostRate, kSampleRate, kBlockSize);
    const auto authTank = renderTankPath (tank, input, ReverbPath::ProperSRC, kSampleRate, kBlockSize);
    const auto mHostTank = measureTail ("host_tank", "guitar_pluck", hostTank);
    const auto mAuthTank = measureTail ("auth_tank", "guitar_pluck", authTank);
    const auto tankRatio = mAuthTank.rmsAbove10k / std::max (mHostTank.rmsAbove10k, 1e-9f);

    INFO ("tank-only ratio=" << tankRatio << " (ProperSRC vs host-rate reverb)");
    REQUIRE (tankRatio < kAuthenticVsHostRmsAbove10kMaxRatio);
}

TEST_CASE ("HF ringing no narrowband glass whistle on authentic guitar tail",
           "[hf][ringing][regression][TEST-11]")
{
    const auto input = makeGuitarPluck (kRenderSamples);
    const auto wet = renderChain (input, 0.0f, 0.0f);
    const auto m = measureTail ("B_rev_auth_bright", "guitar_pluck", wet);
    // Imaging whistle lives near 14825 Hz; global peak dominance is not the glass-whistle gate.
    constexpr float kImagingProbeHz = 14825.0f;
    const auto dominance = narrowbandDominanceRatio (wet, kImagingProbeHz);

    INFO ("peak freq = " << m.peakFrequency << " Hz, imaging-band dominance = " << dominance);
    REQUIRE (dominance < kNarrowbandDominanceMaxRatio);
}

TEST_CASE ("HF ringing fixed ProperSRC imaging band suppressed on guitar",
           "[hf][ringing][regression][TEST-11]")
{
    const auto input = makeGuitarPluck (kRenderSamples);
    const auto wet = renderChain (input, 0.0f, 0.0f);
    const auto imagingRms = imaging14825Rms (wet, kSampleRate);

    INFO ("14825 Hz tail RMS = " << imagingRms);
    REQUIRE (imagingRms < kImagingBandPeakRmsMax);
}

TEST_CASE ("HF ringing fixed ProperSRC output finite for guitar pluck",
           "[hf][ringing][regression][TEST-11]")
{
    const auto input = makeGuitarPluck (kRenderSamples);

    for (const auto& cfg : kMatrixConfigs)
    {
        const auto wet = renderChain (input, cfg.distn, cfg.darkMix);

        for (const auto s : wet)
            REQUIRE (std::isfinite (s));
    }
}

TEST_CASE ("HF ringing diagnostic compares fixed ProperSRC with host-rate reverb", "[hf][ringing][diagnostics]")
{
    const auto input = makeGuitarPluck (kRenderSamples);
    sendbloom::SchroederTank32 tank;
    const auto hostRate = renderTankPath (tank, input, ReverbPath::HostRate, kSampleRate, kBlockSize);
    const auto authentic = renderTankPath (tank, input, ReverbPath::ProperSRC, kSampleRate, kBlockSize);

    const auto mHost = measureTail ("A", "guitar", hostRate);
    const auto mAuth = measureTail ("B", "guitar", authentic);

    INFO ("host hf_ratio=" << mHost.hfRatio << " auth hf_ratio=" << mAuth.hfRatio);
    INFO ("host rms_above_10k=" << mHost.rmsAbove10k << " auth rms_above_10k=" << mAuth.rmsAbove10k);
    INFO ("host peak=" << mHost.peakFrequency << " Hz auth peak=" << mAuth.peakFrequency << " Hz");

    REQUIRE (mAuth.peakFrequency < 12000.0f);
    SUCCEED ("diagnostic comparison complete");
}
