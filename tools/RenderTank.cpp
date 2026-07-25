/*  Offline renderer for the reverb tank and the wet overdrive.

    Writes raw 32-bit float mono at the tank's internal rate so the analysis
    scripts under tools/reference can measure RT60, spectral decay and loop
    ripple against captured reference material without booting a host.

    Usage:
      RenderTank ir   <out.f32> <rt60> <darkMix> <seconds> [ring|legacy]
      RenderTank od   <out.f32> <blend>
      RenderTank host <out.f32> <hostRate> <rt60> <darkMix> <seconds>
      RenderTank src    <out.f32> <hostRate> <seconds>
      RenderTank stream <in.f32> <out.f32> <rt60> <darkMix> <distn> [ring|legacy]
*/

#include <FixedRateAdapter.h>
#include <RateConverterPair.h>
#include <Fv1RingTank.h>
#include <Fv1RingTankTable.h>
#include <SchroederTankCore.h>
#include <WetOverdrive.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace
{
constexpr double kRate = sendbloom::Fv1RingTankTable::kInternalRate;

bool writeF32 (const std::string& path, const std::vector<float>& data)
{
    std::FILE* f = std::fopen (path.c_str(), "wb");

    if (f == nullptr)
        return false;

    const auto written = std::fwrite (data.data(), sizeof (float), data.size(), f);
    std::fclose (f);
    return written == data.size();
}

int renderImpulse (int argc, char** argv)
{
    if (argc < 6)
        return 2;

    const std::string out = argv[2];
    const auto rt60 = static_cast<float> (std::atof (argv[3]));
    const auto dark = static_cast<float> (std::atof (argv[4]));
    const auto seconds = std::atof (argv[5]);
    const std::string engine = argc > 6 ? argv[6] : "ring";

    const auto n = static_cast<int> (seconds * kRate);
    std::vector<float> y (static_cast<size_t> (n), 0.0f);

    if (engine == "legacy")
    {
        sendbloom::SchroederTankCore core;
        core.prepare (kRate, 512);
        core.setParameters (rt60, dark);

        for (int i = 0; i < n; ++i)
            y[static_cast<size_t> (i)] = core.processSample (i == 0 ? 1.0f : 0.0f);
    }
    else
    {
        sendbloom::Fv1RingTank core;
        core.prepare (kRate, 512);
        core.setParameters (rt60, dark);

        for (int i = 0; i < n; ++i)
            y[static_cast<size_t> (i)] = core.processSample (i == 0 ? 1.0f : 0.0f);

        std::fprintf (stderr, "krt=%.6f loop=%d samples (%.1f ms plain, %.1f ms total) ram=%d/%d\n",
                      core.getKrt(),
                      sendbloom::Fv1RingTankTable::loopSamples(),
                      1000.0 * sendbloom::Fv1RingTankTable::loopPlainDelaySamples() / kRate,
                      1000.0 * sendbloom::Fv1RingTankTable::loopSamples() / kRate,
                      sendbloom::Fv1RingTankTable::totalRamWords(),
                      sendbloom::Fv1RingTankTable::kDelayRamWords);
    }

    return writeF32 (out, y) ? 0 : 1;
}

/** Impulse through the full shipping path (SRC -> ring -> SRC) at a host rate. */
int renderHost (int argc, char** argv)
{
    if (argc < 7)
        return 2;

    const std::string out = argv[2];
    const auto hostRate = std::atof (argv[3]);
    const auto rt60 = static_cast<float> (std::atof (argv[4]));
    const auto dark = static_cast<float> (std::atof (argv[5]));
    const auto seconds = std::atof (argv[6]);

    constexpr int kBlock = 512;
    sendbloom::FixedRateAdapter adapter;
    adapter.prepare (hostRate, kBlock);

    const auto n = static_cast<int> (seconds * hostRate);
    std::vector<float> y (static_cast<size_t> (n), 0.0f);
    std::vector<float> in (static_cast<size_t> (kBlock), 0.0f);

    for (int i = 0; i < n; i += kBlock)
    {
        const auto count = std::min (kBlock, n - i);
        std::fill (in.begin(), in.end(), 0.0f);

        if (i == 0)
            in[0] = 1.0f;

        adapter.processBlock (in.data(), y.data() + i, count, rt60, dark);
    }

    return writeF32 (out, y) ? 0 : 1;
}

/** Impulse through the rate converters ONLY (host -> 32768 -> host), tank
    bypassed, so the SRC round-trip response can be measured on its own. */
int renderSrc (int argc, char** argv)
{
    if (argc < 5)
        return 2;

    const std::string out = argv[2];
    const auto hostRate = std::atof (argv[3]);
    const auto seconds = std::atof (argv[4]);

    constexpr int kBlock = 512;
    sendbloom::RateConverterPair converters;
    converters.prepare (hostRate, kBlock);

    const auto n = static_cast<int> (seconds * hostRate);
    std::vector<float> y (static_cast<size_t> (n), 0.0f);
    std::vector<float> in (static_cast<size_t> (kBlock), 0.0f);
    std::vector<double> mid (static_cast<size_t> (converters.getMaxUpsampledLen (kBlock)), 0.0);

    for (int i = 0; i < n; i += kBlock)
    {
        const auto count = std::min (kBlock, n - i);
        std::fill (in.begin(), in.end(), 0.0f);

        if (i == 0)
            in[0] = 1.0f;

        const auto produced = converters.upsample (in.data(), count, mid.data());
        converters.downsample (mid.data(), produced, y.data() + i, count);
    }

    return writeF32 (out, y) ? 0 : 1;
}

/** Stream raw f32 at the tank rate through a tank plus the wet overdrive, so
    real playing can be A/B'd between engines. Python side handles WAV I/O. */
int renderStream (int argc, char** argv)
{
    if (argc < 8)
        return 2;

    const std::string in = argv[2];
    const std::string out = argv[3];
    const auto rt60 = static_cast<float> (std::atof (argv[4]));
    const auto dark = static_cast<float> (std::atof (argv[5]));
    const auto distn = static_cast<float> (std::atof (argv[6]));
    const std::string engine = argv[7];

    std::FILE* f = std::fopen (in.c_str(), "rb");

    if (f == nullptr)
        return 1;

    std::fseek (f, 0, SEEK_END);
    const auto bytes = std::ftell (f);
    std::fseek (f, 0, SEEK_SET);
    std::vector<float> x (static_cast<size_t> (bytes) / sizeof (float));

    if (std::fread (x.data(), sizeof (float), x.size(), f) != x.size())
    {
        std::fclose (f);
        return 1;
    }

    std::fclose (f);

    std::vector<float> y (x.size(), 0.0f);
    sendbloom::WetOverdriveState od;
    od.prepare (kRate);

    if (engine == "legacy")
    {
        sendbloom::SchroederTankCore core;
        core.prepare (kRate, 512);
        core.setParameters (rt60, dark);

        for (size_t i = 0; i < x.size(); ++i)
            y[i] = od.process (core.processSample (x[i]), distn);
    }
    else
    {
        sendbloom::Fv1RingTank core;
        core.prepare (kRate, 512);
        core.setParameters (rt60, dark);

        for (size_t i = 0; i < x.size(); ++i)
            y[i] = od.process (core.processSample (x[i]), distn);
    }

    return writeF32 (out, y) ? 0 : 1;
}

int renderOverdrive (int argc, char** argv)
{
    if (argc < 4)
        return 2;

    const std::string out = argv[2];
    const auto blend = static_cast<float> (std::atof (argv[3]));

    // Transfer curve sweep: -2 .. +2 in 4001 steps, stateless branch.
    std::vector<float> y;
    y.reserve (4001);

    for (int i = 0; i < 4001; ++i)
    {
        const auto x = -2.0f + 4.0f * static_cast<float> (i) / 4000.0f;
        y.push_back (sendbloom::WetOverdrive::process (x, blend));
    }

    return writeF32 (out, y) ? 0 : 1;
}
} // namespace

int main (int argc, char** argv)
{
    if (argc < 2)
    {
        std::fprintf (stderr, "usage: RenderTank ir|od ...\n");
        return 2;
    }

    if (std::strcmp (argv[1], "ir") == 0)
        return renderImpulse (argc, argv);

    if (std::strcmp (argv[1], "od") == 0)
        return renderOverdrive (argc, argv);

    if (std::strcmp (argv[1], "host") == 0)
        return renderHost (argc, argv);

    if (std::strcmp (argv[1], "src") == 0)
        return renderSrc (argc, argv);

    if (std::strcmp (argv[1], "stream") == 0)
        return renderStream (argc, argv);

    std::fprintf (stderr, "unknown mode '%s'\n", argv[1]);
    return 2;
}
