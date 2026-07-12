---
phase: 17-latency-pdc-adr-003
plan: 02
subsystem: docs
tags: [adr, pdc, latency, r8brain, src, fixed-rate-adapter]

requires:
  - phase: 17-latency-pdc-adr-003
    plan: 01
    provides: SrcLatencyTable.h measured four-rate sample counts from LAT-01
provides:
  - ADR-003 architecture and PDC policy record at docs/architecture/ADR-003-proper-32k-src.md
  - LAT-02 Policy A decision with rejected alternatives documented
  - DOC-01 accumulator insufficiency, SRC architecture, and library choice on disk
affects:
  - 17-03
  - 18-enablement

tech-stack:
  added: []
  patterns:
    - "ADR as policy gate before PluginProcessor latency wiring"
    - "Measured latency table must match SrcLatencyTable.h constants"

key-files:
  created:
    - docs/architecture/ADR-003-proper-32k-src.md
  modified: []

key-decisions:
  - "Adopt Policy A — conditional host PDC (0 when authentic_color off; measured round-trip when ProperSRC active)"
  - "Reject Policy C (zero always) for dishonest PDC when ProperSRC is on"
  - "Defer Policy B internal dry delay to Extended unless listening tests show phasing"
  - "Report target-path latency during Phase 16 engine crossfade"

patterns-established:
  - "ADR-003 sample counts mirror source/SrcLatencyTable.h (5208, 5160, 8915, 8670)"

requirements-completed: [LAT-02, DOC-01]

coverage:
  - id: D1
    description: "ADR-003 documents ProperSRC architecture, r8brain choice, and accumulator insufficiency"
    requirement: DOC-01
    verification:
      - kind: other
        ref: "test -f docs/architecture/ADR-003-proper-32k-src.md && grep FixedRateAdapter"
        status: pass
    human_judgment: false
  - id: D2
    description: "ADR-003 locks Policy A conditional host PDC with measured latency table"
    requirement: LAT-02
    verification:
      - kind: other
        ref: "grep 'Policy A' && grep '5160' docs/architecture/ADR-003-proper-32k-src.md"
        status: pass
    human_judgment: true
    rationale: "LAT-02 is an architecture acceptance gate; human review of policy rationale before Phase 18 enablement"

duration: 5min
completed: 2026-07-09
status: complete
---

# Phase 17 Plan 02: ADR-003 Proper 32k SRC Summary

**ADR-003 locks Policy A conditional host PDC, r8brain FixedRateAdapter sandwich, and four-rate measured latency table matching SrcLatencyTable.h**

## Performance

- **Duration:** 5 min
- **Started:** 2026-07-08T22:26:38Z
- **Completed:** 2026-07-08T22:31:00Z
- **Tasks:** 3
- **Files modified:** 1

## Accomplishments

- Created `docs/architecture/` and wrote `ADR-003-proper-32k-src.md`
- Documented why LegacyAccumulator is insufficient (SRC-06 imaging, wrong 32k time base)
- Locked Policy A with rationale, rejected Policy C, deferred Policy B/D/E
- Measured latency table (5208, 5160, 8915, 8670) aligned with `source/SrcLatencyTable.h`
- Documented parallel wet/dry caveat and target-path latency during engine crossfade

## Task Commits

Each task was committed atomically:

1. **Task 1: Create docs/architecture directory** - `88149ac` (chore) — directory + ADR file staged in same commit when `git add` preceded allow-empty
2. **Task 2: Write ADR-003 architecture and accumulator context** - `88149ac` (chore/docs)
3. **Task 3: ADR PDC policy, measured table, and consequences** - `88149ac` (chore/docs) — content delivered in single write; verified by automated greps

**Plan metadata:** skipped (`commit_docs: false`)

## Files Created/Modified

- `docs/architecture/ADR-003-proper-32k-src.md` — LAT-02/DOC-01 architecture decision record

## Decisions Made

- Policy A conditional host PDC is the RC1 default; zero latency when `authentic_color` off
- Policy C rejected — reporting zero with ProperSRC active breaks DAW alignment
- Policy B deferred — host PDC first; internal dry delay only if listening tests fail
- Target-path latency reporting during 35 ms engine crossfade (not max-of-both)

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Latency table sample counts without comma separators for plan grep verify**
- **Found during:** Task 3 verification
- **Issue:** `grep '5160'` failed on formatted `5,160` table cells
- **Fix:** Use bare sample counts (5160, 5208, etc.) in measured latency table to match `SrcLatencyTable.h` and automated verify
- **Files modified:** `docs/architecture/ADR-003-proper-32k-src.md`
- **Committed in:** `88149ac`

**2. Task commit consolidation**
- **Found during:** Task 1 commit
- **Issue:** Staged ADR file caused Task 1 `allow-empty` to include full ADR; Tasks 2–3 had no separate file delta
- **Fix:** Single commit `88149ac` carries all three tasks; automated verification passes on HEAD content
- **Impact:** Atomic delivery unchanged; SUMMARY records one hash for all tasks

---

**Total deviations:** 2 (1 blocking verify fix, 1 commit workflow)
**Impact on plan:** No scope change; LAT-02 and DOC-01 satisfied.

## Issues Encountered

None blocking. Plan 17-03 commits (`1f76484`) landed on the same branch after ADR commit during parallel execution.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- ADR-003 on disk; plan 17-03 (`PluginProcessor` Policy A wiring) already present at `1f76484`
- Phase 18 enablement can proceed after LAT-02 human acceptance of policy rationale

## Self-Check: PASSED

- FOUND: docs/architecture/ADR-003-proper-32k-src.md
- FOUND: 88149ac

---
*Phase: 17-latency-pdc-adr-003*
*Completed: 2026-07-09*
