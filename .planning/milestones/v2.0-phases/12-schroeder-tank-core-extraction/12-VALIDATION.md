---
phase: 12
slug: schroeder-tank-core-extraction
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-07-08
---

# Phase 12 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Catch2 3.8.1 (CPM via cmake/Tests.cmake) |
| **Config file** | cmake/Tests.cmake |
| **Quick run command** | `ctest --test-dir build -C Debug -R "SchroederTankCore|SchroederTank32|HostRate" --output-on-failure` |
| **Full suite command** | `ctest --test-dir build -C Debug --output-on-failure` |
| **Estimated runtime** | ~30 seconds (quick), ~90 seconds (full) |

---

## Sampling Rate

- **After every task commit:** Run quick run command
- **After every plan wave:** Run full suite command
- **Before `/gsd-verify-work`:** Full suite must be green (135+ tests baseline)
- **Max feedback latency:** 90 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 12-01-T1 | 01 | 1 | CORE-01 | T-12-01 | Single processingRate, no mode branches | unit+static | `ctest -R SchroederTankCore` | ❌ Wave 0 | pending |
| 12-01-T2 | 01 | 1 | CORE-02 | — | 32768 Hz unscaled delays | unit | `ctest -R "SchroederTankCore.*32768"` | ❌ Wave 0 | pending |
| 12-01-T3 | 01 | 1 | CORE-04 | — | Fixed-rate RT60 ±15% @ 0.25/0.5/1.0 | unit | `ctest -R "SchroederTankCore.*rt60"` | ❌ Wave 0 | pending |
| 12-02-T1 | 02 | 2 | CORE-03 | T-12-02 | Host wrapper matches legacy host IR | unit | `ctest -R "HostRate.*parity"` | ❌ Wave 0 | pending |
| 12-02-T2 | 02 | 2 | CORE-04 | — | Host-rate RT60 via wrapper ±15% | unit | `ctest -R "HostRate.*rt60"` | ❌ Wave 0 | pending |
| 12-03-T1 | 03 | 3 | CORE-03 | T-12-02 | SchroederTank32 host delegates to wrapper | unit | `ctest -R SchroederTank32` | ✅ partial | pending |
| 12-03-T2 | 03 | 3 | CORE-04 | — | Full regression suite green | integration | `ctest --test-dir build -C Debug` | ✅ | pending |

---

## Wave 0 Gaps

- [ ] `source/SchroederTankCore.h`
- [ ] `source/HostRateReverbEngine.h`
- [ ] `tests/ReverbTestHelpers.h`
- [ ] `tests/SchroederTankCoreTest.cpp`

---

## Phase Gate

All CORE-01 through CORE-04 automated commands pass; SchroederTank32 authentic path tests remain green; no new external packages.
