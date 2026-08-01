#pragma once

#include "Fv1RingTankTable.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>

namespace sendbloom
{

/** The shipping reverb tank: a ring of four [allpass -> delay -> shelving]
    blocks fed by four series input diffusers, with the reverb-time coefficient
    applied once per block.

    This replaces the earlier Freeverb-style tank (four parallel combs, 32-37 ms
    each). That structure could not sound like the reference hardware class for a
    structural reason: its recirculation period was ~36 ms where Spin's own
    design notes require at least 200 ms of plain loop delay, and identify
    shorter loops as the cause of "flutter" and a "tinny sound". This ring runs
    ~497 ms of plain delay (~827 ms including allpasses) and fills 96% of the
    FV-1's real 32,768-word RAM budget.

    Fixed-rate: FixedRateAdapter always prepares this at 32,768 Hz. Lengths are
    scaled if prepared at another rate so the tank stays usable in isolation.
*/
class Fv1RingTank
{
public:
    void prepare (double processingRate, int /*maxBlockSize*/) noexcept
    {
        processingRate_ = processingRate;
        const auto scale = processingRate_ / Fv1RingTankTable::kInternalRate;

        for (size_t i = 0; i < inputDiffusers_.size(); ++i)
            inputDiffusers_[i].prepare (scaled (Fv1RingTankTable::kInputDiffuserDelays[i], scale),
                                        Fv1RingTankTable::kInputDiffuserFeedback,
                                        0);

        const auto modMargin = static_cast<int> (std::ceil (
            std::max (Fv1RingTankTable::lfoDepthSamplesForRate (0, processingRate_),
                      Fv1RingTankTable::lfoDepthSamplesForRate (1, processingRate_)))) + 4;

        for (size_t i = 0; i < ringAllpasses_.size(); ++i)
            ringAllpasses_[i].prepare (scaled (Fv1RingTankTable::kRingAllpassDelays[i], scale),
                                       Fv1RingTankTable::kRingAllpassFeedback,
                                       modMargin);

        for (size_t i = 0; i < ringDelays_.size(); ++i)
        {
            ringDelays_[i].prepare (scaled (Fv1RingTankTable::kRingDelays[i], scale));
            tapOffsets_[i] = std::min (scaled (Fv1RingTankTable::kTapOffsets[i], scale),
                                       ringDelays_[i].length() - 1);
        }

        predelay_.prepare (static_cast<int> (std::ceil (
            Fv1RingTankTable::kDarkPredelaySeconds * processingRate_)) + 1);

        for (size_t i = 0; i < lfoIncrement_.size(); ++i)
        {
            lfoIncrement_[i] = static_cast<float> (2.0 * juce::MathConstants<double>::pi
                                                   * Fv1RingTankTable::kLfoHz[i] / processingRate_);
            lfoDepth_[i] = Fv1RingTankTable::lfoDepthSamplesForRate (i, processingRate_);
        }

        setParameters (1.2f, 0.0f);
        reset();
    }

    void setParameters (float rt60Seconds, float darkMix) noexcept
    {
        darkMix_ = juce::jlimit (0.0f, 1.0f, darkMix);
        krt_ = krtForRT60 (rt60Seconds);

        const auto hfHz = juce::jmap (darkMix_,
                                      Fv1RingTankTable::kHfLossHzBright,
                                      Fv1RingTankTable::kHfLossHzDark);
        hfCoeff_ = onePoleCoefficient (hfHz);
        lfCoeff_ = onePoleCoefficient (Fv1RingTankTable::kLfLossHz);

        hfDepth_ = juce::jmap (darkMix_,
                               Fv1RingTankTable::kHfLossDepthBright,
                               Fv1RingTankTable::kHfLossDepthDark);
        lfDepth_ = juce::jmap (darkMix_,
                               Fv1RingTankTable::kLfLossDepthBright,
                               Fv1RingTankTable::kLfLossDepthDark);
    }

