#pragma once

#include "CDSPResampler.h"
#include "SchroederTank32DelayTable.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <memory>
#include <vector>

namespace sendbloom
{

// Guitar-wet SRC quality: wide transition band + moderate attenuation +
// minimum-phase FIR. Default r8brain (2% TB / ~207 dB / linear-phase) yields
// ~90–118 ms priming — unacceptable for this product. These knobs target
// a few ms of wet-only delay while still meeting SRC-06 imaging gates.
struct SrcQuality
{
    double reqTransBandPercent { 25.0 };
    double reqAttenDb { 90.0 };
    r8b::EDSPFilterPhaseResponse phase { r8b::fprLinearPhase };
};

inline constexpr SrcQuality kProperSrcQuality {};

class RateConverterPair
{
public:
    void prepare (double hostRate, int maxHostBlock) noexcept
    {
        prepare (hostRate, maxHostBlock, kProperSrcQuality);
    }

    void prepare (double hostRate, int maxHostBlock, SrcQuality quality) noexcept
    {
        maxHostBlock_ = maxHostBlock;
        hostRate_ = hostRate;
        hostToInternalRatio_ = SchroederTank32DelayTable::kInternalRate / hostRate;
        quality_ = quality;

        upsampler_ = std::make_unique<r8b::CDSPResampler> (hostRate,
                                                           SchroederTank32DelayTable::kInternalRate,
                                                           maxHostBlock,
                                                           quality.reqTransBandPercent,
                                                           quality.reqAttenDb,
                                                           quality.phase);

        const auto maxInternalIn = upsampler_->getMaxOutLen (maxHostBlock);
        downsampler_ = std::make_unique<r8b::CDSPResampler> (SchroederTank32DelayTable::kInternalRate,
                                                             hostRate,
                                                             maxInternalIn,
                                                             quality.reqTransBandPercent,
                                                             quality.reqAttenDb,
                                                             quality.phase);

        scratchIn_.resize (static_cast<size_t> (maxHostBlock));
        upOut_.resize (static_cast<size_t> (upsampler_->getMaxOutLen (maxHostBlock)));
        downOut_.resize (static_cast<size_t> (downsampler_->getMaxOutLen (maxInternalIn)));
        const auto maxDownsampled = downsampler_->getMaxOutLen (maxInternalIn);
        // r8brain may release a short burst after priming. Keep enough fixed storage for
        // that burst plus several maximum host blocks without allocating on the audio thread.
        leftoverFifo_.resize (static_cast<size_t> (maxDownsampled + 8 * maxHostBlock));
        leftoverDown_ = 0;
    }

    int upsample (const float* hostIn, int nHost, double* internalOut) noexcept
    {
        if (upsampler_ == nullptr || nHost <= 0 || nHost > maxHostBlock_)
            return 0;

        for (int i = 0; i < nHost; ++i)
            scratchIn_[static_cast<size_t> (i)] = static_cast<double> (hostIn[i]);

        double* op = nullptr;
        const int produced = upsampler_->process (scratchIn_.data(), nHost, op);

        const int toCopy = std::min (produced, static_cast<int> (upOut_.size()));
        for (int i = 0; i < toCopy; ++i)
            internalOut[i] = op[i];

        return toCopy;
    }

    int downsample (const double* internalIn, int nInternal, float* hostOut, int nHostWanted) noexcept
    {
        if (downsampler_ == nullptr || nHostWanted <= 0)
            return 0;

        // Every input block must reach the streaming resampler, even when old FIFO output
        // is already sufficient for this host callback. Deferring this call would silently
        // discard the current block because the caller will not submit it again.
        double* op = nullptr;
        const int produced = nInternal > 0
                           ? downsampler_->process (const_cast<double*> (internalIn), nInternal, op)
                           : 0;

        int hostWritten = 0;
        int fifoRead = 0;

        while (hostWritten < nHostWanted && fifoRead < leftoverDown_)
        {
            hostOut[hostWritten++] = static_cast<float> (leftoverFifo_[static_cast<size_t> (fifoRead)]);
            ++fifoRead;
        }

        if (fifoRead > 0)
        {
            const int remaining = leftoverDown_ - fifoRead;
            for (int i = 0; i < remaining; ++i)
                leftoverFifo_[static_cast<size_t> (i)] = leftoverFifo_[static_cast<size_t> (fifoRead + i)];
            leftoverDown_ = remaining;
        }

        int opIdx = 0;
        while (hostWritten < nHostWanted && opIdx < produced)
            hostOut[hostWritten++] = static_cast<float> (op[opIdx++]);

        const int excess = produced - opIdx;
        const auto capacity = static_cast<int> (leftoverFifo_.size());
        const auto buffered = std::min (excess, capacity - leftoverDown_);

        for (int i = 0; i < buffered; ++i)
            leftoverFifo_[static_cast<size_t> (leftoverDown_++)] = op[opIdx + i];

        // The fixed FIFO is deliberately oversized in prepare(). If this ever fires in a
        // debug build, increase the bound rather than allocating or dropping samples here.
        assert (buffered == excess);

        return hostWritten;
    }

    void reset() noexcept
    {
        if (upsampler_ != nullptr)
            upsampler_->clear();

        if (downsampler_ != nullptr)
            downsampler_->clear();

        leftoverDown_ = 0;
    }

    // Host-domain round-trip priming: upsample delay is already in host samples;
    // downsample delay is in 32 kHz samples and must be scaled to host time.
    int getRoundTripLatencySamples() const noexcept
    {
        if (upsampler_ == nullptr || downsampler_ == nullptr || hostRate_ <= 0.0)
            return 0;

        const auto upHost = static_cast<double> (upsampler_->getInLenBeforeOutPos (0));
        const auto downInternal = static_cast<double> (downsampler_->getInLenBeforeOutPos (0));
        const auto downHost = downInternal * (hostRate_ / SchroederTank32DelayTable::kInternalRate);
        return static_cast<int> (std::lround (upHost + downHost));
    }

    int getUpsamplerPrimingSamples() const noexcept
    {
        return upsampler_ != nullptr ? upsampler_->getInLenBeforeOutPos (0) : 0;
    }

    int getDownsamplerPrimingSamples() const noexcept
    {
        return downsampler_ != nullptr ? downsampler_->getInLenBeforeOutPos (0) : 0;
    }

    double getHostToInternalRatio() const noexcept
    {
        return hostToInternalRatio_;
    }

    int getMaxUpsampledLen (int nHost) const noexcept
    {
        return upsampler_ != nullptr ? upsampler_->getMaxOutLen (nHost) : 0;
    }

    SrcQuality getQuality() const noexcept
    {
        return quality_;
    }

private:
    std::unique_ptr<r8b::CDSPResampler> upsampler_;
    std::unique_ptr<r8b::CDSPResampler> downsampler_;
    std::vector<double> scratchIn_;
    std::vector<double> upOut_;
    std::vector<double> downOut_;
    std::vector<double> leftoverFifo_;
    int leftoverDown_ { 0 };
    int maxHostBlock_ { 0 };
    double hostRate_ { 48000.0 };
    double hostToInternalRatio_ { SchroederTank32DelayTable::kInternalRate / 48000.0 };
    SrcQuality quality_ {};
};

} // namespace sendbloom
