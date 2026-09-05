#include <WetOverdrive.h>
#include <catch2/catch_test_macros.hpp>
#include <array>
#include <cmath>
#include <vector>

TEST_CASE ("Wet distortion suppresses folded nonharmonic energy", "[od][antialias][regression]")
{
    constexpr int rate = 48000;
    constexpr auto pi = juce::MathConstants<double>::pi;
    for (const int frequency : { 7001, 10003 })
    {
        sendbloom::WetOverdriveState state;
        state.prepare (rate);
        double energy = 0, dc = 0;
        std::array<double, 4> re {}, im {};
        for (int i = 0; i < rate + rate / 4; ++i)
        {
            const auto x = static_cast<float> (0.8 * std::sin (2 * pi * frequency * i / rate));
            const double y = state.process (x, 1);
            if (i < rate / 4) continue;
            energy += y * y;
            dc += y;
            for (int h = 1; h * frequency < rate / 2; ++h)
            {
                const auto angle = 2 * pi * frequency * h * i / rate;
                re[static_cast<size_t> (h)] += y * std::cos (angle);
                im[static_cast<size_t> (h)] += y * std::sin (angle);
            }
        }
        double harmonic = dc * dc / rate;
        for (int h = 1; h * frequency < rate / 2; ++h)
            harmonic += 2 * (re[static_cast<size_t> (h)] * re[static_cast<size_t> (h)]
                          + im[static_cast<size_t> (h)] * im[static_cast<size_t> (h)]) / rate;
        const auto fundamental = 2 * (re[1] * re[1] + im[1] * im[1]) / rate;
        const auto residualDb = 10 * std::log10 (std::max (energy - harmonic, 1e-20) / fundamental);
        INFO ("frequency=" << frequency << " nonharmonic dBc=" << residualDb);
        REQUIRE (fundamental > 1.0);
        // Coherent one-second measurement, after 250 ms settling. This tests
        // numerical alias rejection, not fidelity to an unmeasured circuit.
        REQUIRE (residualDb < -60.0);
    }
}

TEST_CASE ("Wet reconstruction is independent of callback partition and resets", "[od][antialias][realtime]")
{
    constexpr int count = 8192;
    std::vector<float> input (count), blends (count), expected (count), actual (count);
    for (int i = 0; i < count; ++i)
    {
        input[static_cast<size_t> (i)] = 1.5f * std::sin (static_cast<float> (i) * 0.413f);
        blends[static_cast<size_t> (i)] = static_cast<float> (i % 97) / 96;
    }
    for (const double rate : { 44100.0, 48000.0, 96000.0, 192000.0 })
    {
        sendbloom::WetOverdriveState scalar, blocked;
        scalar.prepare (rate);
        blocked.prepare (rate, 512);
        for (int i = 0; i < count; ++i)
            expected[static_cast<size_t> (i)] = scalar.process (input[static_cast<size_t> (i)], blends[static_cast<size_t> (i)]);
        for (int pass = 0; pass < 2; ++pass)
        {
            blocked.reset();
            for (int offset = 0; offset < count;)
            {
                const auto block = std::min (count-offset, 1 + (offset * 37 + pass * 73) % 512);
                blocked.processBlock (input.data()+offset, actual.data()+offset, block, blends.data()+offset);
                offset += block;
            }
            for (int i = 0; i < count; ++i)
            {
                REQUIRE (std::isfinite (actual[static_cast<size_t> (i)]));
                REQUIRE (std::abs (actual[static_cast<size_t> (i)] - expected[static_cast<size_t> (i)]) < 1e-6f);
            }
        }
    }
}
