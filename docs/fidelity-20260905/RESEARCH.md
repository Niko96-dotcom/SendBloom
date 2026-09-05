# Reverb-X fidelity investigation — 2026-09-05

Target: SendBloom at `95a3012b69ab7c0da5225b5ed422131debac02a2`, initially clean.
The original is the **Rainger FX Reverb-X mini-pedal with Igor**, the five-knob,
Dark/Gate-switch mono pedal in the current manufacturer page and linked manual.
Neither identifies a PCB/firmware revision. The manual URL upload is not a
revision number. No evidence found for a distinct Reverb-X Mk II or revised
algorithm; that is a search limitation, not proof all production units match.
The related **Echo-X** has delay-specific behavior and a documented added Gate
option; its revision history is not transferred to Reverb-X. No public product
schematic, service document, firmware or relevant patent was found.

Sources and retrieval limits are in [SOURCES.jsonl](SOURCES.jsonl). Primary
product page/manual were read again in this run. Spin's Effects direct retrieval
failed repeatedly; search-index text from the original site corroborates its
architecture alternatives. Its demo-board and oscillator material explicitly
allow other clocks: FV-1 attribution does not establish this pedal's clock.

## Evidence map before implementation

| Behavior | Current implementation | Evidence and classification | Decision |
|---|---|---|---|
| Dry/Level/Output | Clean pre-input-gain mono sum, independent wet level, master output, PDC-aligned dry | Manual supports dry remaining present, wet-only dirt and overall output. Exact input control's dry gain and extreme overload behavior are ambiguous across FAQ/manual. PDC is plugin adaptation. | Preserve; no new gain calibration claimed. |
| Input/gate sensitivity | -9/0/+9 dB input map; bounded input soft clip; detector before send | Input changes sensitivity is sourced. Voltage scale, taper, -3dBFS ceiling and LED thresholds are engineering approximations. | Preserve gain map. |
| Gate Pre/Post | One detector/hold/ramp, gate moved across wet processing | Movable gate and fixed abrupt closing are sourced. 1/2ms detector, 5ms hold, 0.2/0.75ms edges, -45dBFS threshold and 2dB hysteresis are unmeasured choices. | Preserve timing and gate gain law. |
| Igor Pre | Send stops new excitation while tank keeps decaying | Sourced endpoint; software firm/soft curves and 3/25ms smoother are approximations | Preserve byte-identical output. |
| **Igor Post** | **Unsent dry input holds the gate open even after release** | **R1 describes reverb cut short with Gate in. Reproducible production test contradicts that endpoint.** | **Key the Post detector from pressure-attenuated input.** |
| Reverb size | 1.2–6s exponential target; four-block single-allpass ring | Maximum supported; floor, taper, topology, density, loop length and RT60 calibration unmeasured. | Preserve accepted voicing. |
| Dark/Bright | Dark adds 55ms predelay and stronger HF damping | Relative distinction is sourced; numerical timing/cutoffs unmeasured | Preserve. |
| FV-1 class | 32,768Hz float tank with ProperSRC; 31,532-word nominal delay budget | Chip attribution is secondary; Spin's architecture and default crystal are not product firmware/circuit proof. | Do not infer exact fidelity from RAM/clock compliance. |
| Modulation | 0.48/0.60Hz delay modulation, chosen depths | Vendor class guidance only; no product measurement | Preserve; no pitch-tracking subsystem is indicated. |
| Dirt | Fixed reciprocal curve, asymmetric small-signal slopes, HP/LP filters, blended clean wet | Wet-only fixed drive/blend are sourced; reciprocal curve, drive, asymmetry, filters and makeup are not measured analog Reverb-X parameters. | Preserve; previous drive retune was rejected in human listening. |
| Noise/headroom | No modeled circuit noise; input clamp and bounded loop | Actual noise spectrum/headroom/voltage calibration unknown | Do not add guessed hiss/hum or tune from compressed demos. |
| Stereo/bypass | Dual mono, optional extended stereo, latency-aligned bypass | Hardware is mono/true bypass; stereo and host PDC are software extensions | Preserve IDs/state and normal host contract. |

