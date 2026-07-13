// On-audio diagnostic: drive a real note-then-mute through the full plugin and
// print how fast the post gate chops the wet tail. Hidden ([.] prefix) so it
// stays out of the normal suite; run explicitly with:  ./Tests "[diag]"
#include <ParameterIDs.h>
#include <PluginProcessor.h>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <cstdio>
#include <vector>

namespace
{
constexpr double kSampleRate = 48000.0;
constexpr int kBlock = 512;

void fillSine (juce::AudioBuffer<float>& b, float amp, float phaseInc, float phase0)
{
    for (int ch = 0; ch < b.getNumChannels(); ++ch)
        for (int i = 0; i < b.getNumSamples(); ++i)
            b.setSample (ch, i, amp * std::sin (phase0 + phaseInc * static_cast<float> (i)));
}

float rms (const juce::AudioBuffer<float>& b, int ch, int start, int count)
{
    double s = 0.0;
    for (int i = start; i < start + count; ++i)
        s += static_cast<double> (b.getSample (ch, i)) * b.getSample (ch, i);
    return static_cast<float> (std::sqrt (s / std::max (1, count)));
}
} // namespace

TEST_CASE ("[diag] post gate close timing on real audio", "[.][diag]")
{
    using namespace sendbloom::ParameterIDs;

    sendbloom::PluginProcessor plugin;
    auto& apvts = plugin.getAPVTS();
    *apvts.getRawParameterValue (inputGain) = 0.8f;   // driven hard, like INPT near overload
    *apvts.getRawParameterValue (outputGain) = 0.0f;
    *apvts.getRawParameterValue (bypass) = 0.0f;
    *apvts.getRawParameterValue (level) = 0.75f;
    *apvts.getRawParameterValue (distn) = 0.3f;
    *apvts.getRawParameterValue (size) = 0.5f;         // ~1.3 s tail: only the gate can cut it fast
    *apvts.getRawParameterValue (sendConnected) = 1.0f;
    *apvts.getRawParameterValue (sendAmount) = 1.0f;
    *apvts.getRawParameterValue (gatePrePost) = 1.0f;  // post gate (gated-reverb mode)
    *apvts.getRawParameterValue (inputThreshold) = 0.5f; // 0 dB trim -> -45 dB reference
    plugin.prepareToPlay (kSampleRate, kBlock);

    const float phaseInc = 2.0f * juce::MathConstants<float>::pi * 220.0f / (float) kSampleRate;
    juce::MidiBuffer midi;
    float phase = 0.0f;

    // ---- Sustained note: open the gate and fill the reverb ----
    juce::AudioBuffer<float> buf (2, kBlock);
    float preRms = 0.0f;
    for (int blk = 0; blk < 60; ++blk)
    {
        fillSine (buf, 0.5f, phaseInc, phase);
        phase += phaseInc * (float) kBlock;
        plugin.processBlock (buf, midi);
        preRms = rms (buf, 0, 0, kBlock);
    }

    // ---- Hard mute: input goes silent; capture the wet tail per sample ----
    std::vector<float> tail;
    tail.reserve (kBlock * 4);
    for (int blk = 0; blk < 4; ++blk)
    {
        buf.clear();
        plugin.processBlock (buf, midi);
        for (int i = 0; i < kBlock; ++i)
            tail.push_back (std::abs (buf.getSample (0, i)));
    }

    // Perceived close = first sample the wet tail drops below 2% of the pre-mute
    // level and stays there (dry is silent during the mute, so output == gated wet).
    const float target = 0.02f * preRms;
    int closeSample = -1;
    for (int i = 0; i + 48 < (int) tail.size(); ++i)
    {
        bool stays = true;
        for (int j = i; j < i + 48; ++j)
            if (tail[(size_t) j] >= target) { stays = false; break; }
        if (stays) { closeSample = i; break; }
    }

    std::printf ("\n=== post gate close timing (fs=%.0f, size~1.3s tail) ===\n", kSampleRate);
    std::printf ("pre-mute wet RMS: %.5f   (2%% target: %.6f)\n", preRms, target);
    std::printf ("wet tail after mute, per 1 ms window:\n");
    for (int ms = 0; ms < 20; ++ms)
    {
        const int start = (int) (ms * kSampleRate / 1000.0);
        if (start + 48 > (int) tail.size()) break;
        float peak = 0.0f;
        for (int j = start; j < start + 48; ++j) peak = std::max (peak, tail[(size_t) j]);
        const float pct = preRms > 0.0f ? 100.0f * peak / preRms : 0.0f;
        std::printf ("  %2d ms | %6.2f%% %s\n", ms, pct,
                     std::string ((size_t) std::min (40.0f, pct * 0.6f), '#').c_str());
    }
    if (closeSample >= 0)
        std::printf (">> gate cut the wet tail at %d samples = %.2f ms after mute\n\n",
                     closeSample, 1000.0 * closeSample / kSampleRate);
    else
        std::printf (">> wet tail did NOT fall below 2%% within %.1f ms\n\n",
                     1000.0 * tail.size() / kSampleRate);

    REQUIRE (closeSample >= 0);
}

