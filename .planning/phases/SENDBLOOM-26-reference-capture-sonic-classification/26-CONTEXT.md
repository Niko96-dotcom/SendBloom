# Phase 26: Reference Capture & Sonic Classification - Context

**Gathered:** 2026-07-12
**Status:** Ready for planning
**Mode:** Smart discuss (recommended answers auto-accepted)

<domain>
## Phase Boundary

Create a reproducible, clean-room reference-capture protocol and deterministic metric tooling, then close ADR-V1-17 honestly. Hardware comparison and listening are evidence inputs, not implementation prerequisites: with no supplied hardware captures or listening verdict, they remain `human_needed` and the sole status is `original-inspired`. This phase does not tune DSP or claim hardware fidelity.

</domain>

<decisions>
## Implementation Decisions

### Capture provenance and grid
- **D-01:** Accept only user-created PCM WAV captures plus explicit JSON metadata; never ingest or commit firmware, EEPROM, bytecode, schematics, proprietary dumps, or third-party extracted assets.
- **D-02:** Document the full reverb, distortion, gate, and pressure grids from the milestone contract, including wet isolation, calibration, filenames, levels, and rejection criteria.
- **D-03:** Keep unavailable hardware comparisons and Niko listening review explicitly `human_needed`; empty placeholders never count as passed evidence.

### Deterministic source and analysis tools
- **D-04:** Use repository-owned Python standard-library tools so capture generation and analysis run without optional scientific packages.
- **D-05:** Generate deterministic stimuli with fixed seeds and machine-readable manifests.
- **D-06:** Emit stable JSON plus a one-row CSV with capture metadata, knob positions, and derived predelay, decay, spectral-centroid, gate-envelope, harmonic-ratio, and DC metrics.
- **D-07:** Validate known synthetic signals with automated unit tests; metric limitations and thresholds are documented rather than hidden.

### Fidelity closeout
- **D-08:** Assign exactly one status in `CLAIM_STATUS.md`: `original-inspired`.
- **D-09:** Public copy remains generic and points to the status; no component-level, exact-emulation, or hardware-matched wording.
- **D-10:** Advancement to `hardware-compared` or `fidelity-claim-approved` requires the evidence named by ADR-V1-17 and is outside this evidence-free closeout.

### Claude's Discretion
- Exact deterministic DSP estimators, file layout, CLI flags, JSON schema details, and test tolerances, provided outputs are auditable and meet REF-01…12.

</decisions>

<canonical_refs>
## Canonical References

- `.planning/MILESTONE-SPEC-v1.0-interaction-truth.md` § ADR-V1-17, §15.4–15.10 — fidelity states, stimuli, grids, analysis outputs, and no-hardware outcome.
- `.planning/REQUIREMENTS.md` REF-01…REF-12 — authoritative phase requirements.
- `docs/CLEAN_ROOM.md` — prohibited inputs and clean-room positioning.
- `docs/RELEASE_CHECKLIST.md` — release truth and human-gate conventions.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `tests/ChainTestHelpers.h` contains Goertzel/harmonic measurement precedent.
- `scripts/verify-v1.sh` is the canonical aggregate automated verifier.
- Existing README and clean-room language already uses original/generic positioning.

### Established Patterns
- Human-only evidence is written as `human_needed`, never promoted by automated tests.
- Release artifacts avoid fixed total test counts and use repository-relative paths.

### Integration Points
- `tools/reference/` for deterministic capture generation and analysis.
- `tests/reference/` for standard-library unit tests.
- `docs/reference-capture-protocol.md` and `CLAIM_STATUS.md` for durable evidence contracts.

</code_context>

<specifics>
## Specific Ideas

- Default to the milestone's Outcome B because no hardware capture package or listening verdict was supplied.
- Analysis must preserve settings and provenance alongside metrics so numbers cannot become detached from capture conditions.

</specifics>

<deferred>
## Deferred Ideas

- Hardware capture grids and Niko blind/level-matched listening review — `human_needed` when equipment/evidence is available.
- DSP retuning from comparison evidence — requires a separate evidence-backed change with safety regression proof and listening approval.

</deferred>

---

*Phase: 26-Reference Capture & Sonic Classification*
*Context gathered: 2026-07-12*
