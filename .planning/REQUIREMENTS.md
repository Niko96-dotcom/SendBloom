# Requirements: SendBloom v1.0

**Defined:** 2026-07-12  
**Core Value:** Pressure Mode, MIDI, and host automation must tell the truth — dry at rest with decaying tails; realtime never lies about blocks, bypass, or allocation.  
**Authoritative source:** `.planning/MILESTONE-SPEC-v1.0-interaction-truth.md`

## v1.0 Requirements

Every requirement below is mandatory for this milestone unless marked `human_needed`.

### Baseline and Traceability (`BASE`)

- [ ] **BASE-01**: Record current commit, branch, build configuration, test count, CI state, and known manual gaps before modifications
- [ ] **BASE-02**: Create `.planning/REQUIREMENTS.md` entries for every requirement in the master milestone spec
- [ ] **BASE-03**: Map each requirement to exactly one phase and at least one verification artifact
- [ ] **BASE-04**: Preserve all existing passing ProperSRC, HF, dry-integrity, and release-truth tests unless a requirement explicitly updates their contract
- [ ] **BASE-05**: Add a durable v1 verifier script that runs the complete automated gate set
- [ ] **BASE-06**: Do not hard-code the expected total number of tests in documentation or scripts
- [ ] **BASE-07**: Record baseline audio metrics for representative factory presets before changing DSP
- [ ] **BASE-08**: Mark human-only gates as `human_needed`, never silently pass them

### Pressure Interaction (`SEND`)

- [ ] **SEND-01**: `send_connected=false` produces always-on wet input
- [ ] **SEND-02**: `send_connected=true` and pressure 0 produces no new wet input
- [ ] **SEND-03**: Pressure >0 sends input into the wet path
- [ ] **SEND-04**: Releasing pressure stops new wet input but preserves existing tail state
- [ ] **SEND-05**: UI mouse release sets `send_amount=0` and leaves `send_connected=true`
- [ ] **SEND-06**: UI pressed overlay follows pressure/pressed state, not connection state
- [ ] **SEND-07**: Advanced UI exposes persistent pressure-mode connection
- [ ] **SEND-08**: Pressing the on-screen pad may auto-connect pressure mode without disconnecting on release
- [ ] **SEND-09**: Firm and Soft curves remain audibly and numerically distinct
- [ ] **SEND-10**: Pressure attack is 3 ms and release is 25 ms
- [ ] **SEND-11**: Factory pressure presets load at rest with `send_amount=0`
- [ ] **SEND-12**: Disconnecting pressure mode restores ordinary always-on reverb
- [ ] **SEND-13**: `send_amount` APVTS ID remains unchanged
- [ ] **SEND-14**: Pressure behavior is invariant across host block sizes

### MIDI (`MIDI`)

- [ ] **MIDI-01**: MIDI CC1 controls realtime pressure only when pressure mode is connected
- [ ] **MIDI-02**: `processBlock` never calls `setValueNotifyingHost`
- [ ] **MIDI-03**: `processBlock` never writes `send_amount` raw APVTS state
- [ ] **MIDI-04**: CC1 event sample positions are respected
- [ ] **MIDI-05**: Multiple CC1 events in one block are applied in order
- [ ] **MIDI-06**: CC1 value 0 releases MIDI pressure
- [ ] **MIDI-07**: Host/UI pressure and MIDI pressure combine via `max`
- [ ] **MIDI-08**: MIDI modulation does not dirty saved plugin state
- [ ] **MIDI-09**: Non-CC1 MIDI messages do not change pressure
- [ ] **MIDI-10**: MIDI behavior remains finite and deterministic at block sizes 1–2048

### Realtime / Block Engine (`RT`)

