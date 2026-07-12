# Phase 20: Pressure Send State Truth - Research

**Researched:** 2026-07-12
**Domain:** JUCE AU/VST3 pressure-send state machine, pad UI, factory presets, asymmetric send smoothing
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

#### Pressure State Machine (ADR-V1-01)
- `send_connected=false` → always-on wet feed (SEND-01)
- `send_connected=true` + pressure/amount 0 → no new wet input; existing tails continue (SEND-02/04)
- Pressure >0 while connected sends into wet path (SEND-03)
- Release sets `send_amount=0`, leaves `send_connected=true` (SEND-05) — never flip to disconnected on pad release
- Disconnecting restores ordinary always-on reverb (SEND-12); `send_amount` APVTS ID unchanged; default amount 0 (SEND-13)

#### UI Pad & Overlay
- On-screen pad `mouseUp`/`release` keeps connected and zeros amount (fix the Phase 19 pressure-release contract)
- Pressed overlay follows pressure/pressed state, not connection alone (SEND-06)
- Advanced UI exposes persistent Pressure Mode connection (SEND-07)
- Pad press may auto-connect without disconnecting on release (SEND-08)
- Prefer minimal UI surgery to achieve state truth; no visual redesign beyond required state/copy (UX-01…05 as mapped)

#### Presets & Curves
- Factory pressure presets load connected with `send_amount=0`; always-on presets load disconnected (SEND-11, UX-04)
- XML and `FactoryPresets.cpp` recall identical state
- Firm vs Soft remain distinct (SEND-09); attack ~3 ms / release ~25 ms (SEND-10)
- Behavior invariant across host block sizes for pressure semantics (SEND-14) — full oversized wet path may still wait on Phase 21 span if blocked by dry fallback; pressure state semantics must still hold on supported blocks

#### Contract Flip Strategy
- Flip Phase 19 `[v1][contract][pressure-release]` (and related SEND contracts) from intentionally red to green by fixing production behavior — do not delete contracts; update expectations only if ADR wording requires
- Leave unrelated Phase 19 contracts red (oversized, bypass, PostHard, Input, MIDI purity, shipping) for later phases
- Keep BASE-04 discipline for ProperSRC/HF/DryPath/release greens unless a SEND requirement explicitly updates them

### Claude's Discretion
- Exact pad component file edits, advanced UI control placement, and whether SEND-14 full block-size invariance needs a temporary caveat if Phase 21 span is still missing
- Whether to add new green SEND tests vs only flipping existing V1Contract pressure suites

### Deferred Ideas (OUT OF SCOPE)
- Span/no-alloc and true bypass → Phase 21
- MIDI CC1 realtime modulation without APVTS mutation → Phase 22
- Branding / third-party string removal beyond pressure UX copy → Phase 25
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| SEND-01 | `send_connected=false` → always-on wet | `PressureSend::computeGain` / snapshot / `PressureController` returns 1.0 when disconnected |
| SEND-02 | connected + pressure 0 → no new wet | Connected-at-rest gain 0; energy tracker pattern from V1Contract |
| SEND-03 | pressure >0 feeds wet | Pad/amount → controller → `PressureSend::process` |
| SEND-04 | release stops new wet; tails continue | Zero amount, keep connected; do not reset tank |
| SEND-05 | mouseUp zeros amount, keeps connected | Fix `PressureSendPad::mouseUp`; flip `[pressure-release]` |
| SEND-06 | overlay follows pressed/pressure | Fix `PedalFaceplatePaint::drawStateOverlays` |
| SEND-07 | Advanced exposes connection | Add `PRESSURE MODE` toggle on `AdvancedDrawer` |
| SEND-08 | pad auto-connect, no disconnect on release | Keep `mouseDown` connect; remove disconnect on `mouseUp` |
| SEND-09 | Firm vs Soft distinct | Keep `ParameterCurves::sendGain` exponents 1.85 / 1.2 |
| SEND-10 | attack ~3 ms / release ~25 ms | New asymmetric smoother in `PressureController` (not symmetric `SmoothedValue`) |
| SEND-11 | pressure presets rest at amount 0 | Edit XML matrix; rebuild BinaryData |
| SEND-12 | disconnect → always-on | Existing disconnected gain=1.0 path |
| SEND-13 | `send_amount` ID unchanged; default 0 | Keep `ParameterIDs::sendAmount`; change layout default |
| SEND-14 | block-size invariant pressure semantics | Test ≤ `preparedMaxBlock_`; caveat oversized dry fallback → Phase 21 |
| UX-01 | Parameter IDs unchanged | Do not rename `send_*` IDs |
| UX-02 | Default `send_amount` = 0 | `ParameterLayout.cpp` |
| UX-03 | XML ↔ FactoryPresets recall identical | Already XML-backed; keep program/XML parity tests green |
| UX-04 | Pressure presets connected + amount 0 | Firm Pressure, Hot Clip, Dry Dub Sends |
| UX-05 | Always-on presets disconnected | Remaining five presets `send_connected=0` |
</phase_requirements>

