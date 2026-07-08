#pragma once

#include "CDSPResampler.h"
#include "SchroederTank32DelayTable.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <vector>

namespace sendbloom
{

class RateConverterPair
{
public:
    void prepare (double hostRate, int maxHostBlock) noexcept
    {
        maxHostBlock_ = maxHostBlock;
        hostToInternalRatio_ = SchroederTank32DelayTable::kInternalRate / hostRate;

        upsampler_ = std::make_unique<r8b::CDSPResampler> (hostRate,
                                                           SchroederTank32DelayTable::kInternalRate,
                                                           maxHostBlock);

        const auto maxInternalIn = upsampler_->getMaxOutLen (maxHostBlock);
        downsampler_ = std::make_unique<r8b::CDSPResampler> (SchroederTank32DelayTable::kInternalRate,
                                                             hostRate,
                                                             maxInternalIn);

        scratchIn_.resize (static_cast<size_t> (maxHostBlock));
        upOut_.resize (static_cast<size_t> (upsampler_->getMaxOutLen (maxHostBlock)));
        downOut_.resize (static_cast<size_t> (downsampler_->getMaxOutLen (maxInternalIn)));
        leftoverFifo_.resize (static_cast<size_t> (downsampler_->getMaxOutLen (maxInternalIn)));
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

        if (hostWritten >= nHostWanted || nInternal <= 0)
            return hostWritten;

        double* op = nullptr;
        const int produced = downsampler_->process (const_cast<double*> (internalIn), nInternal, op);

        int opIdx = 0;
        while (hostWritten < nHostWanted && opIdx < produced)
            hostOut[hostWritten++] = static_cast<float> (op[opIdx++]);

        const int excess = produced - opIdx;
        for (int i = 0; i < excess; ++i)
            leftoverFifo_[static_cast<size_t> (leftoverDown_++)] = op[opIdx + i];

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

    int getRoundTripLatencySamples() const noexcept
    {
        if (upsampler_ == nullptr || downsampler_ == nullptr)
            return 0;

        return static_cast<int> (std::lround (upsampler_->getLatencyFrac()
                                              + downsampler_->getLatencyFrac()));
    }

    double getHostToInternalRatio() const noexcept
    {
        return hostToInternalRatio_;
    }

    int getMaxUpsampledLen (int nHost) const noexcept
    {
        return upsampler_ != nullptr ? upsampler_->getMaxOutLen (nHost) : 0;
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
    double hostToInternalRatio_ { SchroederTank32DelayTable::kInternalRate / 48000.0 };
};

} // namespace sendbloom