- [ ] **RT-01**: No `setSize`, `resize`, `assign`, `make_unique`, `push_back`, or equivalent allocation path runs in `PluginProcessor::processBlock`
- [ ] **RT-02**: Host blocks larger than `preparedMaxBlock_` retain full wet processing
- [ ] **RT-03**: Processing a 2048-sample host block prepared at 512 matches equivalent smaller blocks within tolerance
- [ ] **RT-04**: Span processing respects CC1 event boundaries
- [ ] **RT-05**: Span processing uses at most 128 samples for control-rate reverb values
- [ ] **RT-06**: Dynamic send/distortion/threshold controls are consumed per sample
- [ ] **RT-07**: Input gain, level, output gain, and bypass remain per sample
- [ ] **RT-08**: Authentic-mode changes request exactly one engine target transition per parameter change
- [ ] **RT-09**: Reported latency remains zero under ADR-003 across transitions
- [ ] **RT-10**: Engine crossfade begins within the first processed block after the parameter change
- [ ] **RT-11**: Engine converges to the final requested target after rapid block-to-block toggles
- [ ] **RT-12**: Crossfade completion resets only the now-idle engine
- [ ] **RT-13**: Crossfade completion performs zero heap allocations
- [ ] **RT-14**: Output remains finite through 10,000-block stress with toggles, MIDI, bypass, and oversized blocks
- [ ] **RT-15**: `preparedMaxBlock_ <= 0` is handled safely without allocation or undefined access

### Input, Level, Gate, Bypass (`CORE`)

- [ ] **CORE-01**: Input mapping is `-9/0/+9 dB` at `0/0.5/1`
- [ ] **CORE-02**: Input display calls the canonical DSP curve
- [ ] **CORE-03**: Increasing Input increases wet-path drive
- [ ] **CORE-04**: Increasing Input increases the detector level at fixed raw input
- [ ] **CORE-05**: Dry tap remains before Input gain
- [ ] **CORE-06**: Gate Sens remains an advanced parameter using the existing ID
- [ ] **CORE-07**: Gate Sens display reports the canonical threshold in dB
- [ ] **CORE-08**: Level changes wet return only
- [ ] **CORE-09**: Dead dry-gain fields/smoothers/tests are removed
- [ ] **CORE-10**: PostHard close uses a 0.75 ms ramp, not a one-sample snap
- [ ] **CORE-11**: PostHard reaches zero no later than 1 ms after the close command
- [ ] **CORE-12**: Post gate still chops wet within 15 ms after silence onset
- [ ] **CORE-13**: PreSoft retains its long unobtrusive close behavior
- [ ] **CORE-14**: Settled bypass is channel-preserving
- [ ] **CORE-15**: Settled bypass is unity within floating tolerance
- [ ] **CORE-16**: Settled bypass ignores Input, Distn, Gate, Level, and Output settings
- [ ] **CORE-17**: Bypass transitions remain click-bounded
- [ ] **CORE-18**: Engaged mono-first behavior is unchanged unless `extended_stereo` is later implemented

### Reverb and Dirt Integrity (`DSP`)

- [ ] **DSP-01**: Predelay line is clocked continuously in bright and dark modes
- [ ] **DSP-02**: Dark mode uses a fixed 55 ms delayed tap blended by dark mix
- [ ] **DSP-03**: Re-enabling Dark after bright operation emits no stale frozen burst
- [ ] **DSP-04**: Bright/dark automation remains finite and click-bounded
- [ ] **DSP-05**: LFO modulation depth is invariant in seconds across sample rates
- [ ] **DSP-06**: ProperSRC output is pre-cleared
- [ ] **DSP-07**: ProperSRC unwritten samples remain zero
- [ ] **DSP-08**: Existing ProperSRC imaging/HF gates remain green
- [ ] **DSP-09**: Wet dirt implements the 100 Hz pre-clip high-pass
- [ ] **DSP-10**: Wet dirt implements a 20 Hz post-clip DC blocker
- [ ] **DSP-11**: Wet dirt long-run DC offset is below the defined gate
- [ ] **DSP-12**: Dry path remains unaffected by all wet filtering
- [ ] **DSP-13**: `dirt_os` stays disabled and unimplemented
- [ ] **DSP-14**: `authentic_color` remains off by default
- [ ] **DSP-15**: All factory presets keep `authentic_color=0`

### State, Presets, UI, and Branding (`UX`)