## Summary

Phase 20 fixes SendBloom’s signature Pressure Mode lie: the on-screen pad currently **disconnects on `mouseUp`**, which restores always-on wet feed (`sendGain = 1.0`) instead of connected-at-rest dry feed (`sendGain = 0`) with decaying tails. Factory pressure presets also load with non-zero `send_amount`, and the faceplate “pressed” overlay tracks connection rather than press. DSP already has the right *static* gain equation in `PressureSend` / `ParameterSnapshot`, but smoothing is a single symmetric 25 ms `SmoothedValue` on **gain after** the Firm/Soft curve — ADR-V1-04 requires asymmetric **3 ms attack / 25 ms release on raw pressure before the curve**.

**Primary recommendation:** Implement milestone §9 in three plans — (1) `PressureController` + layout default + processor wiring, (2) pad/overlay/Advanced connection UI, (3) preset XML matrix + green `[pressure-release]` proof and SEND coverage — without touching Phase 21 span or Phase 22 MIDI purity.

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Pressure connection / amount APVTS state | API / Backend (processor params) | Browser / Client (editor) | Host-visible state lives in APVTS; UI only writes via message-thread gestures |
| Effective send gain (connected/rest/press) | API / Backend | — | Audio-thread `PressureController` owns combination + asymmetric smooth + curve |
| Pad press/release gestures | Browser / Client (editor) | — | `PressureSendPad` mouse handlers on message thread |
| Pressed overlay / Advanced toggle | Browser / Client | — | Faceplate paint + `AdvancedDrawer` |
| Factory preset resting state | Database / Storage (embedded XML) | API / Backend | `resources/presets/*.xml` → BinaryData → `FactoryPresets::applyPreset` |
| Wet energy / tail continuity | API / Backend | — | Reverb tank continues when send gain → 0; no tank clear on release |
| Oversized-block wet continuity | API / Backend | — | **Out of scope** — dry fallback remains until Phase 21 |
| MIDI pressure without APVTS write | API / Backend | — | **Out of scope** — Phase 22; MIDI target stubbed to 0 in Phase 20 |

## Exact Bug Loci (verified)

### 1. Pad `mouseUp` disconnects — SEND-05 / `[pressure-release]` RED

```39:46:source/ui/PressureSendPad.cpp
void PressureSendPad::mouseUp (const juce::MouseEvent&)
{
    isPressed = false;
    setConnected (false);   // BUG: flips to always-on wet (gain=1)
    startBloomFade();
    if (auto* parent = getParentComponent())
        parent->repaint();
}
```

**Required:** set `send_amount` to 0 (with gesture begin/end), **do not** write `send_connected`. Keep visual bloom fade (200 ms) display-only.

Observed Phase 19 / current failure: `connectedAfter > 0.5f` → `0.0f` after `mouseUp` `[VERIFIED: Builds/Tests "[pressure-release]"]`.

### 2. Default amount is full wet pressure — UX-02 / ADR-V1-01

```47:49:source/ParameterLayout.cpp
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { sendAmount, 1 }, "Send Amount",
        juce::NormalisableRange<float> { 0.0f, 1.0f }, 1.0f));  // BUG: must be 0.0f
```

### 3. Overlay follows connection — SEND-06