    float processSample (float input) noexcept
    {
        // Dark blends in a predelayed feed ("dark with pre-delay").
        const auto delayed = predelay_.process (input);
        auto v = (input + darkMix_ * (delayed - input)) * Fv1RingTankTable::kInputScale;

        for (auto& ap : inputDiffusers_)
            v = ap.process (v, 0.0f);

        std::array<float, 4> mod {};

        for (size_t i = 0; i < lfoPhase_.size(); ++i)
        {
            lfoPhase_[i] += lfoIncrement_[i];

            if (lfoPhase_[i] > juce::MathConstants<float>::twoPi)
                lfoPhase_[i] -= juce::MathConstants<float>::twoPi;

            mod[2 * i]     = std::sin (lfoPhase_[i]) * lfoDepth_[i];
            mod[2 * i + 1] = std::cos (lfoPhase_[i]) * lfoDepth_[i];
        }

        auto acc = loopState_;

        for (size_t b = 0; b < ringDelays_.size(); ++b)
        {
            // Inject the diffused input at three of the four blocks; Spin:
            // "the input may be inserted at only 2, or even 1 spot within the loop".
            if (b > 0)
                acc += v;

            acc = ringAllpasses_[b].process (acc, mod[b]);
            acc = ringDelays_[b].process (acc);

            // Low-frequency loss: subtract a scaled lowpass (high-pass shelf).
            lfState_[b] += lfCoeff_ * (acc - lfState_[b]);
            acc -= lfDepth_ * lfState_[b];

            // High-frequency loss: subtract a scaled highpass (low-pass shelf).
            hfState_[b] += hfCoeff_ * (acc - hfState_[b]);
            acc -= hfDepth_ * (acc - hfState_[b]);

            acc *= krt_;
        }

        loopState_ = flushDenormal (acc);

        auto out = 0.0f;

        for (size_t i = 0; i < ringDelays_.size(); ++i)
            out += Fv1RingTankTable::kTapWeights[i] * ringDelays_[i].tap (tapOffsets_[i]);

        return out * Fv1RingTankTable::kOutputNormalisation;
    }

    void reset() noexcept
    {
        for (auto& ap : inputDiffusers_)
            ap.reset();

        for (auto& ap : ringAllpasses_)
            ap.reset();

        for (auto& d : ringDelays_)
            d.reset();

        predelay_.reset();
        lfState_.fill (0.0f);
        hfState_.fill (0.0f);
        lfoPhase_.fill (0.0f);
        loopState_ = 0.0f;
    }

    /** Loop gain per traversal — exposed so tests can assert stability. */
    float getKrt() const noexcept { return krt_; }

    /** Ideal loop coefficient for a target RT60, before in-loop damping loss.
        krt is applied once per block, so a full traversal costs krt^4. */
    float krtForRT60 (float rt60Seconds) const noexcept
    {
        const auto rt60 = juce::jmax (rt60Seconds, 0.05f);
        const auto loopSeconds = static_cast<float> (Fv1RingTankTable::loopSamples())
                               / static_cast<float> (processingRate_);
        const auto ideal = std::pow (10.0f, -3.0f * loopSeconds / (4.0f * rt60));
        const auto compensated = std::pow (ideal, 1.0f / Fv1RingTankTable::kKrtCompensation);
        return juce::jlimit (0.0f, Fv1RingTankTable::kKrtMax, compensated);
    }

private:
    static int scaled (int samplesAt32k, double scale) noexcept
    {
        return std::max (1, static_cast<int> (std::lround (samplesAt32k * scale)));
    }

    static float flushDenormal (float x) noexcept
    {
        return std::abs (x) < 1.0e-20f ? 0.0f : x;
    }

    float onePoleCoefficient (float cutoffHz) const noexcept
    {
        const auto nyquist = static_cast<float> (processingRate_ * 0.5);
        const auto fc = juce::jlimit (10.0f, nyquist * 0.98f, cutoffHz);
        const auto omega = 2.0f * juce::MathConstants<float>::pi * fc
                         / static_cast<float> (processingRate_);
        return juce::jlimit (0.0f, 1.0f, 1.0f - std::exp (-omega));
    }

    /** Fixed-length delay with an extra read tap from inside the line. */
    class RingDelay
    {
    public:
        void prepare (int lengthSamples)
        {
            length_ = std::max (2, lengthSamples);
            buffer_.assign (static_cast<size_t> (length_), 0.0f);
            writeIndex_ = 0;
        }