TEST_CASE ("[diag] sustained low E does not chatter the gate", "[.][diag]")
{
    using namespace sendbloom::ParameterIDs;

    // Low E (82.4 Hz), 12.1 ms period. The worst case for chatter is an UNCLIPPED
    // note (low INPT): its envelope ripples fully between peaks, so if the hold is
    // shorter than the below-threshold window the gate stutters mid-note. Test hot
    // (soft-clipped, waveform flattened) and clean (full ripple).
    for (float inGain : { 0.8f, 0.5f, 0.35f })
    {
        sendbloom::PluginProcessor plugin;
        auto& apvts = plugin.getAPVTS();
        *apvts.getRawParameterValue (inputGain) = inGain;
        *apvts.getRawParameterValue (level) = 0.75f;
        *apvts.getRawParameterValue (distn) = 0.0f;
        *apvts.getRawParameterValue (size) = 0.5f;
        *apvts.getRawParameterValue (sendConnected) = 1.0f;
        *apvts.getRawParameterValue (sendAmount) = 1.0f;
        *apvts.getRawParameterValue (gatePrePost) = 1.0f;
        *apvts.getRawParameterValue (inputThreshold) = 0.5f;
        plugin.prepareToPlay (kSampleRate, kBlock);

        const float phaseInc = 2.0f * juce::MathConstants<float>::pi * 82.41f / (float) kSampleRate;
        juce::MidiBuffer midi;
        juce::AudioBuffer<float> buf (2, kBlock);
        float phase = 0.0f;

        for (int blk = 0; blk < 30; ++blk) // warm up
        {
            fillSine (buf, 0.5f, phaseInc, phase);
            phase += phaseInc * (float) kBlock;
            plugin.processBlock (buf, midi);
        }

        int windows = 0, dropouts = 0;
        float ref = 0.0f;
        for (int blk = 0; blk < 20; ++blk)
        {
            fillSine (buf, 0.5f, phaseInc, phase);
            phase += phaseInc * (float) kBlock;
            plugin.processBlock (buf, midi);
            for (int start = 0; start + 48 <= kBlock; start += 48)
            {
                const float w = rms (buf, 0, start, 48);
                ref = std::max (ref, w);
                ++windows;
                if (w < 0.05f * ref && ref > 1e-3f) ++dropouts;
            }
        }

        std::printf ("=== sustained low E (82 Hz), inputGain=%.2f ===  windows: %d  dropouts: %d\n",
                     inGain, windows, dropouts);
        INFO ("inputGain=" << inGain);
        REQUIRE (dropouts == 0);
    }
    std::printf ("\n");
}