```256:264:source/ui/PedalFaceplatePaint.cpp
    const auto sendConnected = isParamOn (apvts, ParameterIDs::sendConnected);
    ...
    if (sendConnected)
        drawFootswitchPressedOverlay (g);
```

**Required:** draw when `send_amount > ~0.001f` **or** pad `isPressed()` (expose getter). Connection needs its own Advanced toggle indicator, not the pressed overlay.

### 4. Advanced drawer has Feel but no connection — SEND-07

`AdvancedDrawer` exposes `send_feel` only. No `send_connected` / `PRESSURE MODE` control `[VERIFIED: source/ui/AdvancedDrawer.cpp]`.

### 5. Preset resting state wrong — SEND-11 / UX-04/05

| Preset | Current `send_connected` | Current `send_amount` | Required (milestone §9.7) |
|--------|--------------------------|------------------------|---------------------------|
| Sparkle Verb | 0 | 0.8 | 0 / 0 (always-on) |
| Cut Sample Gate | 0 | 0.7 | 0 / 0 |
| Spacerock Burn | 0 | 0.9 | 0 / 0 |
| Dry Dub Sends | 0 | 0.6 | **1 / 0** (pressure) |
| Dark Bloom | 0 | 0.75 | 0 / 0 |
| Firm Pressure | 1 | **0.85** | **1 / 0** |
| Gated Room | 0 | 0.7 | 0 / 0 |
| Hot Clip | 1 | **1.0** | **1 / 0** |

`FactoryPresets.cpp` does **not** hardcode values — it only maps names → BinaryData XML. UX-03 means keep program-load ↔ XML-parse parity; edit `resources/presets/*.xml` and rebuild `SendBloomPresets` BinaryData `[VERIFIED: source/FactoryPresets.cpp, CMakeLists.txt juce_add_binary_data]`.

### 6. Curve / attack / release locations — SEND-09 / SEND-10

| Concern | Location | Current | Required |
|---------|----------|---------|----------|
| Firm/Soft curve | `ParameterCurves::sendGain` | `pow(smoothstep(a), 1.85\|1.2)` | Keep distinct `[VERIFIED]` |
| Static connect math | `PressureSend::computeGain` | disconnected→1, connected→curve | Keep |
| Snapshot instantaneous gain | `ParameterSnapshot::capture` | curve then bank | Phase 20: feed **raw** amount/connected/feel into controller |
| Smoothing | `SmoothedParameterBank::sendGain.reset(..., 0.025)` | **symmetric 25 ms on gain** | Replace with asymmetric **3 ms / 25 ms on pressure before curve** (ADR-V1-04) |
| Visual fade only | `PressureSendPad` timer `kBloomFadeMs = 200` | displayAmount | Keep visual-only; not DSP release |

### 7. Always-on vs connected gain (already correct statically)

```10:16:source/PressureSend.h
    static float computeGain (float amountNorm, bool sendConnected, bool firmFeel) noexcept
    {
        if (! sendConnected)
            return 1.0f;
        return ParameterCurves::sendGain (amountNorm, firmFeel);
    }
```

```64:66:source/ParameterSnapshot.h
        s.sendGain = s.sendConnected
                       ? ParameterCurves::sendGain (s.sendAmountNorm, s.sendFirmFeel)
                       : 1.0f;
```

## Phase 19 Contracts — Which Flip Green

| Filter | Phase 20? | Notes |
|--------|-----------|-------|
| `[v1][contract][pressure-release]` | **GREEN target** | Fix pad + resting semantics; do not delete |
| `[oversized-block]` | leave RED | Phase 21 dry fallback |
| `[true-bypass]` | leave RED | Phase 21 |
| `[posthard]` | leave RED | Phase 23 |
| `[input-anchors]` | leave RED | Phase 23 |
| `[midi-apvts]` | leave RED | Phase 22 (`sendParam->store` still present) |
| `[shipping-policy]` | leave RED | Phase 25 |

BASE-04 greens (`[release]`, `[DryPath]`, `[ProperSRC]`, `[HF]`, ENAB) must stay green.

## Standard Stack

