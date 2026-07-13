# SendBloom 1.0.0-rc0 Candidate Release Notes

This candidate establishes truthful Pressure Mode rest/press/release behavior, sample-positioned MIDI CC1 control, bounded realtime span processing, channel-preserving true bypass, corrected Input/Level/Gate behavior, reverb continuity fixes, original SendBloom branding, and an `original-inspired` fidelity classification.

The current remediation worktree is not a public RC release yet. CI on the exact eventual candidate, DAW smoke/soak, JUCE commercial entitlement, Developer ID signing, notarization, and correction of the stale pre-existing RC tag remain open.

The customer-facing 32k Color parameter has been removed. SendBloom now uses one permanently enabled, bandlimited 32,768 Hz ProperSRC reverb path at every DAW sample rate; old beta state containing `authentic_color` is tolerated and ignored. Speculative 9-bit parameter quantization is not part of the production path. Extended Stereo remains available in Advanced, and the unimplemented Dirt OS control remains outside the shipping parameter contract. Hardware comparison is still deferred.
