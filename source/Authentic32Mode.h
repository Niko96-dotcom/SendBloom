#pragma once

#include <cstdint>

namespace sendbloom
{

// Diagnostics-only mode selector for authentic 32k SRC paths (SRC-04).
// Not exposed via APVTS or user-facing UI.
enum class Authentic32Mode : uint8_t
{
    Off,
    LegacyAccumulator,
    ProperSRC
};

} // namespace sendbloom