        float process (float x) noexcept
        {
            const auto out = buffer_[static_cast<size_t> (writeIndex_)];
            buffer_[static_cast<size_t> (writeIndex_)] = x;
            writeIndex_ = writeIndex_ + 1 >= length_ ? 0 : writeIndex_ + 1;
            return out;
        }

        float tap (int offset) const noexcept
        {
            auto i = writeIndex_ - offset;

            while (i < 0)
                i += length_;

            return buffer_[static_cast<size_t> (i)];
        }

        void reset() noexcept
        {
            std::fill (buffer_.begin(), buffer_.end(), 0.0f);
            writeIndex_ = 0;
        }

        int length() const noexcept { return length_; }

    private:
        std::vector<float> buffer_;
        int length_ { 2 };
        int writeIndex_ { 0 };
    };

    /** Schroeder allpass whose delay can be swept by the ring LFOs. */
    class ModulatedAllpass
    {
    public:
        void prepare (int delaySamples, float feedback, int modMargin)
        {
            delay_ = std::max (1, delaySamples);
            feedback_ = feedback;
            capacity_ = delay_ + modMargin + 2;
            buffer_.assign (static_cast<size_t> (capacity_), 0.0f);
            writeIndex_ = 0;
        }

        float process (float x, float modSamples) noexcept
        {
            const auto d = juce::jlimit (1.0f,
                                         static_cast<float> (capacity_ - 2),
                                         static_cast<float> (delay_) + modSamples);
            const auto d0 = static_cast<int> (d);
            const auto frac = d - static_cast<float> (d0);

            const auto a = at (d0);
            const auto b = at (d0 + 1);
            const auto v = a + frac * (b - a);

            const auto w = x + feedback_ * v;
            buffer_[static_cast<size_t> (writeIndex_)] = w;
            writeIndex_ = writeIndex_ + 1 >= capacity_ ? 0 : writeIndex_ + 1;
            return v - feedback_ * w;
        }

        void reset() noexcept
        {
            std::fill (buffer_.begin(), buffer_.end(), 0.0f);
            writeIndex_ = 0;
        }

    private:
        float at (int back) const noexcept
        {
            auto i = writeIndex_ - back;

            while (i < 0)
                i += capacity_;

            return buffer_[static_cast<size_t> (i)];
        }

        std::vector<float> buffer_;
        int capacity_ { 2 };
        int delay_ { 1 };
        int writeIndex_ { 0 };
        float feedback_ { 0.6f };
    };

    /** Plain predelay for Dark. */
    class Predelay
    {
    public:
        void prepare (int lengthSamples)
        {
            length_ = std::max (2, lengthSamples);
            buffer_.assign (static_cast<size_t> (length_), 0.0f);
            writeIndex_ = 0;
        }

        float process (float x) noexcept
        {
            const auto out = buffer_[static_cast<size_t> (writeIndex_)];
            buffer_[static_cast<size_t> (writeIndex_)] = x;
            writeIndex_ = writeIndex_ + 1 >= length_ ? 0 : writeIndex_ + 1;
            return out;
        }

        void reset() noexcept
        {
            std::fill (buffer_.begin(), buffer_.end(), 0.0f);
            writeIndex_ = 0;
        }

    private:
        std::vector<float> buffer_;
        int length_ { 2 };
        int writeIndex_ { 0 };
    };

    std::array<ModulatedAllpass, 4> inputDiffusers_;
    std::array<ModulatedAllpass, 4> ringAllpasses_;
    std::array<RingDelay, 4> ringDelays_;
    Predelay predelay_;

    std::array<int, 4> tapOffsets_ {};
    std::array<float, 4> lfState_ {};
    std::array<float, 4> hfState_ {};
    std::array<float, 2> lfoPhase_ {};
    std::array<float, 2> lfoIncrement_ {};
    std::array<float, 2> lfoDepth_ {};

    double processingRate_ { Fv1RingTankTable::kInternalRate };
    float loopState_ { 0.0f };
    float krt_ { 0.3f };
    float darkMix_ { 0.0f };
    float hfCoeff_ { 0.5f };
    float lfCoeff_ { 0.02f };
    float hfDepth_ { 0.45f };
    float lfDepth_ { 0.35f };
};

} // namespace sendbloom
