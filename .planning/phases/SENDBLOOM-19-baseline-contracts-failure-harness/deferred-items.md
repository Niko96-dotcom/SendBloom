# Deferred items — Phase 19 Plan 02

Discovered during 19-02 green proof; **not** introduced by V1Contract* files.

| Item | Evidence | Disposition |
|------|----------|-------------|
| `XmlDocumentEntityExpansionTest` fails on reachable JUCE 8.0.12 (entity expansion / SYSTEM entities still accepted) | `Builds/Tests "~[v1]"` — 3 FAILED cases in `tests/XmlDocumentEntityExpansionTest.cpp` | Out of scope for 19-02; pre-existing vs Wave-0 JUCE pin. Do not weaken. Track for security/JUCE follow-up. |
| `ZipDecompressionBoundsTest` GZIP declared-length case fails (`bytesRead == 7` vs expected 3) | Same `~[v1]` run | Out of scope; Zip max-uncompressed already SKIP'd under `SENDBLOOM_HAS_JUCE_ZIP_MAX_UNCOMPRESSED` (19-01). |
| MIDI §8.4 DSP-effect half (pressure changes without APVTS write) | Research A1 | Deferred to Phase 22 RT path; purity + source scan shipped in 19-02. |

**BASE-04 primary proof for 19-02:** `[release]`, `[DryPath]`, `[ProperSRC]`, `[HF]`, and `BUILD_DIR=Builds bash scripts/enab-acceptance-gates.sh` all passed. Full `~[v1]` is not a clean gate until Xml/Zip JUCE issues are addressed separately.