### Core

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| JUCE | 8.0.12 (submodule pin) | Plugin/UI/APVTS/SmoothedValue | Project pin `[VERIFIED: 19-BASELINE.md]` |
| Catch2 | 3.8.1 (CPM) | Unit/contract tests | Existing harness `[VERIFIED: cmake/Tests.cmake]` |
| CMake | ≥3.30 (host 3.30.3) | Build/Tests | Existing |

### Supporting (in-tree, no new packages)

| Component | Purpose | When to Use |
|-----------|---------|-------------|
| `PressureSend` | Multiply wet input by send gain | Keep as final multiply |
| `PressureController` (**new**) | Host(+MIDI stub) max → asymmetric smooth → curve → gain | Phase 20 DSP owner |
| `ParameterCurves::sendGain` | Firm/Soft | Unchanged math |
| `EnergyTrackingReverb` pattern | Wet-feed assertions | Extend from V1Contract |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| `PressureController` | Only fix pad + presets | Leaves SEND-10 wrong (symmetric gain smooth) — reject per ADR-V1-04 |
| Dual `SmoothedValue` attack/release | Custom one-pole `AsymmetricSmoother` | Dual SmoothedValue still awkward for direction changes; custom one-pole matches ADR text |
| Soft Pressure factory preset | `send_feel` Soft via Advanced | No Soft factory preset exists; Firm/Soft is feel choice — do not invent Soft Pressure XML |

**Installation:** none — no new external packages. Rebuild after XML edits:

```bash
cmake --build Builds --config Release --target Tests
Builds/Tests "[pressure-release]"
Builds/Tests "[send]"
```

## Package Legitimacy Audit

| Package | Registry | Age | Downloads | Source Repo | Verdict | Disposition |
|---------|----------|-----|-----------|-------------|---------|-------------|
| — | — | — | — | — | — | No new packages for Phase 20 |

**Packages removed due to [SLOP] verdict:** none  
**Packages flagged as suspicious [SUS]:** none  

Catch2 remains the existing CPM dependency (not an npm install). Do not add npm packages for this C++ phase.

## Architecture Patterns

### System Architecture Diagram

```text
[Pad mouseDown/Drag/Up] ──setValueNotifyingHost──► APVTS
        │                                            │
        │ auto-connect                                │ send_connected
        │ amount from Y / zero on up                   │ send_amount
        ▼                                            ▼
[Advanced PRESSURE MODE toggle] ──────────────────► APVTS
                                                     │
                              ParameterSnapshot.capture (raw fields)
                                                     │
                                                     ▼
                                         PressureController (audio thread)
                                         ┌─────────────────────────────┐
                                         │ if !connected → gain=1.0    │
                                         │ raw = max(host, midi=0)     │
                                         │ asym smooth 3ms / 25ms      │
                                         │ gain = sendGain(raw, firm)  │
                                         └─────────────────────────────┘
                                                     │
                                                     ▼
                                         PressureSend::process(wet, gain)
                                                     │
                                                     ▼
                                              Reverb tank / tails
```

MIDI CC1 → APVTS `store` still exists (Phase 22 debt). Phase 20 leaves `midiPressureTarget = 0` inside controller.

### Recommended Project Structure

```text
source/
├── PressureController.h     # NEW — prepare/reset, setConnected/Host/Midi/FirmFeel, processSample
├── PressureSend.h           # KEEP — multiply helper + unit-testable computeGain
├── ParameterLayout.cpp      # default send_amount → 0
├── ParameterSnapshot.h      # expose raw send fields; stop treating bank sendGain as pressure truth
├── SmoothedParameterBank.h  # remove or bypass sendGain smoother once controller owns path
├── PluginProcessor.cpp/.h   # own PressureController; call per sample / block
└── ui/
    ├── PressureSendPad.*    # release-to-zero; public isPressed/getDisplayAmount; gestures
    ├── AdvancedDrawer.*     # PRESSURE MODE toggle → send_connected
    └── PedalFaceplatePaint.*# overlay from amount/pressed
resources/presets/*.xml      # §9.7 matrix
tests/
├── V1ContractPressureReleaseTest.cpp  # flip green (no delete)
├── PressureSendTest.cpp               # keep + extend curves/controller
└── (optional) V1ContractPressure* / preset resting tests
```

### Pattern 1: Pad release-to-zero (ADR-V1-02)

