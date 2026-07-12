# Requirements: SendBloom v1.0

**Defined:** 2026-07-12  
**Core Value:** Pressure Mode, MIDI, and host automation must tell the truth — dry at rest with decaying tails; realtime never lies about blocks, bypass, or allocation.  
**Authoritative source:** `.planning/MILESTONE-SPEC-v1.0-interaction-truth.md`

## v1.0 Requirements

Every requirement below is mandatory for this milestone unless marked `human_needed`.

### Baseline and Traceability (`BASE`)

- [x] **BASE-01**: Record current commit, branch, build configuration, test count, CI state, and known manual gaps before modifications
- [x] **BASE-02**: Create `.planning/REQUIREMENTS.md` entries for every requirement in the master milestone spec
- [x] **BASE-03**: Map each requirement to exactly one phase and at least one verification artifact
- [x] **BASE-04**: Preserve all existing passing ProperSRC, HF, dry-integrity, and release-truth tests unless a requirement explicitly updates their contract
- [x] **BASE-05**: Add a durable v1 verifier script that runs the complete automated gate set
- [x] **BASE-06**: Do not hard-code the expected total number of tests in documentation or scripts
- [x] **BASE-07**: Record baseline audio metrics for representative factory presets before changing DSP
- [x] **BASE-08**: Mark human-only gates as `human_needed`, never silently pass them

### Pressure Interaction (`SEND`)

- [x] **SEND-01**: `send_connected=false` produces always-on wet input
- [x] **SEND-02**: `send_connected=true` and pressure 0 produces no new wet input
- [x] **SEND-03**: Pressure >0 sends input into the wet path
- [x] **SEND-04**: Releasing pressure stops new wet input but preserves existing tail state
- [x] **SEND-05**: UI mouse release sets `send_amount=0` and leaves `send_connected=true`
- [x] **SEND-06**: UI pressed overlay follows pressure/pressed state, not connection state
- [x] **SEND-07**: Advanced UI exposes persistent pressure-mode connection
- [x] **SEND-08**: Pressing the on-screen pad may auto-connect pressure mode without disconnecting on release
- [x] **SEND-09**: Firm and Soft curves remain audibly and numerically distinct
- [x] **SEND-10**: Pressure attack is 3 ms and release is 25 ms
- [x] **SEND-11**: Factory pressure presets load at rest with `send_amount=0`
- [x] **SEND-12**: Disconnecting pressure mode restores ordinary always-on reverb
- [x] **SEND-13**: `send_amount` APVTS ID remains unchanged
- [x] **SEND-14**: Pressure behavior is invariant across host block sizes

### MIDI (`MIDI`)

- [x] **MIDI-01**: MIDI CC1 controls realtime pressure only when pressure mode is connected
- [x] **MIDI-02**: `processBlock` never calls `setValueNotifyingHost`
- [x] **MIDI-03**: `processBlock` never writes `send_amount` raw APVTS state
- [x] **MIDI-04**: CC1 event sample positions are respected
- [x] **MIDI-05**: Multiple CC1 events in one block are applied in order
- [x] **MIDI-06**: CC1 value 0 releases MIDI pressure
- [x] **MIDI-07**: Host/UI pressure and MIDI pressure combine via `max`
- [x] **MIDI-08**: MIDI modulation does not dirty saved plugin state
- [x] **MIDI-09**: Non-CC1 MIDI messages do not change pressure
- [x] **MIDI-10**: MIDI behavior remains finite and deterministic at block sizes 1–2048

### Realtime / Block Engine (`RT`)

- [x] **RT-01**: No `setSize`, `resize`, `assign`, `make_unique`, `push_back`, or equivalent allocation path runs in `PluginProcessor::processBlock`
- [x] **RT-02**: Host blocks larger than `preparedMaxBlock_` retain full wet processing
- [x] **RT-03**: Processing a 2048-sample host block prepared at 512 matches equivalent smaller blocks within tolerance
- [x] **RT-04**: Span processing respects CC1 event boundaries
- [x] **RT-05**: Span processing uses at most 128 samples for control-rate reverb values
- [x] **RT-06**: Dynamic send/distortion/threshold controls are consumed per sample
- [x] **RT-07**: Input gain, level, output gain, and bypass remain per sample
- [x] **RT-08**: Authentic-mode changes request exactly one engine target transition per parameter change
- [x] **RT-09**: Reported latency remains zero under ADR-003 across transitions
- [x] **RT-10**: Engine crossfade begins within the first processed block after the parameter change
- [x] **RT-11**: Engine converges to the final requested target after rapid block-to-block toggles
- [x] **RT-12**: Crossfade completion resets only the now-idle engine
- [x] **RT-13**: Crossfade completion performs zero heap allocations
- [x] **RT-14**: Output remains finite through 10,000-block stress with toggles, MIDI, bypass, and oversized blocks
- [x] **RT-15**: `preparedMaxBlock_ <= 0` is handled safely without allocation or undefined access

