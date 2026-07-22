# SendBloom UI realism research

Research date: 2026-07-22

## What premium hardware-style interfaces actually do

- Softube explicitly separates a **true-life-rendered** Suite view from a more
  schematic modular Studio view. The lesson for SendBloom is to keep the main
  pedal physically coherent while letting the Advanced drawer be the denser,
  software-native surface.
- Softube's Vintage Amp Room describes its complete setups as photorealistic 3D
  renders and maps the visible amp controls directly to the real front panel.
  Our equivalent is the single procedural Cycles scene plus invisible JUCE hit
  targets, rather than independently illustrated widgets.
- UAD pairs hardware-faithful faces with software precision: live values on
  hover, fine adjustment, mouse-wheel control, default reset, preset browsing,
  bypass/A-B utilities, and scalable presentation. Realism is therefore not an
  excuse to hide state or make a control imprecise.
- BOSS compact pedals and the MXR Phase 90 show the durable stompbox vocabulary:
  a small number of strongly silhouetted controls, readable printed legends,
  sparse position markings, rugged moulded knobs, and generous separation from
  the foot control.

## Applied to this pass

1. Constrain the control family with real moulded-knob proportions rather than
   hybridising visual cues from unrelated pedals. A Davies 1510-style reference
   is about 19 mm diameter by 14.5 mm high and includes a radial set screw.
2. Replace the broad reflective crown with a recessed, perfectly planar,
   high-roughness top and long ivory paint index.
3. Give both controls a genuinely vertical, pinchable barrel with broad moulded
   lands, narrow axial grooves, a flat cap and a visible chrome set screw.
4. Seat every control on a stationary nickel washer and bake sparse
   scale markings plus contact shadows into the same faceplate scene.
5. Keep every moving angle as a path-traced filmstrip frame so the room lighting
   does not rotate with the knob.
6. Keep exact values on hover, add a physical vertical-drag cursor, mouse-wheel
   adjustment, tooltips, keyboard focus, and double-click reset.
7. Remove the exaggerated label bevel so dynamic text reads as screen print on
   the rendered enclosure.
8. Replace the cream insert with procedural black wrinkle powder and ivory
   pad-print; the powder signal affects bump, roughness and albedo rather than
   acting as a flat colour layer.
9. Use the Hammond 1590B's published 60.5 mm width, 31 mm overall depth and
   4.19 mm lid thickness as the dimensional scale. At the 345 px plate width,
   the enclosure is about 177 render units deep—not the rejected 36-unit shell.
10. Pitch the orthographic camera 13.5 degrees and counter-scale the z=0 plane
    in world Y so the full physical sidewall becomes visible without moving the
    established JUCE layout or hit targets.
11. Build each cable connection as a mechanical chain: side-wall cut-out,
    insulating bezel, chrome hex facia, threaded socket nose, inserted 6.35 mm
    shaft, diecast right-angle shell, retained cover screws, ribbed strain relief,
    then a 5–7 mm cable rising from the bench to the jack centre.
12. Replace decorative silver marks with cause-based, layered damage: raised
    coating lip, oxide/primer, smaller aluminium core. Grooves get a dark recess
    and a narrow light-facing metal shoulder; wear clusters at edges, sockets,
    screws and shoe-contact areas.
13. Add a low frontal studio kicker so the deep near wall and chrome retain
    reflected shape instead of collapsing to featureless black.
14. Treat the brand mark as hardware, not composited artwork: import the exact
    `brand_logo.svg` silhouettes, model the nickel rim, recessed black bed and
    orange inlay separately, then extrude every Niko/FX glyph one physical
    millimetre. Keep the o counter open over a recessed orange insert so inner
    shadows come from geometry without changing the approved wordmark.