**What:** `mouseDown` may connect + begin amount gesture; `mouseUp` zeros amount and ends gesture; never disconnects.  
**When to use:** All on-screen pressure edits.  
**Example:**

```cpp
// Source: ADR-V1-02 + https://docs.juce.com/master/classjuce_1_1AudioProcessorParameter.html
void PressureSendPad::mouseUp (const juce::MouseEvent&)
{
    isPressed = false;
    if (amountParam != nullptr)
    {
        amountParam->beginChangeGesture();
        amountParam->setValueNotifyingHost (0.0f);
        amountParam->endChangeGesture();
    }
    // do NOT call setConnected(false)
    startBloomFade();
    if (auto* parent = getParentComponent())
        parent->repaint();
}
```

### Pattern 2: Asymmetric pressure smoothing (ADR-V1-04)

**What:** Smooth normalized pressure with separate attack/release coeffs, then apply Firm/Soft curve.  
**When to use:** Every connected sample while producing send gain.  
**Anti-pattern:** `sendGain.reset(sampleRate, 0.025)` symmetric gain ramp.

### Anti-Patterns to Avoid

- **Disconnect-on-release:** restores always-on wet — the Phase 19 defect.
- **Symmetric `SmoothedValue` for pressure:** violates SEND-10 / ADR-V1-04 `[CITED: docs.juce.com SmoothedValue + milestone ADR-V1-04]`.
- **Overlay keyed on `send_connected`:** stuck “pressed” while resting dry.
- **Fixing MIDI APVTS mutation in Phase 20:** steals Phase 22 scope; leave `[midi-apvts]` red.
- **Implementing span/no-alloc for SEND-14:** Phase 21; use caveat instead.
- **Deleting failing contracts:** flip by production fix only.
- **Renaming parameter IDs:** UX-01 / ADR-V1-01 locked.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Host automation gestures | Custom host notify | `beginChangeGesture` / `setValueNotifyingHost` / `endChangeGesture` | Host recording semantics `[CITED: docs.juce.com AudioProcessorParameter]` |
| Firm/Soft mapping | New curve invent | `ParameterCurves::sendGain` | Already tested distinct |
| Preset embedding | Parallel C++ value tables | XML + BinaryData + `FactoryPresets::applyPreset` | Single source of truth |
| Wet-feed proof | Subjective listen-only | Energy-tracking `IReverbEngine` mock (V1Contract pattern) | Deterministic SEND-02/04/05 |
| Catch2 runner | New framework | Existing `Builds/Tests` + tags | Phase 19 harness |

**Key insight:** The static gain equation is already right; the lie is UI/preset resting state plus wrong smoothing stage/order.

## Common Pitfalls

### Pitfall 1: Zeroing amount without gesture bracketing
**What goes wrong:** Hosts miss automation endpoints; flaky host automation.  
**How to avoid:** Pair begin/end around press→drag→release amount edits `[CITED: JUCE AudioProcessorParameter]`.  
**Warning signs:** DAW automation records only discrete jumps.

### Pitfall 2: Smoothing gain after curve
**What goes wrong:** Attack/release timings fail SEND-10 tolerances (±25% / ±20%).  
**How to avoid:** Smooth raw pressure, then curve inside `PressureController`.  
**Warning signs:** Soft/Firm timing tests measure post-curve gain ramps.

### Pitfall 3: Clearing reverb on release
**What goes wrong:** SEND-04 fails; tails die.  
**How to avoid:** Only change send gain; never `reset()` tank on amount→0.  
**Warning signs:** Energy at reverb input is 0 (good) but wet output snaps silent (bad).

### Pitfall 4: Claiming SEND-14 complete while oversized dry fallback exists
**What goes wrong:** False green on blocks `> preparedMaxBlock_`.  
**How to avoid:** Document caveat; test 1…prepared sizes; leave oversized to Phase 21.  
**Warning signs:** `numSamples > preparedMaxBlock_` path forces `wet = 0.0f` `[VERIFIED: PluginProcessor.cpp:215-274]`.