### Input, Level, Gate, Bypass (`CORE`)

- [x] **CORE-01**: Input mapping is `-9/0/+9 dB` at `0/0.5/1`
- [x] **CORE-02**: Input display calls the canonical DSP curve
- [x] **CORE-03**: Increasing Input increases wet-path drive
- [x] **CORE-04**: Increasing Input increases the detector level at fixed raw input
- [x] **CORE-05**: Dry tap remains before Input gain
- [x] **CORE-06**: Gate Sens remains an advanced parameter using the existing ID
- [x] **CORE-07**: Gate Sens display reports the canonical threshold in dB
- [x] **CORE-08**: Level changes wet return only
- [x] **CORE-09**: Dead dry-gain fields/smoothers/tests are removed
- [x] **CORE-10**: PostHard close uses a 0.75 ms ramp, not a one-sample snap
- [x] **CORE-11**: PostHard reaches zero no later than 1 ms after the close command
- [x] **CORE-12**: Post gate still chops wet within 15 ms after silence onset
- [x] **CORE-13**: PreSoft retains its long unobtrusive close behavior
- [x] **CORE-14**: Settled bypass is channel-preserving
- [x] **CORE-15**: Settled bypass is unity within floating tolerance
- [x] **CORE-16**: Settled bypass ignores Input, Distn, Gate, Level, and Output settings
- [x] **CORE-17**: Bypass transitions remain click-bounded
- [x] **CORE-18**: Engaged mono-first behavior is unchanged unless `extended_stereo` is later implemented

### Reverb and Dirt Integrity (`DSP`)

- [x] **DSP-01**: Predelay line is clocked continuously in bright and dark modes
- [x] **DSP-02**: Dark mode uses a fixed 55 ms delayed tap blended by dark mix
- [x] **DSP-03**: Re-enabling Dark after bright operation emits no stale frozen burst
- [x] **DSP-04**: Bright/dark automation remains finite and click-bounded
- [x] **DSP-05**: LFO modulation depth is invariant in seconds across sample rates
- [x] **DSP-06**: ProperSRC output is pre-cleared
- [x] **DSP-07**: ProperSRC unwritten samples remain zero
- [x] **DSP-08**: Existing ProperSRC imaging/HF gates remain green
- [x] **DSP-09**: Wet dirt implements the 100 Hz pre-clip high-pass
- [x] **DSP-10**: Wet dirt implements a 20 Hz post-clip DC blocker
- [x] **DSP-11**: Wet dirt long-run DC offset is below the defined gate
- [x] **DSP-12**: Dry path remains unaffected by all wet filtering
- [x] **DSP-13**: `dirt_os` stays disabled and unimplemented
- [x] **DSP-14**: `authentic_color` remains off by default
- [x] **DSP-15**: All factory presets keep `authentic_color=0`

### State, Presets, UI, and Branding (`UX`)

- [x] **UX-01**: Parameter IDs remain unchanged
- [x] **UX-02**: Default `send_amount` becomes 0
- [x] **UX-03**: Factory preset XML and `FactoryPresets.cpp` remain identical in recalled state
- [x] **UX-04**: Pressure presets use connected mode and zero resting pressure
- [x] **UX-05**: Always-on presets use disconnected mode
- [x] **UX-06**: UI explains Pressure Mode without third-party controller naming
- [x] **UX-07**: No product-facing source string contains third-party product/brand/controller names
- [x] **UX-08**: No shipping resource filename contains those names
- [x] **UX-09**: Procedural fallback says `SENDBLOOM`, not the referenced product name
- [x] **UX-10**: Exact reference faceplate asset is removed from the shipping binary unless written permission and a separate legal decision are recorded
- [x] **UX-11**: A Niko-approved original SendBloom faceplate is the production asset, or the original procedural faceplate ships (`human_needed` for asset approval)
- [x] **UX-12**: Legal metadata scan normalizes punctuation/spacing/case and scans filenames
- [x] **UX-13**: `design-qa.md` contains portable repository-relative paths and current evidence
- [x] **UX-14**: Editor hotspots and state overlays remain hittable and correctly aligned after asset replacement
- [x] **UX-15**: Existing preset sessions are explicitly classified as pre-v1 development state; no hidden migration promise is invented
- [x] **UX-16**: README and clean-room docs describe only verified behavior

