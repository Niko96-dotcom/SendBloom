# Phase 27: RC Verification, Licensing & Distribution - Context

**Gathered:** 2026-07-12
**Status:** Ready for planning
**Mode:** Smart discuss with standing acceptance of recommended answers

<domain>
## Phase Boundary

Close every locally automatable RC gate, produce truthful release evidence and distribution tooling, and leave external CI, DAW, credential, signing, notarization, and manual evidence explicitly `human_needed`. Do not create `v1.0.0-rc0` until every required gate and clean-tag condition is actually satisfied.

</domain>

<decisions>
## Implementation Decisions

### Automated release blockers
- Repair the stale ReleaseTruth source-structure assertion to follow the production span helper while preserving its block-level integration intent.
- Treat JUCE 8.0.12 XML/GZIP behavior as upstream API reality, not as product hardening proof.
- Enforce bounded, DTD-free XML at SendBloom's host/preset ingestion boundaries; test the product boundary directly.
- Do not claim a generic GZIP declared-length field is an output security cap when JUCE documents it as metadata.

### Licensing and distribution
- Use the recommended commercial JUCE distribution path, subject to real confirmation that Niko holds a valid covering JUCE commercial license.
- Keep SendBloom's own source under the existing MIT license only if that commercial-license fact is confirmed; otherwise release remains blocked and GPL alignment must be handled explicitly.
- Provide secret-free packaging/signing/notarization scripts that consume environment/keychain credentials.
- Package only locally built and verified platforms; never fabricate Windows/Linux artifacts from macOS.

### Evidence and promotion
- `VERSION` becomes numeric `1.0.0`; RC identity remains exclusively the eventual `v1.0.0-rc0` tag.
- Local automated reports may record only commands actually run and their outputs.
- CI run URLs, host/version/tester/date DAW smoke and soak, JUCE entitlement confirmation, signing identity, notarization submission, and clean-machine checks remain external evidence gates.
- No RC tag, milestone archive, or completed release claim while any required external gate is absent.

### Claude's Discretion
- Exact release-script structure, artifact layout, log naming, and defensive XML helper design may follow repository conventions.

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `scripts/verify-v1.sh` already distinguishes automated gates from `human_needed` gates.
- `scripts/check-legal-metadata.sh`, CI pluginval configuration, `docs/RELEASE_CHECKLIST.md`, and Phase 26 claim evidence provide reusable release inputs.

### Established Patterns
- CMake/Ninja Release builds, Catch2/CTest, strictness-10 pluginval, shell-based truth gates, and Markdown evidence reports.
- Planning artifacts never promote verifier-only or unavailable human evidence to PASS.

### Integration Points
- `source/PluginProcessor.cpp` and `source/FactoryPresets.cpp` are XML ingestion boundaries.
- `tests/ReleaseTruthTest.cpp`, `tests/XmlDocumentEntityExpansionTest.cpp`, and `tests/ZipDecompressionBoundsTest.cpp` contain the five known blockers.
- `VERSION`, release docs/scripts, CI, AU/VST3 artifacts, and final tag form the RC promotion surface.

</code_context>

<specifics>
## Specific Ideas

The user's standing `approved` accepts recommended product/legal choices but is not evidence of external facts. The release report must make that distinction obvious.

</specifics>

<deferred>
## Deferred Ideas

Path A hardware capture remains post-RC0. Storefront, telemetry, cloud licensing, AAX, and CLAP remain outside this phase.

</deferred>