- [ ] **UX-01**: Parameter IDs remain unchanged
- [ ] **UX-02**: Default `send_amount` becomes 0
- [ ] **UX-03**: Factory preset XML and `FactoryPresets.cpp` remain identical in recalled state
- [ ] **UX-04**: Pressure presets use connected mode and zero resting pressure
- [ ] **UX-05**: Always-on presets use disconnected mode
- [ ] **UX-06**: UI explains Pressure Mode without third-party controller naming
- [ ] **UX-07**: No product-facing source string contains third-party product/brand/controller names
- [ ] **UX-08**: No shipping resource filename contains those names
- [ ] **UX-09**: Procedural fallback says `SENDBLOOM`, not the referenced product name
- [ ] **UX-10**: Exact reference faceplate asset is removed from the shipping binary unless written permission and a separate legal decision are recorded
- [ ] **UX-11**: A Niko-approved original SendBloom faceplate is the production asset, or the original procedural faceplate ships (`human_needed` for asset approval)
- [ ] **UX-12**: Legal metadata scan normalizes punctuation/spacing/case and scans filenames
- [ ] **UX-13**: `design-qa.md` contains portable repository-relative paths and current evidence
- [ ] **UX-14**: Editor hotspots and state overlays remain hittable and correctly aligned after asset replacement
- [ ] **UX-15**: Existing preset sessions are explicitly classified as pre-v1 development state; no hidden migration promise is invented
- [ ] **UX-16**: README and clean-room docs describe only verified behavior

### Reference and Claim Evidence (`REF`)

- [ ] **REF-01**: Add a reproducible reference-capture protocol
- [ ] **REF-02**: Add tooling to measure predelay, decay, spectral centroid, gate envelope, harmonic ratios, and DC offset
- [ ] **REF-03**: Store derived metrics with capture metadata and knob positions
- [ ] **REF-04**: Never commit third-party firmware, EEPROM, bytecode, schematics, or proprietary dumps
- [ ] **REF-05**: Hardware recordings, if made, are user-created audio captures only
- [ ] **REF-06**: Compare at least five Size positions in bright and dark modes if hardware is available (`human_needed` if no hardware)
- [ ] **REF-07**: Compare at least five Input and Distn combinations if hardware is available (`human_needed` if no hardware)
- [ ] **REF-08**: Compare pre/post gate timing if hardware is available (`human_needed` if no hardware)
- [ ] **REF-09**: Compare controller press/release behavior if hardware is available (`human_needed` if no hardware)
- [ ] **REF-10**: Niko performs a blind or level-matched listening review (`human_needed`)
- [ ] **REF-11**: Closeout assigns one ADR-V1-17 fidelity status
- [ ] **REF-12**: Public copy matches the assigned status

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

Which phases cover which requirements. Filled during roadmap creation.

| Requirement | Phase | Status |
|-------------|-------|--------|
| BASE-01 … BASE-08 | Phase 19 | Pending |
| SEND-01 … SEND-14 | Phase 20 | Pending |
| UX-01 … UX-05 (state/preset resting) | Phase 20 | Pending |
| RT-01 … RT-15 (span/bypass foundation) | Phase 21 | Pending |
| CORE-14 … CORE-18 (true bypass) | Phase 21 | Pending |
| MIDI-01 … MIDI-10 | Phase 22 | Pending |
| RT-04 … RT-07 (per-sample delivery / MIDI spans) | Phase 22 | Pending |
| CORE-01 … CORE-13 | Phase 23 | Pending |
| DSP-01 … DSP-15 | Phase 24 | Pending |
| UX-06 … UX-16 | Phase 25 | Pending |
| REF-01 … REF-12 | Phase 26 | Pending |
| REL-01 … REL-20 | Phase 27 | Pending |

**Coverage:**
- v1 requirements: 128 total (BASE 8 + SEND 14 + MIDI 10 + RT 15 + CORE 18 + DSP 15 + UX 16 + REF 12 + REL 20)
- Mapped to phases: 128 (provisional master map; roadmapper must validate 100%)
- Unmapped: 0

---
*Requirements defined: 2026-07-12*  
*Last updated: 2026-07-12 after milestone v1.0 requirements import*
