---
phase: 19
slug: baseline-contracts-failure-harness
status: verified
threats_open: 0
asvs_level: 1
created: 2026-07-12
verified: 2026-07-12
---

# Phase 19 — Security

> ASVS L1 light review for a **harness-only** phase (scripts + Catch2 tests + planning/docs).
> No new network endpoints, authentication, session, or authorization surfaces.
> Threats from PLAN `<threat_model>` blocks and RESEARCH (false-green gates, supply chain) — mitigations verified present.

---

## Scope & Surface

| Surface | In Phase 19? | Notes |
|---------|--------------|-------|
| Production DSP/UI (`source/`) | no | Zero behavior fixes; observe-only contracts |
| Network / auth / sessions | no | Local ctest + bash gates only |
| New packages / npm | no | Catch2 via existing CPM pin; known submodule SHAs |
| Scripts | yes | `scripts/verify-v1.sh` composes local subprocesses |
| Tests / docs | yes | `V1Contract*`, baseline metrics, RELEASE_CHECKLIST |

**ASVS categories (L1 light):**

| ASVS | Applies | Control |
|------|---------|---------|
| V2 Authentication | no | N/A |
| V3 Session Management | no | N/A |
| V4 Access Control | no | N/A |
| V5 Input Validation | light | Deterministic fixtures; bounds in Catch2; no untrusted network I/O |
| V5 / supply chain | yes | Known cmake/JUCE SHAs; no new packages |
| V6 Cryptography | no | N/A |

---

## Trust Boundaries

| Boundary | Description | Data Crossing |
|----------|-------------|---------------|
| developer workstation → cmake submodule / CPM | Fetch of cmake-includes + Catch2 pin | Source tarballs / git objects — pin to known SHAs only |
| developer workstation → JUCE submodule | Audio framework pin | Reachable tag/SHA only (8.0.12) |
| planning docs / verify-v1 → CI & humans | Gate status claims | Must not falsely claim human or automated PASS |
| test harness → PluginProcessor | Offline renders | Deterministic in-repo FactoryPresets / fixtures only |

---

## Threat Register

| Threat ID | Category | Component | Severity | Disposition | Mitigation | Status |
|-----------|----------|-----------|----------|-------------|------------|--------|
| T-19-01 | Tampering | cmake submodule / CPM Catch2 | high | mitigate | Restored known sudara/cmake-includes SHA with `Tests.cmake`; Catch2@3.8.1 via existing CPM; no npm Catch2 | closed |
| T-19-02 | Repudiation | 19-BASELINE.md manual gaps | medium | mitigate | Manual gaps listed as gaps/not verified — never as pass; mirrored in verify-v1 `human_needed` | closed |
| T-19-03 | Tampering | BaselinePresetMetricsTest preset XML | low | accept | In-repo FactoryPresets only; XmlDocumentEntityExpansionTest remains elsewhere | closed |
| T-19-04 | Tampering | V1Contract*.cpp vs green tests | high | mitigate | New contract files only; preexisting MidiSendAmount/ParameterCurves/ReleaseTruth/PostGateTiming/DryPathIntegrity untouched; BASE-04 green proof passed | closed |
| T-19-05 | Repudiation | Intentionally failing tests | medium | mitigate | Tags `[v1][contract][<id>]` + SUMMARY/BASELINE expected-reds docs; contracts assert real REQUIREs (not `[.]` skipped) | closed |
| T-19-06 | Denial of service | Flaky contract setup | medium | mitigate | §8.4 settle/prepare recipes; failures match intended predicates (EC=42) | closed |
| T-19-07 | Repudiation | verify-v1 human section (false-green gates) | high | mitigate | Explicit `human_needed` lines; grep rejects `human_needed`+PASS pairing; exit nonzero on automated FAIL; pluginval SKIPPED ≠ PASS | closed |
| T-19-08 | Tampering | hard-coded totals in docs/scripts | medium | mitigate | Runtime `ctest -N` discovery; checklist discovery language; no `expected_total` / `TOTAL_TESTS=N` | closed |
| T-19-09 | Information disclosure | pluginval logs | low | accept | Optional local tool; no secrets expected in Phase 19 harness | closed |
| T-19-SC | Tampering | package installs (supply chain) | high | mitigate | No new packages across 19-01/02/03; legitimacy OK for existing Catch2 pin | closed |

*Status: open · closed · open — below block threshold (non-blocking)*
*Severity: critical > high > medium > low — only open threats at or above `workflow.security_block_on` count toward `threats_open`*
*Disposition: mitigate · accept · transfer*

### RESEARCH patterns (cross-check)

| Pattern | STRIDE | Mitigation present? |
|---------|--------|---------------------|
| Slopsquat / wrong-ecosystem package | Tampering | yes — T-19-01 / T-19-SC |
| Hiding failing gates as pass (false-green) | Repudiation | yes — T-19-02 / T-19-07 / T-19-08 |
| XML entity expansion in preset load | Tampering | yes — existing XmlDocumentEntityExpansionTest not weakened (T-19-03 accept + defer JUCE noise) |
| Zip slip in resources | Tampering | yes — ZipDecompressionBoundsTest retained; max-uncompressed SKIP until API present |

---

## Accepted Risks Log

| Risk ID | Threat Ref | Rationale | Accepted By | Date |
|---------|------------|-----------|-------------|------|
| AR-19-01 | T-19-03 | Factory preset XML from in-repo fixtures only; entity-expansion coverage lives in separate security test (deferred JUCE 8.0.12 Xml/Zip noise tracked outside Phase 19 harness goal) | plan disposition + secure-phase L1 | 2026-07-12 |
| AR-19-02 | T-19-09 | Optional pluginval may emit local logs; harness does not introduce secrets or network auth | plan disposition + secure-phase L1 | 2026-07-12 |

---

## Evidence Summary

| Check | Evidence |
|-------|----------|
| No new network/auth | SUMMARYs + VERIFICATION: scripts/tests/docs/submodules only; `source/` diff empty |
| Supply chain | cmake `d5cb9b3`, JUCE `8.0.12` / `29396c22`; no new package installs |
| False-green gates | `verify-v1.sh` STATUS TABLE + HUMAN_NEEDED; BASE-08 greps clean |
| Contract honesty | Seven `V1Contract*.cpp`; intentional reds documented; greens preserved |
| Register authored at plan time | yes — `<threat_model>` in 19-01/02/03 PLAN.md |

---

## Security Audit Trail

| Audit Date | Threats Total | Closed | Open | Run By |
|------------|---------------|--------|------|--------|
| 2026-07-12 | 10 | 10 | 0 | gsd-nyquist-auditor (ASVS L1 light / secure-phase post-verify) |

---

## Sign-Off

- [x] All threats have a disposition (mitigate / accept / transfer)
- [x] Accepted risks documented in Accepted Risks Log
- [x] `threats_open: 0` confirmed
- [x] `status: verified` set in frontmatter
- [x] No new network/auth surfaces in phase scope
- [x] RESEARCH false-green + supply-chain threats marked mitigated

**Approval:** verified 2026-07-12
