# Phase 19 Baseline Report (BASE-01)

Frozen snapshot of repository / build / test truth **before** production DSP or UI fixes. Phase 19 commits harness, docs, and intentionally failing contracts only (D-04) — **no** production fixes under `source/` for Pressure Mode, span, MIDI purity, Input/Level/Gate, reverb, or branding.

## Identity

| Field | Value |
|-------|-------|
| Commit SHA | `9024ccf324745823270691654c8935368e51f772` |
| Branch | `main` |
| `VERSION` | `0.0.1` |
| Captured (UTC) | `2026-07-12T15:15:19Z` |
| Host | macOS Darwin arm64 (local workstation) |

## Build configuration

| Field | Value |
|-------|-------|
| Generator | Unix Makefiles |
| `CMAKE_BUILD_TYPE` | `Release` |
| `BUILD_DIR` | `Builds` (matches CI `env.BUILD_DIR`) |
| Configure | `cmake -B Builds -DCMAKE_BUILD_TYPE=Release .` |
| Tests target | `cmake --build Builds --config Release --target Tests` |
| cmake submodule | `d5cb9b387d38208b3250398666c5c4b88361a9e9` (`Tests.cmake` present) |
| JUCE submodule | `29396c22c93392d6738e021b83196283d6e4d850` (tag `8.0.12`, reachable) |

## Catch2 / ctest discovery

| Field | Value |
|-------|-------|
| Discovered at capture | **204** (`ctest --test-dir Builds -N` → `Total Tests: 204`) |
| Discovery rule | Runtime only — do **not** hard-code this total into scripts or future docs as an expected constant (BASE-06) |
| Baseline metrics filter | `Builds/Tests "[baseline][metrics]"` |

## CI workflows

| File | Workflow `name` | Job |
|------|-----------------|-----|
| `.github/workflows/build_and_test.yml` | `SendBloom` | `build_and_test` (matrix: Linux / macOS / Windows) |

CI uses `BUILD_DIR=Builds`, `BUILD_TYPE=Release`, recursive submodules, ctest, and pluginval strictness 10 (see workflow).

## Factory preset metrics (BASE-07)

Committed tables and method: [`19-BASELINE-METRICS.md`](./19-BASELINE-METRICS.md).

Summary: all eight factory presets rendered offline at 48 kHz / 512 with a deterministic sine; peak and RMS recorded for L/R. Test tag `[baseline][metrics]` asserts finiteness only.

## Known factory pressure truth (not a pass)

- `Firm_Pressure.xml` / `Hot_Clip.xml`: `send_connected=1` with non-zero `send_amount` (0.85 / 1.0). Documented as current truth; not fixed in Phase 19.

## Known manual / human gaps (not passes)

Mark these `human_needed` / not verified — never treat as automated green (BASE-08 / D-02):

| Gap | Notes |
|-----|-------|
| AU pluginval | CI pluginval path targets VST3; AU `.component` needs manual or future CI |
| Windows matrix | Not run on this capture host |
| Linux matrix | Not run on this capture host |
| DAW smoke | See `docs/DAW_SMOKE_RC0.md` — human evidence |
| Signing / notarization | Release packaging Phase 27 |
| License decision | Product/legal gate — not automated here |
| Unreachable historical JUCE pin | Parent previously recorded `c3c318cf` (missing ZipFile max-uncompressed API on public 8.0.12); realigned to `8.0.12` for builds |

## Phase 19 scope reminder

- **In scope:** cmake restore, this baseline, requirement→artifact traceability, failing `[v1][contract]` tests, `scripts/verify-v1.sh`, honest `human_needed` marking.
- **Out of scope:** production DSP/UI behavior changes under `source/` (Phases 20–25).
