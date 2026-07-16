#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace sendbloom::ui
{

/** Exact 2x2 box average, alpha-weighted. Use this (not Graphics resampling)
    to derive standard-DPI variants of the 2x renders: JUCE's high-quality
    resampler low-pass filters a 2:1 downscale hard enough to erase the
    ~2 px enamel stipple and film grain the art carries. */
juce::Image boxHalveImage (const juce::Image& source);

/** True when `g` maps logical px to >= 1.5 physical px (a hi-DPI backing
    store), i.e. 2x art will be drawn near 1:1 rather than downscaled. */
bool wantsHiResArt (juce::Graphics& g);

/** Single source of truth for the 420x780 faceplate layout. The painter draws the
    path-traced artwork (tools/render_ui.py) at these rectangles and the editor
    parks its invisible hit-targets on the same ones, so art and interaction
    cannot drift apart.

    The background render fills the editor frame exactly; the usable white
    plate spans x 38..383, y 60..702 with its centreline at x = 210. */
namespace facelayout
{
    constexpr int kFaceCentreX = 210;

    // The visible white plate (inside the chassis lip) after the background crop.
    inline const juce::Rectangle<float> kPlate { 38.0f, 60.0f, 345.0f, 642.0f };
    constexpr float kPlateCornerRadius = 19.0f;

    // Full nameplate incl. its chrome rim (aspect ~4.8:1); sits between the top screws.
    inline const juce::Rectangle<int> kLogo { 85, 70, 250, 52 };

    // Taller than the photographed field was: the vector nameplate is a chunkier
    // 5.5:1, so the rect follows the art rather than squashing it. Kept centred
    // on y=150 so the preset text and the load/save buttons stay put.
    inline const juce::Rectangle<int> kPresetField { 54, 129, 232, 42 };
    inline const juce::Rectangle<int> kPresetText { 70, 134, 186, 32 };
    inline const juce::Rectangle<int> kPresetLoad { 294, 136, 30, 29 };
    inline const juce::Rectangle<int> kPresetSave { 332, 136, 27, 29 };

    // Voice row: three matched knobs, signal order left to right.
    constexpr int kKnobLarge = 84;
    constexpr int kCaptionHeight = 16;
    inline const juce::Rectangle<int> kDistortionKnob { 54, 184, kKnobLarge, kKnobLarge + kCaptionHeight };
    inline const juce::Rectangle<int> kSizeKnob { 168, 184, kKnobLarge, kKnobLarge + kCaptionHeight };
    inline const juce::Rectangle<int> kLevelKnob { 282, 184, kKnobLarge, kKnobLarge + kCaptionHeight };

    // Gain row: input/output pair with the clip lens seated between them.
    constexpr int kKnobSmall = 66;
    inline const juce::Rectangle<int> kInputKnob { 86, 314, kKnobSmall, kKnobSmall + kCaptionHeight };
    inline const juce::Rectangle<int> kOutputKnob { 268, 314, kKnobSmall, kKnobSmall + kCaptionHeight };
    inline const juce::Rectangle<int> kClipLens { 194, 328, 32, 32 };
    inline const juce::Rectangle<int> kClipLabel { 182, 364, 56, 13 };

    // Hardware row: dark-mode button and pre/post gate toggle.
    inline const juce::Rectangle<int> kDarkButton { 84, 438, 74, 74 };
    inline const juce::Rectangle<int> kGateTitle { 262, 414, 76, 14 };
    inline const juce::Rectangle<int> kGatePre { 262, 430, 76, 12 };
    inline const juce::Rectangle<int> kGateSwitch { 272, 444, 56, 62 };
    inline const juce::Rectangle<int> kGatePost { 262, 508, 76, 12 };
    inline const juce::Rectangle<int> kGateHitBox { 258, 414, 84, 106 };

    inline const juce::Rectangle<int> kPressureLabel { 135, 528, 150, 14 };
    inline const juce::Rectangle<int> kFootswitch { 138, 546, 144, 145 };

    inline const juce::Rectangle<int> kAdvancedLabel { 264, 654, 94, 18 };
    inline const juce::Rectangle<int> kAdvancedHitBox { 258, 642, 100, 38 };
    inline const juce::Rectangle<int> kAdvancedDrawer { 214, 476, 168, 204 };
} // namespace facelayout

/** One key light for the whole plate so every highlight and shadow agrees — the
    single strongest cue that the parts share a real surface. The light sits upper-
    left; highlights face it, cast/contact shadows fall to the lower-right. */
namespace lighting
{
    inline const juce::Point<float> toLight   { -0.55f, -0.83f }; // unit-ish, toward the light
    inline const juce::Point<float> shadowDir {  0.55f,  0.83f }; // opposite: where shadows fall

    /** Integer pixel offset for a shadow cast `distance` px from its caster. */
    inline juce::Point<int> shadowOffset (float distance) noexcept
    {
        return { juce::roundToInt (shadowDir.x * distance),
                 juce::roundToInt (shadowDir.y * distance) };
    }
} // namespace lighting

/** True when footswitch pressed overlay should draw (press/amount — not connection alone). */
bool shouldDrawFootswitchPressedOverlay (bool padPressed, float displayAmount, float sendAmountNorm) noexcept;

/** Draw the production photographed chassis and its live control-state artwork. */
void paintPedalFaceplate (juce::Graphics& g,
                          juce::Rectangle<float> bounds,
                          juce::Colour cyan,
                          juce::AudioProcessorValueTreeState& apvts,
                          bool clipActive,
                          bool advancedExpanded,
                          bool padPressed = false,
                          float padDisplayAmount = 0.0f,
                          float footTravel = -1.0f);

/** Drawn from the editor's paintOverChildren: dims the whole scene when dark mode
    is engaged (children included) and re-draws the emissive clip lamp above the
    dimmed room so it keeps glowing. */
void paintPedalOverlay (juce::Graphics& g,
                        juce::Rectangle<float> bounds,
                        bool darkMode,
                        bool clipActive);

} // namespace sendbloom::ui
