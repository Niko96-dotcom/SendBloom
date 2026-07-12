# SendBloom Design QA

## Findings

- No actionable P0/P1/P2 mismatches remain in the automated design surface.
- The production visual source of truth is the procedural chassis in `source/ui/PedalFaceplatePaint.cpp` (`paintProceduralChassis`). This is Path B: the procedural SENDBLOOM faceplate is the sole production paint path.
- Path A, a Niko-approved `sendbloom-faceplate.png`, is deferred until post-RC0 and remains `human_needed`.

## Open Questions

- Final original-asset approval remains a post-RC0 human decision. It does not block Path B or the current release-candidate design gate.
- Host and platform visual checks remain `human_needed` where they were not exercised by the current local run.

## Implementation Checklist

- Source visual truth: `source/ui/PedalFaceplatePaint.cpp` (`paintProceduralChassis`).
- Viewport: `420x780`.
- Default state: collapsed procedural faceplate with the SENDBLOOM wordmark.
- State coverage:
  - Pressure Mode off and on.
  - Pressure pad pressed and released.
  - Gate PRE and POST.
  - Dark mode off and on.
  - Bypass off and on.
  - Advanced drawer collapsed and expanded.
  - Clip LED inactive and active.
- Screenshot evidence is generated from the current editor rather than treated as a committed source asset. Build the snapshot target with `cmake --build Builds --target EditorSnapshot`, capture the editor viewport, and store QA captures under `artifacts/` when evidence is recorded.
- Hotspots and overlays remain aligned to the unchanged `420x780` procedural layout.
- Original-branding sign-off: Path B contains SendBloom-owned procedural geometry and typography. Path A approval is deferred post-RC0 (`human_needed`).
- Build gate: run `cmake --build Builds --config Release --target SendBloom_All Tests EditorSnapshot` and record the result and date at QA time.
- Test gate: run `ctest --test-dir Builds -N` to discover the current suite size; run `ctest --test-dir Builds --output-on-failure` for the pass result. Record the discovered count and date at QA time; do not hard-code a fixed total (BASE-06).
- Shipping-policy gates are green after Phase 25 Plan 01; this QA record now reflects the procedural SENDBLOOM production faceplate.

## Follow-up Polish

- Path B removed the development asset fallback in this phase; the procedural chassis is the sole production paint path.
- Revisit Path A only after RC0 when an original faceplate asset has explicit Niko approval (`human_needed`).

Final result: automated design and shipping-policy surfaces pass; deferred human visual and asset approvals remain explicitly `human_needed`.
