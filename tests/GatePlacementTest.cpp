// ADR-V1-11c — what each Gate switch position does on real audio.
//
// The unit tests in NoiseGateTest.cpp pin the single gate envelope. These pin
// the thing that actually differs between the two switch positions: where that
// gain is applied. Before this file the pre position had no on-audio coverage at
// all — the only "pre" test fed a silent input and then asserted the mixer's own
// formula, so it passed with the pre gate deleted.
#include "ChainTestHelpers.h"
#include <GatedBloomChain.h>
#include <ParameterCurves.h>
#include <ParameterIDs.h>
#include <PluginProcessor.h>
#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <vector>

namespace
{

constexpr double kSampleRate = 48000.0;
constexpr int kBlockSize = 512;
constexpr float kThresholdDb = -45.0f;

std::vector<float> sine (size_t numSamples, float hz, float amplitude)
{
    std::vector<float> v (numSamples);
    const auto inc = 2.0f * juce::MathConstants<float>::pi * hz / static_cast<float> (kSampleRate);

    for (size_t i = 0; i < numSamples; ++i)
        v[i] = amplitude * std::sin (inc * static_cast<float> (i));

    return v;
}

/** Mirrors PluginProcessor::processSpan: the detector keys off the wet-path
    signal, and the chain gets both. */
std::vector<float> renderWet (const std::vector<float>& in, bool gatePre, float thresholdDb)
{
    sendbloom::GatedBloomChain chain;
    chain.prepare (kSampleRate, kBlockSize);

    std::vector<float> out (in.size(), 0.0f);
    std::vector<float> mono (static_cast<size_t> (kBlockSize));
    std::vector<float> env (static_cast<size_t> (kBlockSize));
    const auto rt60 = sendbloom::ParameterCurves::sizeToRT60 (0.5f);

    for (size_t offset = 0; offset < in.size(); offset += static_cast<size_t> (kBlockSize))
    {
        const auto n = static_cast<int> (std::min (static_cast<size_t> (kBlockSize),
                                                   in.size() - offset));

        for (int i = 0; i < n; ++i)
        {
            mono[static_cast<size_t> (i)] = in[offset + static_cast<size_t> (i)];
            env[static_cast<size_t> (i)] =
                chain.getEnvelope().process (std::abs (mono[static_cast<size_t> (i)]));
        }

        chain.processBlock (mono.data(), env.data(), out.data() + offset, n,
                            rt60, 0.0f, 0.3f, 1.0f, gatePre, thresholdDb);
    }

    return out;
}

float rmsAt (const std::vector<float>& v, size_t from, size_t count)
{
    const auto end = std::min (v.size(), from + count);

    if (from >= end)
        return 0.0f;

    return sendbloom::test::rms (std::vector<float> (v.begin() + static_cast<long> (from),
                                                     v.begin() + static_cast<long> (end)));
}

} // namespace

TEST_CASE ("Pre placement keeps sub-threshold hum out of the wet path",
           "[gate][placement][CORE-13]")
{
    // The manual's stated reason the gate exists: taming cable hum reaching the
    // reverb and distortion stages when there is no signal.
    const auto hum = sine (static_cast<size_t> (kSampleRate * 3.0), 120.0f,
                           juce::Decibels::decibelsToGain (-50.0f));

    const auto gated = renderWet (hum, true, kThresholdDb);
    const auto ungated = renderWet (hum, true, -200.0f); // same chain, gate held open

    const auto window = static_cast<size_t> (kSampleRate * 0.5);
    const auto gatedRms = rmsAt (gated, gated.size() - window, window);
    const auto ungatedRms = rmsAt (ungated, ungated.size() - window, window);

    INFO ("gated " << gatedRms << " vs ungated " << ungatedRms);
    REQUIRE (ungatedRms > 1.0e-5f);          // the reference really does pass hum
    REQUIRE (gatedRms < ungatedRms * 0.01f); // >= 40 dB of rejection
}

