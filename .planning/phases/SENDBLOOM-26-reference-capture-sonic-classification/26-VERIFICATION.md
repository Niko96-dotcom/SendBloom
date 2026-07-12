# Phase 26 Verification

**Date:** 2026-07-12  
**Status:** passed (Outcome B; phase-scoped automated criteria)  
**Claim status:** `original-inspired`

## Success criteria

1. **Protocol/tooling/metadata — PASS.** Deterministic stimuli and PCM analysis cover predelay, decay, spectral centroid, gate envelope, harmonics, and DC. JSON/CSV retain capture metadata and knob settings. Synthetic tests pass 3/3.
2. **Clean-room inputs — PASS.** Protocol permits only user-created hardware audio; verifier rejects tracked extracted hardware/firmware artifact extensions; legal metadata scan passes. No captures or prohibited artifacts were committed.
3. **Exactly one honest closeout — PASS.** `CLAIM_STATUS.md` assigns only `original-inspired`; README matches. Hardware grids and Niko listening are explicitly `human_needed`, never automated PASS.

## Commands and results

| Command | Result |
|---|---|
| `bash scripts/verify-reference-claims.sh` | PASS; 3/3 reference tests |
| `bash scripts/check-legal-metadata.sh` | PASS |
| `bash -n scripts/verify-v1.sh scripts/verify-reference-claims.sh` | PASS |
| `BUILD_DIR=Builds bash scripts/enab-acceptance-gates.sh` (via canonical runner) | PASS; 10/10 |
| `bash scripts/verify-v1.sh` | RED; 254/259 ctest passed, reference gate PASS |

## Canonical-suite exception (not hidden)

The aggregate runner remains RED because of five pre-existing, out-of-scope failures: one stale source-structure assertion expects `chain.processBlock` directly inside `PluginProcessor::processBlock` although Phase 21 moved it into `processSpan`; four older JUCE XML/GZIP hardening tests disagree with the currently checked-out JUCE behavior. Phase 26 changed documentation, Python tooling/tests, and verifier shell only. These failures are not represented as Phase 26 passes and must be resolved before an RC can claim a green full suite.

## Human evidence

- Five-position Bright/Dark Size grid: `human_needed` (no hardware supplied).
- Input × Distn grid: `human_needed`.
- Pre/Post gate comparison: `human_needed`.
- Controller press/release comparison: `human_needed`.
- Niko blind or level-matched listening review: `human_needed`.

Outcome B from milestone §15.10 is satisfied: tooling/protocol complete, hardware unavailable documented, sole status `original-inspired`, no fidelity claim.