### Pitfall 5: MIDI mutation masks resting-amount tests
**What goes wrong:** CC1 still writes APVTS; “resting” amount becomes non-zero mid-test.  
**How to avoid:** Pressure tests avoid CC1; leave MIDI purity red until Phase 22.  
**Warning signs:** `[midi-apvts]` unexpectedly green without Phase 22 work.

### Pitfall 6: BinaryData stale after XML edit
**What goes wrong:** Runtime presets still show 0.85/1.0.  
**How to avoid:** Rebuild `SendBloomPresets` / `Tests` after XML changes.  
**Warning signs:** `PresetTest` / ReleaseTruth XML parity fail or still load old amounts.

## Code Examples

### PressureController shape (milestone §9.5)

```cpp
// Source: .planning/MILESTONE-SPEC-v1.0-interaction-truth.md §9.5 / ADR-V1-04
class PressureController
{
public:
    void prepare (double sampleRate) noexcept;
    void reset() noexcept;
    void setConnected (bool connected) noexcept;
    void setHostPressureTarget (float normalized) noexcept;
    void setMidiPressureTarget (float normalized) noexcept; // Phase 20: always 0
    void setFirmFeel (bool firm) noexcept;
    float processSample() noexcept; // returns send gain
};
// connected? max(host,midi) → asymmetricSmooth → ParameterCurves::sendGain
// !connected → 1.0f
```

### Overlay truth (ADR-V1-02)

```cpp
// Prefer: amount > epsilon || pad.isPressed()
// Fallback if paint cannot see pad: send_amount > 0.001f
if (pressedOrAmount)
    drawFootswitchPressedOverlay (g);
```

## State of the Art

| Old Approach | Current Approach (Phase 20) | When Changed | Impact |
|--------------|----------------------------|--------------|--------|
| Release = disconnect | Release = amount 0, stay connected | ADR-V1-01/02 | Dry-at-rest Pressure Mode |
| Default amount 1.0 | Default amount 0.0 | ADR-V1-01 | Safe init / UX-02 |
| Symmetric 25 ms gain smooth | Asymmetric 3/25 ms pressure smooth | ADR-V1-04 | SEND-10 |
| Overlay = connected | Overlay = pressed/amount | ADR-V1-02 | SEND-06 |
| Pressure presets wet on load | Connected + amount 0 | §9.7 | SEND-11 / UX-04 |

**Deprecated/outdated:**
- Treat `send_connected` as the pad pressed visual.
- Treat `SmoothedParameterBank::sendGain` as the pressure feel implementation.

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| — | (none material) | — | Locked ADRs + codebase verification cover decisions |

Discretion items resolved below as **recommended defaults** (user auto-accepted smart-discuss).

## Open Questions

> **Planning lock (2026-07-12):** All six items remain RESOLVED. Plans 20-01…03 implement the recommendations below (controller in 20-01; pad/overlay/Advanced in 20-02; preset matrix + SEND-14 caveat tests in 20-03). No open blockers.

1. **SEND-14 vs Phase 21 oversized dry fallback — RESOLVED**
   - What we know: `numSamples > preparedMaxBlock_` forces wet=0 `[VERIFIED]`.
   - Recommendation: Assert pressure semantics for blocks ≤ prepared max (e.g. 64/128/256/512). Document temporary SEND-14 caveat for oversized hosts until Phase 21. Do not implement span engine here.
   - **Plan lock:** 20-03 Task 2 + `20-VALIDATION.md` intentional-red table.

2. **New SEND tests vs only flip V1Contract — RESOLVED**
   - Recommendation: Flip `[pressure-release]` by production fix **and** add focused greens for preset resting matrix, attack/release timing, Advanced toggle, overlay predicate. Do not delete Phase 19 contracts.
   - **Plan lock:** 20-01 controller tests; 20-02 flip pressure-release; 20-03 preset matrix.

3. **Dry Dub Sends as pressure preset — RESOLVED**
   - Recommendation: Follow locked milestone §9.7 matrix (`send_connected=1`, `send_amount=0`). Name already implies dry sends.
   - **Plan lock:** 20-03 XML matrix.

4. **Soft Pressure factory preset — RESOLVED**
   - Recommendation: Do not add. Soft remains `send_feel` choice; Firm Pressure stays Firm feel.
   - **Plan lock:** 20-03 forbidden Soft Pressure preset.