TEST_CASE ("Pre placement leaves the tail alone while Post chops it",
           "[gate][placement][CORE-12][CORE-13]")
{
    auto sig = sine (static_cast<size_t> (kSampleRate * 2.0), 220.0f, 0.5f);
    sig.resize (static_cast<size_t> (kSampleRate * 3.0), 0.0f);

    const auto pre = renderWet (sig, true, kThresholdDb);
    const auto post = renderWet (sig, false, kThresholdDb);

    const auto muteAt = static_cast<size_t> (kSampleRate * 2.0);
    const auto window = static_cast<size_t> (kSampleRate * 0.05);
    const auto reference = rmsAt (pre, muteAt - window, window);
    REQUIRE (reference > 1.0e-3f);

    // Post: gone within the 15 ms budget, and it stays gone.
    REQUIRE (rmsAt (post, muteAt + static_cast<size_t> (kSampleRate * 0.015), window)
             < reference * 0.02f);
    REQUIRE (rmsAt (post, muteAt + static_cast<size_t> (kSampleRate * 0.5), window)
             < reference * 0.02f);

    // Pre: the gate stops feeding the tank, but the tank keeps ringing.
    REQUIRE (rmsAt (pre, muteAt + static_cast<size_t> (kSampleRate * 0.5), window)
             > reference * 0.02f);
}

TEST_CASE ("Pre placement stops hum entering the tank once the gate has closed",
           "[gate][placement][CORE-13]")
{
    // Regression guard for the profile removed in ADR-V1-11c: its 150 ms one-pole
    // release was only 5 dB down 100 ms into a gap and took 705 ms to reach
    // -40 dB, so hum kept feeding the tank — and reverberating — long after the
    // player stopped. Two renders differing only by hum during the gap; the
    // difference is exactly what the pre gate let through.
    const auto total = static_cast<size_t> (kSampleRate * 3.0);
    auto note = sine (total, 220.0f, 0.5f);
    std::fill (note.begin() + static_cast<long> (kSampleRate * 1.0), note.end(), 0.0f);

    const auto hum = sine (total, 120.0f, juce::Decibels::decibelsToGain (-50.0f));
    auto noteWithHum = note;

    for (auto i = static_cast<size_t> (kSampleRate * 1.1); i < total; ++i)
        noteWithHum[i] += hum[i];

    const auto clean = renderWet (note, true, kThresholdDb);
    const auto dirty = renderWet (noteWithHum, true, kThresholdDb);

    std::vector<float> delta (total);

    for (size_t i = 0; i < total; ++i)
        delta[i] = dirty[i] - clean[i];

    const auto from = static_cast<size_t> (kSampleRate * 1.5);
    REQUIRE (rmsAt (delta, from, total - from) < 1.0e-7f);
}

TEST_CASE ("Post placement leaves the tank running behind a closed gate",
           "[gate][placement][CORE-12]")
{
    // The reference DSP chip never stops reverberating; a post gate only mutes
    // its output. Re-triggering must therefore reveal the buried tail rather
    // than start a fresh one.
    auto sig = sine (static_cast<size_t> (kSampleRate * 1.0), 220.0f, 0.5f);
    sig.resize (static_cast<size_t> (kSampleRate * 2.0), 0.0f);
    const auto probe = sine (static_cast<size_t> (kSampleRate * 0.05), 220.0f, 0.05f);
    sig.insert (sig.end(), probe.begin(), probe.end());

    const auto wet = renderWet (sig, false, kThresholdDb);
    const auto reopenAt = static_cast<size_t> (kSampleRate * 2.0);

    const auto buried = rmsAt (wet, reopenAt - 480, 480);
    const auto revealed = rmsAt (wet, reopenAt + 240, 480);

    REQUIRE (buried < 1.0e-6f);          // fully muted before the re-trigger
    REQUIRE (revealed > buried + 0.01f); // and a real tail is there waiting
}