### Reference and Claim Evidence (`REF`)

- [x] **REF-01**: Add a reproducible reference-capture protocol
- [x] **REF-02**: Add tooling to measure predelay, decay, spectral centroid, gate envelope, harmonic ratios, and DC offset
- [x] **REF-03**: Store derived metrics with capture metadata and knob positions
- [x] **REF-04**: Never commit third-party firmware, EEPROM, bytecode, schematics, or proprietary dumps
- [x] **REF-05**: Hardware recordings, if made, are user-created audio captures only
- [ ] **REF-06**: Compare at least five Size positions in bright and dark modes if hardware is available (`human_needed` if no hardware) — no hardware supplied; grid remains `human_needed`
- [ ] **REF-07**: Compare at least five Input and Distn combinations if hardware is available (`human_needed` if no hardware) — no hardware supplied; grid remains `human_needed`
- [ ] **REF-08**: Compare pre/post gate timing if hardware is available (`human_needed` if no hardware) — no hardware supplied; comparison remains `human_needed`
- [ ] **REF-09**: Compare controller press/release behavior if hardware is available (`human_needed` if no hardware) — no hardware supplied; comparison remains `human_needed`
- [ ] **REF-10**: Niko performs a blind or level-matched listening review (`human_needed`) — explicitly recorded `human_needed`, not passed
- [x] **REF-11**: Closeout assigns one ADR-V1-17 fidelity status
- [x] **REF-12**: Public copy matches the assigned status

### Release (`REL`)

- [ ] **REL-01**: `VERSION` is numeric `1.0.0`
- [ ] **REL-02**: RC tag is `v1.0.0-rc0`
- [ ] **REL-03**: CMake config/build succeeds from a clean directory
- [ ] **REL-04**: Full Catch2 suite passes in Release
- [ ] **REL-05**: VST3 pluginval strictness 10 passes locally
- [ ] **REL-06**: AU validation uses the actual AU type discovered by `auval -a`, not an assumed `aufx`
- [ ] **REL-07**: AU pluginval or equivalent validation passes locally
- [ ] **REL-08**: GitHub Actions is green on macOS, Windows, and Linux
- [ ] **REL-09**: Logic Pro AU smoke passes (`human_needed`)
- [ ] **REL-10**: Cubase VST3 smoke passes (`human_needed`)
- [ ] **REL-11**: REAPER VST3 smoke passes (`human_needed`)
- [ ] **REL-12**: Minimum 10-minute abuse/soak passes in each host (`human_needed`)
- [ ] **REL-13**: JUCE commercial-vs-GPL decision is documented and approved by Niko (`human_needed`)
- [ ] **REL-14**: Repository/distribution license matches the JUCE decision
- [ ] **REL-15**: macOS binaries are Developer ID signed for public distribution (`human_needed` credentials)
- [ ] **REL-16**: macOS distribution package is notarized and stapled (`human_needed` credentials)
- [ ] **REL-17**: Release artifacts have SHA-256 checksums
- [ ] **REL-18**: Release checklist contains real tester/date/result evidence
- [ ] **REL-19**: Working tree is clean at tag
- [ ] **REL-20**: No human gate is represented as automated success

## Future Requirements

Deferred beyond v1.0; tracked but not in this roadmap.

### Deferred product features

- **FUT-01**: Extended Stereo production implementation
- **FUT-02**: Dirt OS / wet-path oversampling
- **FUT-03**: CLAP and AAX formats
- **FUT-04**: Preset browser architecture
- **FUT-05**: Cloud licensing / storefront / telemetry
- **FUT-06**: FDN or alternate reverb topology as production default
- **FUT-07**: Making 32k Color default-on (requires separate product decision + evidence)