5. **PressureController in Phase 20 vs pad-only — RESOLVED**
   - Recommendation: Add `PressureController` now (MIDI target stubbed 0) so SEND-10 is honest; Phase 22 only wires MIDI max + removes APVTS store.
   - **Plan lock:** 20-01.

6. **Overlay access to pad — RESOLVED**
   - Recommendation: Prefer threading `isPressed`/`displayAmount` from editor into paint; if awkward, APVTS `send_amount > 0.001f` is an acceptable interim for SEND-06 (amount is 0 at rest after fix).
   - **Plan lock:** 20-02 Task 2.

## Recommended Plan Split (3 plans)

| Plan | Title | Owns | Exit criteria |
|------|-------|------|---------------|
| **20-01** | PressureController + defaults | `PressureController.h`, processor wiring, `ParameterLayout` default 0, snapshot/bank sendGain handoff, SEND-01…04/09/10/12/13 unit tests | Connected-at-rest dry; disconnected unity; Firm≠Soft; attack/release within tolerance on prepared blocks |
| **20-02** | Pad, overlay, Advanced toggle | `PressureSendPad.*`, `PedalFaceplatePaint.*`, `AdvancedDrawer.*`, `PluginEditor.*` | `[pressure-release]` GREEN; SEND-05/06/07/08 observable |
| **20-03** | Preset matrix + UX + SEND-14 caveat | `resources/presets/*.xml`, BinaryData rebuild, preset/layout tests, optional resting-state contract extensions | UX-01…05; SEND-11; program/XML parity; SEND-14 documented caveat + in-range block tests |

Optional 4th split: peel Advanced toggle into its own plan if UI wave needs parallelization — not required.

## Risks (carry into plans)

| Risk | Impact | Mitigation |
|------|--------|------------|
| **MIDI still mutates APVTS** (`PluginProcessor.cpp` CC1 → `sendParam->store`) until Phase 22 | Host-visible amount can change from MIDI; resting truth polluted; `[midi-apvts]` stays red | Do not “fix” in Phase 20; controller `midiPressureTarget=0`; pressure tests avoid CC1 |
| **Oversized dry fallback** until Phase 21 | SEND-14 incomplete for `numSamples > preparedMaxBlock_` | Caveat in tests/docs; leave `[oversized-block]` red |
| BinaryData not rebuilt after XML | Presets appear unfixed | Plan verify rebuild + preset assertions |
| Dual smoothing (bank + controller) | Double release / wrong timings | Remove/bypass `SmoothedParameterBank` sendGain once controller owns gain |
| Changing BASE-04 / legacy MIDI greens | Regress Phase 19 discipline | Touch only pressure-related production; leave `MidiSendAmountTest` / MIDI ReleaseTruth cases as-is until Phase 22 |

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| CMake | Build | ✓ | 3.30.3 | — |
| `Builds/Tests` | Contract/unit verify | ✓ | Catch2 3.8.1 | Rebuild Tests target |
| JUCE submodule | Plugin/UI | ✓ | 8.0.12 | — |
| Graphify graph | Cross-doc | ✗ | — | Use milestone + codebase (no graph.json) |

**Missing dependencies with no fallback:** none  
**Missing dependencies with fallback:** knowledge graph absent — research used milestone ADR text + source.

Step 2.6: external tools present for this phase.

## Validation Architecture

### Test Framework

| Property | Value |
|----------|-------|
| Framework | Catch2 3.8.1 |
| Config file | `cmake/Tests.cmake` (GLOB `tests/*.cpp`) |
| Quick run command | `Builds/Tests "[pressure-release]"` |
| Full suite command | `Builds/Tests "~[v1]"` plus `Builds/Tests "[v1][contract][pressure-release]"` after fix; keep `[release]`/`[DryPath]` green |

