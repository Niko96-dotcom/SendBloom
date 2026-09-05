#include <PluginProcessor.h>
#include <ParameterIDs.h>
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cmath>
#include <cstdlib>
#include <new>
#if defined(_WIN32)
#include <malloc.h>
#endif

namespace
{
// Count C++ allocations on the calling thread only. Catch assertions, input setup,
// and GUI activity stay outside the measured interval. Direct malloc calls are
// outside this counter's coverage.
thread_local bool counting = false;
thread_local size_t allocations = 0;

void* allocate (size_t size, size_t alignment = 0)
{
    if (counting)
        ++allocations;

    void* memory = nullptr;
    size = size == 0 ? 1 : size;
    if (alignment == 0)
        memory = std::malloc (size);
    else
    {
#if defined(_WIN32)
        memory = _aligned_malloc (size, alignment);
#else
        if (posix_memalign (&memory, alignment, size) != 0)
            memory = nullptr;
#endif
    }
    if (memory == nullptr)
        throw std::bad_alloc {};
    return memory;
}

void freeAligned (void* memory) noexcept
{
#if defined(_WIN32)
    _aligned_free (memory);
#else
    std::free (memory);
#endif
}

struct AllocationScope
{
    AllocationScope() { allocations = 0; counting = true; }
    ~AllocationScope() { counting = false; }
};
} // namespace

// Replacements are linked into Tests only, never the plugin bundles.
void* operator new (size_t size) { return allocate (size); }
void* operator new[] (size_t size) { return allocate (size); }
void* operator new (size_t size, std::align_val_t alignment) { return allocate (size, static_cast<size_t> (alignment)); }
void* operator new[] (size_t size, std::align_val_t alignment) { return allocate (size, static_cast<size_t> (alignment)); }
void operator delete (void* memory) noexcept { std::free (memory); }
void operator delete[] (void* memory) noexcept { std::free (memory); }
void operator delete (void* memory, size_t) noexcept { std::free (memory); }
void operator delete[] (void* memory, size_t) noexcept { std::free (memory); }
void operator delete (void* memory, std::align_val_t) noexcept { freeAligned (memory); }
void operator delete[] (void* memory, std::align_val_t) noexcept { freeAligned (memory); }
void operator delete (void* memory, size_t, std::align_val_t) noexcept { freeAligned (memory); }
void operator delete[] (void* memory, size_t, std::align_val_t) noexcept { freeAligned (memory); }

TEST_CASE ("Allocation counter detects scalar array and aligned C++ allocations", "[realtime][allocation]")
{
    {
        AllocationScope scope;
        // Explicit allocation calls cannot be elided like new expressions.
        auto* scalar = ::operator new (32);
        auto* array = ::operator new[] (64);
        auto* aligned = ::operator new (128, std::align_val_t { 64 });
        ::operator delete (scalar);
        ::operator delete[] (array);
        ::operator delete (aligned, std::align_val_t { 64 });
    }
    REQUIRE (allocations == 3);
}

TEST_CASE ("Prepared processor callbacks perform no C++ heap allocations",
           "[realtime][allocation][integration][RT-01][PDC-03]")
{
    using namespace sendbloom::ParameterIDs;
    constexpr std::array<int, 6> sizes { 1, 32, 127, 128, 512, 2048 };
    for (const auto rate : { 44100.0, 48000.0, 96000.0, 192000.0 })
    {
        sendbloom::PluginProcessor plugin;
        auto& state = plugin.getAPVTS();
        *state.getRawParameterValue (sendConnected) = 1.0f;
        *state.getRawParameterValue (sendAmount) = 0.75f;
        *state.getRawParameterValue (distn) = 1.0f;
        plugin.prepareToPlay (rate, 512);

        // Include the first callback after prepare, SRC priming, parameter ramps,
        // and sustained processing. Oversized blocks exercise internal splitting.
        for (int block = 0; block < 60; ++block)
        {
            const auto samples = sizes[static_cast<size_t> (block) % sizes.size()];
            juce::AudioBuffer<float> buffer (2, samples);
            for (int i = 0; i < samples; ++i)
            {
                buffer.setSample (0, i, 0.2f * std::sin (static_cast<float> (i) * 0.1f));
                buffer.setSample (1, i, -0.1f);
            }
            juce::MidiBuffer midi;
            midi.addEvent (juce::MidiMessage::controllerEvent (1, 1, 100), 0);
            if (samples > 32)
                midi.addEvent (juce::MidiMessage::controllerEvent (1, 121, 0), 31);
            *state.getRawParameterValue (bypass) = block % 2 == 0 ? 0.0f : 1.0f;
            *state.getRawParameterValue (gatePrePost) = block % 2 == 0 ? 0.0f : 1.0f;
            *state.getRawParameterValue (size) = block % 2 == 0 ? 0.25f : 0.75f;

            size_t engagedAllocations = 0;
            {
                AllocationScope scope;
                plugin.processBlock (buffer, midi);
                engagedAllocations = allocations;
            }
            size_t bypassAllocations = 0;
            {
                AllocationScope scope;
                plugin.processBlockBypassed (buffer, midi);
                bypassAllocations = allocations;
            }
            INFO ("rate=" << rate << " samples=" << samples << " block=" << block);
            REQUIRE (engagedAllocations == 0);
            REQUIRE (bypassAllocations == 0);
        }
    }
}

TEST_CASE ("Ignoring large SysEx messages performs no C++ heap allocations",
           "[midi][realtime][allocation][regression]")
{
    sendbloom::PluginProcessor plugin;
    plugin.prepareToPlay (48000.0, 128);
    juce::AudioBuffer<float> buffer (2, 1024);
    buffer.clear();
    std::array<juce::uint8, 4096> payload {};
    juce::MidiBuffer midi;
    for (int position : { -1, 0, 63, 128, 511, 1023, 1024 })
        midi.addEvent (juce::MidiMessage::createSysExMessage (payload.data(),
                          static_cast<int> (payload.size())), position);
    midi.addEvent (juce::MidiMessage::controllerEvent (1, 1, 100), 63);
    {
        AllocationScope scope;
        plugin.processBlock (buffer, midi);
    }
    REQUIRE (allocations == 0);
}