The repository's earlier density/drive candidate has explicit **rejected** status
in the public-reference report: the baseline was preferred in all four listening
cells. Favorable kurtosis is not permission to revive that rejected sound.

## Accepted hypothesis and falsifying tests

The manufacturer describes release-dependent Post muting even with continuing
playing. The smallest implementation is to change the Post detector key from
pre-send input to sent input. Pre still sees pre-send input; the existing smooth
placement depth interpolates between the two. This is a **behavioral inference**,
not a traced circuit node. Input-dependent partial-pressure gating and exact
release timing remain unmeasured. No new thresholds or pressure curve are tuned.

Before modification, render full production DSP twice over 10 inputs × 6 settings.
Retain the original binary, source HEAD archive, renderer source, CMake cache,
parameter JSON, input provenance/hashes and output hashes. Freeze these criteria:

- Post output must null against PDC-aligned dry by 200ms after release while a
  220Hz tone continues. This is a conservative software criterion, not a hardware
  timing claim. Test 44.1/48/96kHz, 64/511 samples, both feels and host/MIDI control.
- Pre must retain nonzero wet energy in the same window.
- All non-pressure and Pre cells must remain byte-identical to baseline.
- Full production builds/tests, C++ allocation guard and pluginval must pass;
  CPU checked with alternating existing benchmark runs.
- No retuning of presets, tank, filters, nonlinearities, input/output gain or IDs.

## Audio references and their limits

No sample-aligned Reverb-X DI/processed stems were found. The existing
[public catalogue](../reverb-x-public-reference-catalog.md) remains useful, but
its historical video timestamps were **not re-auditioned** in this run: the
video tool fetches failed. No audio from these videos was downloaded or fitted.

- [Get Offset](https://getoffsetpodcast.com/rainger-fx-reverb-x/): primary creator
  transcript/page retrieved; Squier Paranormal Offset Tele through Strymon Iridium
  Round B. Historical 17:49–17:58 Post/Igor segment is a review lead. Live phrases,
  cabinet simulation, input changes and recording gain prevent numerical fitting.
- [Knobs](https://www.youtube.com/watch?v=QrPpxCW4EzI): manufacturer-linked
  qualitative demonstration; historical catalogue points to 4:15–4:43 Igor.
  No paired DI or calibrated knob positions; fetch failed this run.
- [TUNNEL OF REVERB](https://www.youtube.com/watch?v=iagH2FIFI6A): historical
  catalogue identifies reamped acoustic and drums but no labeled DI stem; its
  Dark section includes ADG-1. No new observation claimed after failed retrieval.

Real local DI is used only as **test excitation**, not as a hardware reference:
Guitar-TECHS P3 open/dense chords, fast strums and slow arpeggio (CC BY 4.0;
[dataset](https://zenodo.org/records/14963133)); FreePats FSBS soft E2/E3/E4
(CC0 per local provenance). Input byte hashes match the existing acquisition
manifest. No pickup/amp difference is scored as a pedal difference. Lead-in and
tail silence are added without normalizing input gain; generated chord, tone,
quiet decay, transients and impulse make levels explicit. See artifact corpus.json
for each capture chain, trim, file/inode/hash, and settings.

## Smallest resolving capture

A single 10-second mono direct recording plus the exact simultaneously captured
DI would settle the key uncertainty. Keep a steady reamped 220Hz tone (or looped
DI chord) playing; Level/Size noon, Distn minimum, Input set below overload, Gate
Post, Bright. Hold Igor, release it while source continues, press again. Include
one held halfway-pressure plateau. Capture at 48kHz/24-bit with no amp/cab,
normalization, noise reduction or limiter; note serial/PCB revision if available,
knob photo, interface/reamp gains and a voltage calibration tone. The full-release
segment tests closure time, the partial plateau distinguishes a sent-input key
from a separate pressure mute, and repress reveals whether tank state survives.
Repeat once in Pre if another ten seconds are available. A remote owner can
record this; ownership of hardware by the developer is not necessary.