### Phase Requirements → Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| SEND-05 | mouseUp connected-at-rest | contract | `Builds/Tests "[pressure-release]"` | ✅ red → green |
| SEND-01…04 | connect/rest/press/tail | unit/integration | `Builds/Tests "[send]"` (+ extend) | ✅ partial (`PressureSendTest`) |
| SEND-09 | Firm≠Soft | unit | `Builds/Tests "[send][PressureSend]"` | ✅ |
| SEND-10 | 3/25 ms | unit | new controller timing case | ❌ Wave 0 |
| SEND-06/07/08 | overlay/toggle/auto-connect | unit/UI | extend pad/editor tests | ❌ Wave 0 (pad covered partially by contract) |
| SEND-11 / UX-04/05 | preset matrix | unit | extend `PresetTest` / ReleaseTruth | ✅ needs expectation update |
| UX-02 | default amount 0 | unit | extend `ParameterLayoutTest` | ✅ needs new assert |
| UX-01/13 | IDs stable | unit | `ParameterIDsTest` | ✅ |
| SEND-14 | block sizes ≤ prepared | integration | new/tagged cases | ❌ Wave 0 (+ caveat doc) |
| MIDI-03 | APVTS purity | contract | `Builds/Tests "[midi-apvts]"` | ✅ stay RED |

### Sampling Rate

- **Per task commit:** `Builds/Tests "[pressure-release],[send]"`
- **Per wave merge:** above + `Builds/Tests "[preset]"` + `Builds/Tests "[parm][layout]"` + `Builds/Tests "[release]"`
- **Phase gate:** pressure-release green; listed SEND/UX asserts green; other `[v1][contract]` still red as expected; BASE-04 greens intact

### Wave 0 Gaps

- [ ] `PressureController` attack/release timing tests (SEND-10)
- [ ] Preset resting-state asserts for all eight programs (SEND-11 / UX-04/05)
- [ ] `ParameterLayoutTest` default `send_amount == 0` (UX-02)
- [ ] Advanced toggle / overlay predicate tests (SEND-06/07) — optional if contract + manual UI-SPEC acceptance cover
- [ ] SEND-14 in-range block-size cases + documented oversized caveat

## Security Domain

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V2 Authentication | no | — |
| V3 Session Management | no | — |
| V4 Access Control | no | — |
| V5 Input Validation | yes | Clamp pressure norms to [0,1]; bool connect via APVTS |
| V6 Cryptography | no | — |

### Known Threat Patterns for JUCE plugin UI/DSP

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Out-of-range send amount | Tampering | `NormalisableRange` + `jlimit` on pad Y |
| Audio-thread heap from UI path | Denial of Service | Message-thread param writes only; controller allocation-free after prepare |
| MIDI forging APVTS state | Tampering | Deferred Phase 22 — do not expand attack surface in Phase 20 |

## Project Constraints (from .cursor/rules/)

No `.cursor/rules/` directory present in the project root. Follow milestone ADRs, CONTEXT.md, UI-SPEC (minimal state-truth, no redesign), and existing Catch2/`[v1][contract]` discipline from Phase 19.

## Sources

### Primary (HIGH confidence)

- Codebase: `PressureSendPad.cpp`, `ParameterLayout.cpp`, `PedalFaceplatePaint.cpp`, `ParameterSnapshot.h`, `SmoothedParameterBank.h`, `PluginProcessor.cpp`, preset XML, `V1ContractPressureReleaseTest.cpp`
- `.planning/MILESTONE-SPEC-v1.0-interaction-truth.md` — ADR-V1-01/02/04, §9 Phase 20
- `.planning/phases/SENDBLOOM-19-.../19-02-SUMMARY.md` — contract failure map
- Runtime: `Builds/Tests "[pressure-release]"` failed `connectedAfter` as expected

### Secondary (MEDIUM confidence)

- Context7 `/websites/juce_master` — `beginChangeGesture` / `setValueNotifyingHost` / `SmoothedValue.reset` semantics
- `.planning/REQUIREMENTS.md` SEND-01…14, UX-01…05
- `20-UI-SPEC.md`, `20-CONTEXT.md`

### Tertiary (LOW confidence)

- None required for planning blockers

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — pinned JUCE/Catch2 in repo
- Architecture: HIGH — ADR + existing gain equation + verified bug loci
- Pitfalls: HIGH — Phase 19 contracts + oversized/MIDI deferred risks observed in source

**Research date:** 2026-07-12  
**Valid until:** 2026-08-11 (30 days; ADRs locked)