## Out of Scope

| Item | Reason |
|------|--------|
| New reverb topology / FDN default | v1 locks current Schroeder tank |
| Changing ProperSRC quality preset | Evidence-only later; not this milestone |
| 32k Color default-on | Product policy unchanged |
| Implementing Extended Stereo | Explicit deferral; param may remain disabled |
| Implementing Dirt OS | Explicit deferral; param may remain disabled |
| Oversampling as new feature | Tied to Dirt OS deferral |
| CLAP / AAX | Not this release |
| Preset browser / cloud licensing / storefront / telemetry | Post-v1 |
| Visual redesign beyond branding/state truth | Branding only |
| Renaming parameter IDs | Immutable |
| Changing `NkMo` / `SbLm` / bundle ID | Locked product identity |
| Claiming circuit emulation / exact fidelity without Phase 26 evidence | ADR-V1-17 |
| Committing firmware/EEPROM/schematics | Clean-room policy |

## Locked ADRs (v1)

These constrain implementation; full text lives in the master milestone spec:

| ADR | Summary |
|-----|---------|
| ADR-V1-01 | Pressure mode state semantics |
| ADR-V1-02 | UI controller press/release (release zeros amount, keeps connected) |
| ADR-V1-03 | MIDI is modulation, not APVTS mutation |
| ADR-V1-04 | Asymmetric pressure smoothing 3 ms / 25 ms |
| ADR-V1-05 | Processing spans, `kControlQuantum = 128` |
| ADR-V1-06 | Dynamic per-sample control arrays |
| ADR-V1-07 | Direct authentic-mode request (no boolean smoother) |
| ADR-V1-08 | Canonical Input −9/0/+9 |
| ADR-V1-09 | Unity dry level; wet-only Level |
| ADR-V1-10 | True bypass final crossfade |
| ADR-V1-11 | PostHard 0.75 ms de-click ramp |
| ADR-V1-12 | Continuous fixed 55 ms dark tap |
| ADR-V1-13 | Modulation time invariant |
| ADR-V1-14 | SRC underfill pre-clear |
| ADR-V1-15 | Wet dirt HP + DC blocker |
| ADR-V1-16 | Numeric `VERSION` 1.0.0; RC in tag name |
| ADR-V1-17 | Fidelity classification |

## Traceability

Exact 1:1 requirement → phase mapping (roadmap 2026-07-12). No orphans, no duplicates.

