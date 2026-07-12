---
phase: 14
slug: block-level-integration
status: draft
nyquist_compliant: true
wave_0_complete: false
created: 2026-07-08
---

# Phase 14 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Catch2 3.8.1 (CPM via cmake/Tests.cmake) |
| **Config file** | cmake/Tests.cmake |
| **Quick run command** | `ctest --test-dir build -R "GatedBloomChain|SchroederTank32.*processBlock|INTEG-01|INTEG-04" --output-on-failure` |
| **Full suite command** | `ctest --test-dir build --output-on-failure` |
| **Estimated runtime** | ~45 seconds (quick), ~120 seconds (full) |

---

## Sampling Rate

- **After every task commit:** Run task-specific automated command from plan verify block
- **After every plan wave:** Run quick run command
- **Before `/gsd-verify-work`:** Full suite must be green + TEST-09 cases with `authentic_color=1`
- **Max feedback latency:** 120 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 14-01-T1 | 01 | 1 | INTEG-01 | T-14-01 | IReverbEngine default processBlock compiles | build | `cmake --build build --target Tests -j8` | ✅ | pending |
| 14-01-T2 | 01 | 1 | INTEG-01 | T-14-01 | SchroederTank32 routes ProperSRC block path | build | `cmake --build build --target Tests -j8` | ✅ | pending |
| 14-01-T3 | 01 | 1 | INTEG-01 | — | processBlock unit tests pass | unit | `ctest --test-dir build -R "SchroederTank32.*processBlock\|INTEG-01" --output-on-failure` | ❌ Wave 0 | pending |
| 14-02-T1 | 02 | 2 | INTEG-02 | T-14-06 | Scratch buffers prepare-only | build | `cmake --build build --target Tests -j8` | ✅ | pending |
| 14-02-T2 | 02 | 2 | INTEG-02 | T-14-04 | Two-phase block split implemented | build | `cmake --build build --target Tests -j8` | ✅ | pending |
| 14-02-T3 | 02 | 2 | INTEG-03 | T-14-05 | v1 parity GatedBloomChain tests | unit | `ctest --test-dir build -R "GatedBloomChain" --output-on-failure` | ✅ | pending |
| 14-03-T1 | 03 | 3 | INTEG-02 | T-14-09 | Processor scratch prepare-only | build | `cmake --build build --target SendBloom_VST3 -j8` | ✅ | pending |
| 14-03-T2 | 03 | 3 | INTEG-03 | T-14-08 | Dry/mix path regression | integration | `ctest --test-dir build -R "GatedBloomChain\|DryPath\|PostGate\|ReleaseTruth" --output-on-failure` | ✅ | pending |
| 14-03-T3 | 03 | 3 | INTEG-03 | — | 10k host-rate stress | stress | `ctest --test-dir build -R "10k varying block stress" --output-on-failure` | ✅ | pending |
| 14-04-T1 | 04 | 4 | INTEG-04 | T-14-10 | No new APVTS params | static | `ctest --test-dir build -R "INTEG-04" --output-on-failure` | ❌ Wave 0 | pending |
| 14-04-T2 | 04 | 4 | INTEG-04 | T-14-11 | Presets/fresh load authentic off | regression | `ctest --test-dir build -R "INTEG-04\|fresh plugin load\|factory presets" --output-on-failure` | ✅ | pending |
| 14-05-T1 | 05 | 4 | TEST-09 | T-14-12 | Static alloc token scan | static | `ctest --test-dir build -R "TEST-09.*static\|allocation tokens" --output-on-failure` | ❌ Wave 0 | pending |
| 14-05-T2 | 05 | 4 | TEST-09 | T-14-12 | 10k authentic-color stress | stress | `ctest --test-dir build -R "TEST-09\|10k varying block stress" --output-on-failure` | ✅ partial | pending |
| 14-05-T3 | 05 | 4 | TEST-09 | T-14-14 | Block finite-output integration | integration | `ctest --test-dir build -R "BlockIntegration\|TEST-09" --output-on-failure` | ❌ Wave 0 | pending |

---

## Wave 0 Gaps

- [ ] `tests/SchroederTank32BlockTest.cpp`
- [ ] `tests/Phase14IntegrabilityTest.cpp`
- [ ] `tests/IntegrationAllocScanTest.cpp`
- [ ] `tests/BlockIntegrationTest.cpp`
- [ ] Extended `tests/RealtimeStressTest.cpp` TEST-09 tags

---

## Phase Gate

Full suite green; TEST-09 static scan + 10k `authentic_color=1` stress pass; INTEG-03 regression tier (GatedBloomChain, DryPath, PostGate) unchanged; no new APVTS parameters.