15. Apply the same rule to preset furniture. Keep only the selected preset name
    live in JUCE, but build the selector cut-out, nickel bezel, insulating
    gasket, smoked-black carrier, orange arrow inlay and guide rails as solids.
    Layer the LOAD folder and SAVE disk from separate bevelled pieces and extrude both
    legends so their depth, shadows and edge highlights come from the shared
    scene rather than from button-face images.
16. Respect assembly order in every graphic. Screen print goes down before
    hardware is mounted, so visible frame segments need deliberate clearances
    around screw heads, switch sweeps, bezels and the treadle carrier. Audit
    every moving and software-overlay state at 1×; a collision hidden in the
    default pose is still a production defect.
17. Keep secondary hardware in the product's established material hierarchy.
    A real miniature toggle is a layered panel assembly—bore, insulating gasket,
    lock washer, hex nut, threaded bushing, pivot and finger cap—not a polished
    ball on a cone. Let dark phenolic remain the dominant visible surface, use
    aged nickel for narrow structural highlights, and inspect both switch poses
    for believable grip, reflections, contact shadows and printed-label clearance.

## Mechanical reference audit

| Detail | Reference fact | Scene consequence |
|---|---|---|
| Enclosure | Hammond 1590B: 112.4 × 60.5 × 31 mm; 4.19 mm lid | 1.86:1 face ratio and 177-unit top-to-floor depth |
| Socket | Cliff S2: 11.2 mm panel hole, up to 4.6 mm panel, optional nickel/chrome hex nut | Socket hardware crosses the side wall; it is not a top-face prop |
| Plug | Neutrik NP2RX: 14.5 mm all-metal shell, 4–7 mm cable, chuck strain relief | Slim diecast L-shell, threaded/nose transition and ribbed boot |
| Cable | Free The Tone CU-5050: 5 mm cable and cable-clamping strain relief | 5.6-unit radius lead rests on the floor and rises only near the jack |
| Knob | Davies 1510-style: 19 × 14.5 mm with set screw | Tall pinch wall, narrow moulded grooves, flat top and side fastener |
| Full pedal | BOSS compact reference: 73 × 129 × 59 mm including treadle | The foot control and enclosure must project unmistakable height |

## Primary references

- [Softube Amp Room manual](https://www.softube.com/se/user-manuals/amp-room)
- [Softube Vintage Amp Room manual](https://www.softube.com/user-manuals/vintage-amp-room)
- [Universal Audio plug-in manual](https://help.uaudio.com/hc/en-us/articles/5085501350932-Using-UAD-Plug-Ins-Manual)
- [Universal Audio UAFX pedals](https://www.uaudio.com/pages/uafx-pedals)
- [BOSS DS-1 official product page](https://www.boss.info/global/products/ds-1/)
- [MXR Phase 90 official product page](https://www.jimdunlop.com/mxr-phase-90/)
- [Rogan PT-series pointer controls](https://rogancorp.com/products/pt-series-pointer-control-knobs/)
- [Davies 1900-H control knob](https://daviesmolding.com/catalog/series/knobs/control/1900-h/)
- [Kilo aluminium control-knob dimensions](https://www.potentiometers.com/pdf/KiloKnobs.pdf)
- [Hammond 1590B dimensioned drawing](https://www.hammfg.com/files/parts/pdf/1590B.pdf)
- [Neutrik NP2RX-B right-angle plug](https://www.neutrik.com/en/product/np2rx-b)
- [Cliff S2 external-thread jack socket](https://www.cliffuk.co.uk/products/jacksockets/S2.pdf)
- [Free The Tone CU-5050 instrument link cable](https://www.freethetone.com/en/products/detail30/)

## Deliberate boundaries

- No trademarked knob or enclosure is copied exactly.
- The fixed 420 x 780 layout, signal order, orange/ivory brand language and
  pressure-send treadle remain recognisably SendBloom.
- Resizing, a global UAD-style toolbar and A/B state management are valuable
  product features, but are not disguised as visual-realism work. They need a
  separate interaction/state design pass.