| Requirement | Phase | Status | Verification artifact |
|-------------|-------|--------|------------------------|
| BASE-01 | Phase 19 | Complete | .planning/phases/SENDBLOOM-19-baseline-contracts-failure-harness/19-BASELINE.md |
| BASE-02 | Phase 19 | Complete | .planning/REQUIREMENTS.md |
| BASE-03 | Phase 19 | Complete | tests/RequirementsTraceabilityTest.cpp#[traceability][BASE-03] |
| BASE-04 | Phase 19 | Complete | tests/ReleaseTruthTest.cpp; tests/DryPathIntegrityTest.cpp |
| BASE-05 | Phase 19 | Complete | scripts/verify-v1.sh (Plan 03) |
| BASE-06 | Phase 19 | Complete | scripts/verify-v1.sh; 19-BASELINE.md discovered-at-capture |
| BASE-07 | Phase 19 | Complete | tests/BaselinePresetMetricsTest.cpp#[baseline][metrics]; 19-BASELINE-METRICS.md |
| BASE-08 | Phase 19 | Complete | scripts/verify-v1.sh human_needed; docs/RELEASE_CHECKLIST.md |
| SEND-01 | Phase 20 | Complete | tests/V1ContractPressure*.cpp#[v1][contract] (Phase 20); tests/PressureSendTest.cpp |
| SEND-02 | Phase 20 | Complete | tests/V1ContractPressure*.cpp#[v1][contract] (Phase 20); tests/PressureSendTest.cpp |
| SEND-03 | Phase 20 | Complete | tests/V1ContractPressure*.cpp#[v1][contract] (Phase 20); tests/PressureSendTest.cpp |
| SEND-04 | Phase 20 | Complete | tests/V1ContractPressure*.cpp#[v1][contract] (Phase 20); tests/PressureSendTest.cpp |
| SEND-05 | Phase 20 | Complete | tests/V1ContractPressure*.cpp#[v1][contract] (Phase 20); tests/PressureSendTest.cpp |
| SEND-06 | Phase 20 | Complete | tests/V1ContractPressure*.cpp#[v1][contract] (Phase 20); tests/PressureSendTest.cpp |
| SEND-07 | Phase 20 | Complete | tests/V1ContractPressure*.cpp#[v1][contract] (Phase 20); tests/PressureSendTest.cpp |
| SEND-08 | Phase 20 | Complete | tests/V1ContractPressure*.cpp#[v1][contract] (Phase 20); tests/PressureSendTest.cpp |
| SEND-09 | Phase 20 | Complete | tests/V1ContractPressure*.cpp#[v1][contract] (Phase 20); tests/PressureSendTest.cpp |
| SEND-10 | Phase 20 | Complete | tests/V1ContractPressure*.cpp#[v1][contract] (Phase 20); tests/PressureSendTest.cpp |
| SEND-11 | Phase 20 | Complete | tests/V1ContractPressure*.cpp#[v1][contract] (Phase 20); tests/PressureSendTest.cpp |
| SEND-12 | Phase 20 | Complete | tests/V1ContractPressure*.cpp#[v1][contract] (Phase 20); tests/PressureSendTest.cpp |
| SEND-13 | Phase 20 | Complete | tests/V1ContractPressure*.cpp#[v1][contract] (Phase 20); tests/PressureSendTest.cpp |
| SEND-14 | Phase 20 | Complete | tests/V1ContractPressure*.cpp#[v1][contract] (Phase 20); tests/PressureSendTest.cpp |
| UX-01 | Phase 20 | Complete | tests/V1Contract*.cpp / UI faceplate sources (Phase 20 UX-01..05) |
| UX-02 | Phase 20 | Complete | tests/V1Contract*.cpp / UI faceplate sources (Phase 20 UX-01..05) |
| UX-03 | Phase 20 | Complete | tests/V1Contract*.cpp / UI faceplate sources (Phase 20 UX-01..05) |
| UX-04 | Phase 20 | Complete | tests/V1Contract*.cpp / UI faceplate sources (Phase 20 UX-01..05) |
| UX-05 | Phase 20 | Complete | tests/V1Contract*.cpp / UI faceplate sources (Phase 20 UX-01..05) |
| RT-01 | Phase 21 | Complete | tests/V1ContractRealtime*.cpp#[v1][contract] (Phase 21); BypassCrossfadeTest |
| RT-02 | Phase 21 | Complete | tests/V1ContractRealtime*.cpp#[v1][contract] (Phase 21); BypassCrossfadeTest |
| RT-03 | Phase 21 | Complete | tests/V1ContractRealtime*.cpp#[v1][contract] (Phase 21); BypassCrossfadeTest |
| RT-05 | Phase 21 | Complete | tests/V1ContractRealtime*.cpp#[v1][contract] (Phase 21); BypassCrossfadeTest |
| RT-08 | Phase 21 | Complete | tests/V1ContractRealtime*.cpp#[v1][contract] (Phase 21); BypassCrossfadeTest |
| RT-09 | Phase 21 | Complete | tests/V1ContractRealtime*.cpp#[v1][contract] (Phase 21); BypassCrossfadeTest |
| RT-10 | Phase 21 | Complete | tests/V1ContractRealtime*.cpp#[v1][contract] (Phase 21); BypassCrossfadeTest |
| RT-11 | Phase 21 | Complete | tests/V1ContractRealtime*.cpp#[v1][contract] (Phase 21); BypassCrossfadeTest |
| RT-12 | Phase 21 | Complete | tests/V1ContractRealtime*.cpp#[v1][contract] (Phase 21); BypassCrossfadeTest |
| RT-13 | Phase 21 | Complete | tests/V1ContractRealtime*.cpp#[v1][contract] (Phase 21); BypassCrossfadeTest |
| RT-14 | Phase 21 | Complete | tests/V1ContractRealtime*.cpp#[v1][contract] (Phase 21); BypassCrossfadeTest |
| RT-15 | Phase 21 | Complete | tests/V1ContractRealtime*.cpp#[v1][contract] (Phase 21); BypassCrossfadeTest |
| CORE-14 | Phase 21 | Complete | Core/bypass/span contracts (Phase 21); BypassCrossfadeTest |
| CORE-15 | Phase 21 | Complete | Core/bypass/span contracts (Phase 21); BypassCrossfadeTest |
| CORE-16 | Phase 21 | Complete | Core/bypass/span contracts (Phase 21); BypassCrossfadeTest |
| CORE-17 | Phase 21 | Complete | Core/bypass/span contracts (Phase 21); BypassCrossfadeTest |
| CORE-18 | Phase 21 | Complete | Core/bypass/span contracts (Phase 21); BypassCrossfadeTest |
| MIDI-01 | Phase 22 | Complete | tests/V1ContractMidi*.cpp#[v1][contract][midi]; tests/MidiSendAmountTest.cpp |
| MIDI-02 | Phase 22 | Complete | tests/V1ContractMidiApvtsPurityTest.cpp#[midi-apvts] |
| MIDI-03 | Phase 22 | Complete | tests/V1ContractMidiApvtsPurityTest.cpp#[midi-apvts] |
| MIDI-04 | Phase 22 | Complete | tests/V1ContractMidiSampleAccurateTest.cpp#[v1][contract][midi] |
| MIDI-05 | Phase 22 | Complete | tests/V1ContractMidiSampleAccurateTest.cpp#[v1][contract][midi] |
| MIDI-06 | Phase 22 | Complete | tests/V1ContractMidiSampleAccurateTest.cpp#[v1][contract][midi] |
| MIDI-07 | Phase 22 | Complete | tests/V1ContractMidiSampleAccurateTest.cpp#[v1][contract][midi] |
| MIDI-08 | Phase 22 | Complete | tests/V1ContractMidiApvtsPurityTest.cpp#[midi-apvts] |
| MIDI-09 | Phase 22 | Complete | tests/V1ContractMidiSampleAccurateTest.cpp#[v1][contract][midi] |
| MIDI-10 | Phase 22 | Complete | tests/V1ContractMidiSampleAccurateTest.cpp#[v1][contract][midi] |
| RT-04 | Phase 22 | Complete | tests/V1ContractMidiSampleAccurateTest.cpp#[RT-04] |
| RT-06 | Phase 22 | Complete | tests/V1ContractPerSampleControlsTest.cpp#[per-sample] |
| RT-07 | Phase 22 | Complete | tests/V1ContractPerSampleControlsTest.cpp#[per-sample] |
| CORE-01 | Phase 23 | Complete | Input/Level/Gate contracts (Phase 23); PostGateTimingTest.cpp |
| CORE-02 | Phase 23 | Complete | Input/Level/Gate contracts (Phase 23); PostGateTimingTest.cpp |
| CORE-03 | Phase 23 | Complete | Input/Level/Gate contracts (Phase 23); PostGateTimingTest.cpp |
| CORE-04 | Phase 23 | Complete | Input/Level/Gate contracts (Phase 23); PostGateTimingTest.cpp |
| CORE-05 | Phase 23 | Complete | Input/Level/Gate contracts (Phase 23); PostGateTimingTest.cpp |
| CORE-06 | Phase 23 | Complete | Input/Level/Gate contracts (Phase 23); PostGateTimingTest.cpp |
| CORE-07 | Phase 23 | Complete | Input/Level/Gate contracts (Phase 23); PostGateTimingTest.cpp |
| CORE-08 | Phase 23 | Complete | Input/Level/Gate contracts (Phase 23); PostGateTimingTest.cpp |
| CORE-09 | Phase 23 | Complete | Input/Level/Gate contracts (Phase 23); PostGateTimingTest.cpp |
| CORE-10 | Phase 23 | Complete | Input/Level/Gate contracts (Phase 23); PostGateTimingTest.cpp |
| CORE-11 | Phase 23 | Complete | Input/Level/Gate contracts (Phase 23); PostGateTimingTest.cpp |
| CORE-12 | Phase 23 | Complete | Input/Level/Gate contracts (Phase 23); PostGateTimingTest.cpp |
| CORE-13 | Phase 23 | Complete | Input/Level/Gate contracts (Phase 23); PostGateTimingTest.cpp |
| DSP-01 | Phase 24 | Complete | DSP fidelity contracts (Phase 24); existing tank/HF/ProperSRC tests |
| DSP-02 | Phase 24 | Complete | DSP fidelity contracts (Phase 24); existing tank/HF/ProperSRC tests |
| DSP-03 | Phase 24 | Complete | DSP fidelity contracts (Phase 24); existing tank/HF/ProperSRC tests |
| DSP-04 | Phase 24 | Complete | DSP fidelity contracts (Phase 24); existing tank/HF/ProperSRC tests |
| DSP-05 | Phase 24 | Complete | DSP fidelity contracts (Phase 24); existing tank/HF/ProperSRC tests |
| DSP-06 | Phase 24 | Complete | DSP fidelity contracts (Phase 24); existing tank/HF/ProperSRC tests |
| DSP-07 | Phase 24 | Complete | DSP fidelity contracts (Phase 24); existing tank/HF/ProperSRC tests |
| DSP-08 | Phase 24 | Complete | DSP fidelity contracts (Phase 24); existing tank/HF/ProperSRC tests |
| DSP-09 | Phase 24 | Complete | DSP fidelity contracts (Phase 24); existing tank/HF/ProperSRC tests |
| DSP-10 | Phase 24 | Complete | DSP fidelity contracts (Phase 24); existing tank/HF/ProperSRC tests |
| DSP-11 | Phase 24 | Complete | DSP fidelity contracts (Phase 24); existing tank/HF/ProperSRC tests |
| DSP-12 | Phase 24 | Complete | DSP fidelity contracts (Phase 24); existing tank/HF/ProperSRC tests |
| DSP-13 | Phase 24 | Complete | DSP fidelity contracts (Phase 24); existing tank/HF/ProperSRC tests |
| DSP-14 | Phase 24 | Complete | DSP fidelity contracts (Phase 24); existing tank/HF/ProperSRC tests |
| DSP-15 | Phase 24 | Complete | DSP fidelity contracts (Phase 24); existing tank/HF/ProperSRC tests |
| UX-06 | Phase 25 | Complete | UX visual/branding gates (Phase 25); docs + human_needed where applicable |
| UX-07 | Phase 25 | Complete | UX visual/branding gates (Phase 25); docs + human_needed where applicable |
| UX-08 | Phase 25 | Complete | UX visual/branding gates (Phase 25); docs + human_needed where applicable |
| UX-09 | Phase 25 | Complete | UX visual/branding gates (Phase 25); docs + human_needed where applicable |
| UX-10 | Phase 25 | Complete | UX visual/branding gates (Phase 25); docs + human_needed where applicable |
| UX-11 | Phase 25 | Complete | UX visual/branding gates (Phase 25); docs + human_needed where applicable |
| UX-12 | Phase 25 | Complete | UX visual/branding gates (Phase 25); docs + human_needed where applicable |
| UX-13 | Phase 25 | Complete | UX visual/branding gates (Phase 25); docs + human_needed where applicable |
| UX-14 | Phase 25 | Complete | UX visual/branding gates (Phase 25); docs + human_needed where applicable |
| UX-15 | Phase 25 | Complete | UX visual/branding gates (Phase 25); docs + human_needed where applicable |
| UX-16 | Phase 25 | Complete | UX visual/branding gates (Phase 25); docs + human_needed where applicable |
| REF-01 | Phase 26 | Pending | docs reference / clean-room artifacts (Phase 26); docs/CLEAN_ROOM.md |
| REF-02 | Phase 26 | Pending | docs reference / clean-room artifacts (Phase 26); docs/CLEAN_ROOM.md |
| REF-03 | Phase 26 | Pending | docs reference / clean-room artifacts (Phase 26); docs/CLEAN_ROOM.md |
| REF-04 | Phase 26 | Pending | docs reference / clean-room artifacts (Phase 26); docs/CLEAN_ROOM.md |
| REF-05 | Phase 26 | Pending | docs reference / clean-room artifacts (Phase 26); docs/CLEAN_ROOM.md |
| REF-06 | Phase 26 | Pending | docs reference / clean-room artifacts (Phase 26); docs/CLEAN_ROOM.md |
| REF-07 | Phase 26 | Pending | docs reference / clean-room artifacts (Phase 26); docs/CLEAN_ROOM.md |
| REF-08 | Phase 26 | Pending | docs reference / clean-room artifacts (Phase 26); docs/CLEAN_ROOM.md |
| REF-09 | Phase 26 | Pending | docs reference / clean-room artifacts (Phase 26); docs/CLEAN_ROOM.md |
| REF-10 | Phase 26 | Pending | docs reference / clean-room artifacts (Phase 26); docs/CLEAN_ROOM.md |
| REF-11 | Phase 26 | Pending | docs reference / clean-room artifacts (Phase 26); docs/CLEAN_ROOM.md |
| REF-12 | Phase 26 | Pending | docs reference / clean-room artifacts (Phase 26); docs/CLEAN_ROOM.md |
| REL-01 | Phase 27 | Pending | docs/RELEASE_CHECKLIST.md; scripts/check-legal-metadata.sh; Phase 27 packaging |
| REL-02 | Phase 27 | Pending | docs/RELEASE_CHECKLIST.md; scripts/check-legal-metadata.sh; Phase 27 packaging |
| REL-03 | Phase 27 | Pending | docs/RELEASE_CHECKLIST.md; scripts/check-legal-metadata.sh; Phase 27 packaging |
| REL-04 | Phase 27 | Pending | docs/RELEASE_CHECKLIST.md; scripts/check-legal-metadata.sh; Phase 27 packaging |
| REL-05 | Phase 27 | Pending | docs/RELEASE_CHECKLIST.md; scripts/check-legal-metadata.sh; Phase 27 packaging |
| REL-06 | Phase 27 | Pending | docs/RELEASE_CHECKLIST.md; scripts/check-legal-metadata.sh; Phase 27 packaging |
| REL-07 | Phase 27 | Pending | docs/RELEASE_CHECKLIST.md; scripts/check-legal-metadata.sh; Phase 27 packaging |
| REL-08 | Phase 27 | Pending | docs/RELEASE_CHECKLIST.md; scripts/check-legal-metadata.sh; Phase 27 packaging |
| REL-09 | Phase 27 | Pending | docs/RELEASE_CHECKLIST.md; scripts/check-legal-metadata.sh; Phase 27 packaging |
| REL-10 | Phase 27 | Pending | docs/RELEASE_CHECKLIST.md; scripts/check-legal-metadata.sh; Phase 27 packaging |
| REL-11 | Phase 27 | Pending | docs/RELEASE_CHECKLIST.md; scripts/check-legal-metadata.sh; Phase 27 packaging |
| REL-12 | Phase 27 | Pending | docs/RELEASE_CHECKLIST.md; scripts/check-legal-metadata.sh; Phase 27 packaging |
| REL-13 | Phase 27 | Pending | docs/RELEASE_CHECKLIST.md; scripts/check-legal-metadata.sh; Phase 27 packaging |
| REL-14 | Phase 27 | Pending | docs/RELEASE_CHECKLIST.md; scripts/check-legal-metadata.sh; Phase 27 packaging |
| REL-15 | Phase 27 | Pending | docs/RELEASE_CHECKLIST.md; scripts/check-legal-metadata.sh; Phase 27 packaging |
| REL-16 | Phase 27 | Pending | docs/RELEASE_CHECKLIST.md; scripts/check-legal-metadata.sh; Phase 27 packaging |
| REL-17 | Phase 27 | Pending | docs/RELEASE_CHECKLIST.md; scripts/check-legal-metadata.sh; Phase 27 packaging |
| REL-18 | Phase 27 | Pending | docs/RELEASE_CHECKLIST.md; scripts/check-legal-metadata.sh; Phase 27 packaging |
| REL-19 | Phase 27 | Pending | docs/RELEASE_CHECKLIST.md; scripts/check-legal-metadata.sh; Phase 27 packaging |
| REL-20 | Phase 27 | Pending | docs/RELEASE_CHECKLIST.md; scripts/check-legal-metadata.sh; Phase 27 packaging |

**Coverage:**

- v1 requirements: 128 total (BASE 8 + SEND 14 + MIDI 10 + RT 15 + CORE 18 + DSP 15 + UX 16 + REF 12 + REL 20)
- Mapped to phases: 128/128 ✓
- Unmapped: 0
- Duplicates: 0

**RT split (resolved):** Phase 21 owns `RT-01..03`, `RT-05`, `RT-08..15` + `CORE-14..18`. Phase 22 owns `MIDI-01..10` + `RT-04`, `RT-06`, `RT-07`.

---
*Requirements defined: 2026-07-12*  
*Last updated: 2026-07-12 — Phase 19-01 added Verification artifact column (BASE-03)*
