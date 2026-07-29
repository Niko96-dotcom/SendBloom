#pragma once

#include <array>
#include <cstddef>

namespace sendbloom
{

/** Delay lengths and coefficients for the allpass-ring ("Barr ring") tank.

    The reference hardware class is built on the Spin Semiconductor FV-1, whose
    published datasheet fixes three numbers that dominate how such a reverb can
    possibly sound:

      - Fs = 32,768 Hz (standard watch crystal on X1/X2).
      - 32,768 words of delay RAM — exactly 1.0 s of total delay, for everything.
      - Converter response is −3 dB at ~15 kHz.

    Spin's own design notes (Keith Barr, "Effects → Reverberation" and
    "Considerations when building a reverb") describe the topology those numbers
    imply, and state the rules this table is built to satisfy:

      - The loop is a ring of blocks, each "2 allpass filters and a delay",
        with a reverb-time coefficient applied once per block.
      - "The total delay (excluding allpass filter delays) in the loop should be
        at least 200ms. Shorter delay time will lead to flutter... very short
        delays will cause a tinny sound."   -> kRingDelays sum to ~497 ms.
      - Allpass coefficients "on the order of 0.6 will build quicker, but be
        'fat' during the initial sound period" (0.7+ tends to ring).
      - "add a few series allpass filters in the input signal path, so that the
        signal inserted into the loop has a higher initial density."
      - Ringing is smoothed by "slowly modulate some of the delay lengths within
        the reverb loop ... using say, the SIN output in one place and COS in
        another."

    Lengths below are our own mutually prime choices inside the ranges Spin's
    freely published reference programs occupy, not a copy of any program, and
    the whole structure is sized to fit the real 32,768-word RAM budget.
    See docs/fv1-reverb-architecture.md.
*/
struct Fv1RingTankTable
{
    static constexpr double kInternalRate = 32768.0;

    /** The FV-1's entire delay RAM, in words. The structure below must fit. */
    static constexpr int kDelayRamWords = 32768;

    // Input diffusion: four series allpasses ahead of the ring.
    static constexpr std::array<int, 4> kInputDiffuserDelays { 241, 443, 863, 1097 };

    // The ring: two serial allpasses then one delay per block, four blocks.
    // Asymmetric short+long per block (sums match prior singles 2311/2909/3167/2417).
    // Short AP first (unmodulated) densifies; long second (modulated) keeps
    // relative LFO depth near the old single-allpass HF behaviour.
    static constexpr std::array<int, 8> kRingAllpassDelays {
        588, 1723, 706, 2203, 756, 2411, 586, 1831
    };
    static constexpr std::array<int, 4> kRingDelays { 3623, 4597, 4391, 3671 };

    static constexpr float kInputDiffuserFeedback = 0.6f;
    static constexpr float kRingAllpassFeedback = 0.6f;

    /** Headroom scale on the way into the diffusers (Spin: "leave headroom"). */
    static constexpr float kInputScale = 0.25f;

    // Output is gathered as weighted taps from inside the ring delays, so the
    // return is a blend of four points in the decay rather than one.
    //
    // Block 0 carries only recirculated signal; the input is injected at blocks
    // 1-3. So the earliest the tank can speak is the tap on block 1, and its
    // offset alone decides whether Bright reads as "immediate" or as having a
    // built-in predelay. 211 samples is 6.4 ms — the tank answers the pick, then
    // the 27/37/54 ms taps bloom in behind it. Dark gets its delayed onset from
    // kDarkPredelaySeconds instead, which is the contrast the manual describes:
    // "bright and immediate, or dark with pre-delay".
    static constexpr std::array<int, 4> kTapOffsets { 1201, 211, 897, 1780 };
    static constexpr std::array<float, 4> kTapWeights { 0.6f, 0.8f, 0.7f, 0.5f };

    // Per-block shelving losses inside the ring. Low-frequency loss is what
    // keeps a 6 s tail from turning to mud; the old parallel-comb tank had none.
    static constexpr float kLfLossHz = 105.0f;
    static constexpr float kHfLossHzBright = 11500.0f;
    static constexpr float kHfLossHzDark = 2100.0f;
    static constexpr float kHfLossDepthBright = 0.45f;
    static constexpr float kHfLossDepthDark = 0.88f;
    static constexpr float kLfLossDepthBright = 0.35f;
    static constexpr float kLfLossDepthDark = 0.15f;

    /** Dark adds predelay ahead of the tank ("dark with pre-delay"). */
    static constexpr float kDarkPredelaySeconds = 0.055f;

    // Two slow LFOs, sin and cos each, smear the long (second) allpass of each block.
    static constexpr std::array<float, 2> kLfoHz { 0.48f, 0.60f };
    static constexpr std::array<float, 2> kLfoDepthSeconds {
        4.625f / 32768.0f, 4.125f / 32768.0f
    };

    /** In-loop damping removes energy the ideal krt solution does not model, so
        krt is raised by this exponent to recentre measured T30 on the target. */
    static constexpr float kKrtCompensation = 1.0648f;

    /** Hard ceiling on loop gain — the ring must never be able to self-oscillate. */
    static constexpr float kKrtMax = 0.92f;

    /** Matches the shipped wet return level of the previous tank so the Level
        curve, factory presets and clip LED thresholds carry over unchanged.
        Calibrated on impulse-response RMS at RT60 3 s, both tanks, via
        tools/RenderTank — the ring is intrinsically ~23 dB hotter because its
        loop holds far more energy. */
    static constexpr float kOutputNormalisation = 2.48f;

    static constexpr int sumOf (const std::array<int, 4>& a) noexcept
    {
        return a[0] + a[1] + a[2] + a[3];
    }

    static constexpr int sumOf (const std::array<int, 8>& a) noexcept
    {
        return a[0] + a[1] + a[2] + a[3] + a[4] + a[5] + a[6] + a[7];
    }

    /** Plain delay in the loop — Barr's ">= 200 ms or it flutters" budget. */
    static constexpr int loopPlainDelaySamples() noexcept { return sumOf (kRingDelays); }

    /** Full loop traversal, allpasses included; sets the recirculation period. */
    static constexpr int loopSamples() noexcept
    {
        return sumOf (kRingAllpassDelays) + sumOf (kRingDelays);
    }

    static constexpr int totalRamWords() noexcept
    {
        return sumOf (kInputDiffuserDelays) + loopSamples()
             + static_cast<int> (kDarkPredelaySeconds * kInternalRate);
    }

    static float lfoDepthSamplesForRate (std::size_t index, double rate) noexcept
    {
        return static_cast<float> (kLfoDepthSeconds[index] * rate);
    }
};

// The two constraints that make this a plausible model of the hardware class,
// checked at compile time so a future retune cannot silently break either.
static_assert (Fv1RingTankTable::loopPlainDelaySamples() * 1000
                   / static_cast<int> (Fv1RingTankTable::kInternalRate) >= 200,
               "Ring loop must hold at least 200 ms of plain delay or it flutters.");
static_assert (Fv1RingTankTable::totalRamWords() <= Fv1RingTankTable::kDelayRamWords,
               "Structure must fit the FV-1's 32,768-word delay RAM.");

} // namespace sendbloom
