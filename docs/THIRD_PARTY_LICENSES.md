# Third-Party Licenses

SendBloom incorporates the following third-party software. Full license texts are available from the upstream repositories.

## r8brain-free-src

SendBloom's ProperSRC path uses [r8brain-free-src](https://github.com/avaneev/r8brain-free-src)
(MIT License, Copyright (c) 2013-2025 Aleksey Vaneev) for bandlimited sample-rate conversion
between host rate and the 32,768 Hz Schroeder tank core. See [ADR-003](architecture/ADR-003-proper-32k-src.md) and `cmake-local/R8brain.cmake` for integration details.