TEST_CASE ("Flipping the Gate switch over a live tail is click-bounded",
           "[gate][placement][switch][CORE-17]")
{
    using namespace sendbloom::ParameterIDs;

    // Before ADR-V1-11c the switch was an unsmoothed per-block bool: flipping it
    // over a decaying tail stepped the wet output by up to 0.44 in one sample as
    // the post node muted or unmuted a full-level tail.
    //
    // Baseline is an instance parked in Pre. Both placements gate at the same
    // node relative to the tank input, so during a note-then-silence run the tank
    // content is identical across instances — the Pre instance's output over the
    // flip block is exactly the tail the flipping instance ramps to or from.
    const auto configure = [] (sendbloom::PluginProcessor& p, float gateValue)
    {
        auto& apvts = p.getAPVTS();
        *apvts.getRawParameterValue (inputGain) = 0.5f;
        *apvts.getRawParameterValue (outputGain) = 0.0f;
        *apvts.getRawParameterValue (bypass) = 0.0f;
        *apvts.getRawParameterValue (level) = 1.0f;
        *apvts.getRawParameterValue (distn) = 0.3f;
        *apvts.getRawParameterValue (size) = 0.5f;
        *apvts.getRawParameterValue (inputThreshold) = 0.5f;
        *apvts.getRawParameterValue (gatePrePost) = gateValue;
        p.prepareToPlay (kSampleRate, kBlockSize);
    };

    sendbloom::PluginProcessor baseline, preToPost, postToPre;
    configure (baseline, 0.0f);
    configure (preToPost, 0.0f);
    configure (postToPre, 1.0f);

    juce::AudioBuffer<float> bufBaseline (2, kBlockSize), bufPreToPost (2, kBlockSize),
        bufPostToPre (2, kBlockSize);
    juce::MidiBuffer midi;
    const auto inc = 2.0f * juce::MathConstants<float>::pi * 220.0f / static_cast<float> (kSampleRate);
    float phase = 0.0f;

    const auto runBlock = [&] (bool silent)
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            for (int i = 0; i < kBlockSize; ++i)
            {
                const auto s = silent ? 0.0f
                                      : 0.5f * std::sin (phase + inc * static_cast<float> (i));
                bufBaseline.setSample (ch, i, s);
                bufPreToPost.setSample (ch, i, s);
                bufPostToPre.setSample (ch, i, s);
            }
        }

        phase += inc * static_cast<float> (kBlockSize);
        baseline.processBlock (bufBaseline, midi);
        preToPost.processBlock (bufPreToPost, midi);
        postToPre.processBlock (bufPostToPre, midi);
    };

    for (int b = 0; b < 100; ++b) // sustained note fills the tank
        runBlock (false);

    for (int b = 0; b < 20; ++b) // release: gate closes, tail is live
        runBlock (true);

    const auto maxStep = [] (const juce::AudioBuffer<float>& b, float previous)
    {
        auto largest = std::abs (b.getSample (0, 0) - previous);

        for (int i = 1; i < b.getNumSamples(); ++i)
            largest = std::max (largest, std::abs (b.getSample (0, i) - b.getSample (0, i - 1)));

        return largest;
    };

    const auto lastBaseline = bufBaseline.getSample (0, kBlockSize - 1);
    const auto lastPreToPost = bufPreToPost.getSample (0, kBlockSize - 1);
    const auto lastPostToPre = bufPostToPre.getSample (0, kBlockSize - 1);

    *preToPost.getAPVTS().getRawParameterValue (gatePrePost) = 1.0f;
    *postToPre.getAPVTS().getRawParameterValue (gatePrePost) = 0.0f;
    runBlock (true);

    const auto baselineStep = maxStep (bufBaseline, lastBaseline);
    const auto preToPostStep = maxStep (bufPreToPost, lastPreToPost);
    const auto postToPreStep = maxStep (bufPostToPre, lastPostToPre);

    INFO ("baseline " << baselineStep << " pre->post " << preToPostStep
                      << " post->pre " << postToPreStep);

    // The tail is live, so there is something real to click on.
    REQUIRE (bufBaseline.getMagnitude (0, 0, kBlockSize) > 0.05f);

    // A 5 ms ramp adds at most peak/240 per sample on top of the tail's own slew.
    const auto allowance = baselineStep + bufBaseline.getMagnitude (0, 0, kBlockSize) / 100.0f;
    REQUIRE (preToPostStep <= allowance);
    REQUIRE (postToPreStep <= allowance);

    // And the flip must actually have taken effect, or the bound above is vacuous.
    REQUIRE (bufPostToPre.getMagnitude (0, 0, kBlockSize) > 0.05f);
}
