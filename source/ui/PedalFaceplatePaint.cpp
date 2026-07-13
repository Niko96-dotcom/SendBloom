#include "PedalFaceplatePaint.h"
#include "../ParameterIDs.h"
#include <BinaryData.h>

#include <cmath>

namespace sendbloom::ui
{

namespace
{

juce::Image loadImage (const void* data, size_t size)
{
    return juce::ImageFileFormat::loadFrom (data, size);
}

struct PedalArtwork
{
    juce::Image background { loadImage (BinaryData::pedal_background_png, BinaryData::pedal_background_pngSize) };
    juce::Image logo { loadImage (BinaryData::brand_logo_png, BinaryData::brand_logo_pngSize) };
    juce::Image preset { loadImage (BinaryData::preset_field_png, BinaryData::preset_field_pngSize) };
    juce::Image load { loadImage (BinaryData::preset_load_png, BinaryData::preset_load_pngSize) };
    juce::Image save { loadImage (BinaryData::preset_save_png, BinaryData::preset_save_pngSize) };
    juce::Image darkOff { loadImage (BinaryData::dark_off_png, BinaryData::dark_off_pngSize) };
    juce::Image darkOn { loadImage (BinaryData::dark_on_png, BinaryData::dark_on_pngSize) };
    juce::Image gatePre { loadImage (BinaryData::gate_pre_png, BinaryData::gate_pre_pngSize) };
    juce::Image gatePost { loadImage (BinaryData::gate_post_png, BinaryData::gate_post_pngSize) };
    juce::Image footUp { loadImage (BinaryData::footswitch_up_png, BinaryData::footswitch_up_pngSize) };
    juce::Image footDown { loadImage (BinaryData::footswitch_down_png, BinaryData::footswitch_down_pngSize) };
    juce::Image clipOff { loadImage (BinaryData::clip_off_png, BinaryData::clip_off_pngSize) };
    juce::Image clipOn { loadImage (BinaryData::clip_on_png, BinaryData::clip_on_pngSize) };
};

const PedalArtwork& artwork()
{
    static const PedalArtwork images;
    return images;
}

const auto kInk = juce::Colour (0xff161413);
const auto kOrange = juce::Colour (0xffe66c0b);

// Registration marks (teal dots, since painted out of the art) sat on the
// footswitch's fixed collar. Measured centroids in source-image pixels...
const juce::Point<float> kFootUpDotA   { 397.2f, 473.9f };
const juce::Point<float> kFootUpDotB   { 512.6f, 288.5f };
const juce::Point<float> kFootDownDotA { 389.9f, 474.1f };
const juce::Point<float> kFootDownDotB { 508.0f, 287.1f };
// ...both aligned onto these fixed plate targets (editor coords), so the collar
// is pinned and only the cap moves between raised and stomped.
const juce::Point<float> kFootTargetA  { 234.4f, 660.9f };
const juce::Point<float> kFootTargetB  { 262.4f, 616.0f };

void drawImage (juce::Graphics& g, const juce::Image& image, juce::Rectangle<float> bounds,
                juce::RectanglePlacement placement = juce::RectanglePlacement::centred)
{
    if (! image.isValid())
        return;

    // Images composite with the current colour's opacity; never inherit a label alpha.
    g.setOpacity (1.0f);
    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
    g.drawImage (image, bounds, placement, false);
}

// Some parts are photographed in two states whose frames differ in size (a toggle
// lever swings past the edge, so PRE and POST crop to different heights). A plain
// centred blit then lands the fixed body in a different spot each state and the
// part appears to hop. Instead, register both frames by a landmark on the *fixed*
// body — the hex nut — so it stays pinned and only the moving part travels.
// nutNorm is the nut centre as a fraction of the frame; nutWidthPx is its pixel
// width, so both states share one on-screen scale.
void drawNutAnchoredImage (juce::Graphics& g, const juce::Image& image,
                           float nutNormX, float nutNormY, float nutWidthPx,
                           juce::Point<float> targetNutCentre, float targetNutWidth)
{
    if (! image.isValid())
        return;

    const auto scale = targetNutWidth / nutWidthPx;
    const auto w = static_cast<float> (image.getWidth())  * scale;
    const auto h = static_cast<float> (image.getHeight()) * scale;
    const juce::Rectangle<float> dest (targetNutCentre.x - nutNormX * w,
                                       targetNutCentre.y - nutNormY * h,
                                       w, h);

    g.setOpacity (1.0f);
    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
    g.drawImage (image, dest, juce::RectanglePlacement::stretchToFit, false);
}

// Two-point similarity registration: map a pair of landmarks (in source-image
// pixels) onto fixed plate targets, correcting for the small translation, scale,
// and rotation drift between two frames photographed apart. Used for the
// footswitch, whose up/down shots were misaligned a few pixels at the collar.
juce::AffineTransform registerByTwoPoints (juce::Point<float> s1, juce::Point<float> s2,
                                           juce::Point<float> t1, juce::Point<float> t2)
{
    const auto s = s2 - s1;
    const auto t = t2 - t1;
    const auto scale = t.getDistanceFromOrigin() / s.getDistanceFromOrigin();
    const auto angle = std::atan2 (t.y, t.x) - std::atan2 (s.y, s.x);

    return juce::AffineTransform::translation (-s1.x, -s1.y)
        .scaled (scale)
        .rotated (angle)
        .translated (t1.x, t1.y);
}

// The scene photograph keeps the pedal small inside a moody workbench shot. Crop the
// draw to a hero framing: the chassis fills the editor with only a sliver of scene
// around it, so every control on the plate gets real estate.
void drawBackground (juce::Graphics& g, const juce::Image& image, juce::Rectangle<float> bounds)
{
    if (! image.isValid())
        return;

    constexpr int srcX = 90, srcY = 70, srcW = 612, srcH = 1136;
    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
    g.drawImage (image,
                 juce::roundToInt (bounds.getX()), juce::roundToInt (bounds.getY()),
                 juce::roundToInt (bounds.getWidth()), juce::roundToInt (bounds.getHeight()),
                 srcX, srcY, srcW, srcH);
}

// ---- depth pass -------------------------------------------------------------
// The extraction mattes strip the studio contact shadows off every part, so the
// composited plate reads flat. Rebuild the depth photographically: one soft key
// light from above (sheen + vignette on the plate) and a contact shadow under
// each raised part. Everything here is static, so it renders once into a layer.

void addPathShadow (juce::Graphics& g, const juce::Path& path,
                    float alpha, int radius, juce::Point<int> offset)
{
    juce::DropShadow (juce::Colours::black.withAlpha (alpha), radius, offset).drawForPath (g, path);
}

void addEllipseShadow (juce::Graphics& g, juce::Rectangle<float> area,
                       float alpha, int radius, juce::Point<int> offset)
{
    juce::Path path;
    path.addEllipse (area);
    addPathShadow (g, path, alpha, radius, offset);
}

void addRoundedShadow (juce::Graphics& g, juce::Rectangle<float> area, float corner,
                       float alpha, int radius, juce::Point<int> offset)
{
    juce::Path path;
    path.addRoundedRectangle (area, corner);
    addPathShadow (g, path, alpha, radius, offset);
}

void drawPlateLighting (juce::Graphics& g)
{
    using namespace facelayout;
    juce::Graphics::ScopedSaveState state (g);

    juce::Path plate;
    plate.addRoundedRectangle (kPlate, kPlateCornerRadius);
    g.reduceClipRegion (plate);

    // Key light catches the top of the plate...
    juce::ColourGradient sheen (juce::Colours::white.withAlpha (0.08f),
                                kPlate.getCentreX(), kPlate.getY(),
                                juce::Colours::transparentWhite,
                                kPlate.getCentreX(), kPlate.getY() + 190.0f, false);
    g.setGradientFill (sheen);
    g.fillRect (kPlate.withHeight (190.0f));

    // ...and falls away toward the edges and the stomp end.
    juce::ColourGradient vignette (juce::Colours::transparentBlack,
                                   kPlate.getCentreX(), kPlate.getCentreY() - 60.0f,
                                   juce::Colours::black.withAlpha (0.17f),
                                   kPlate.getX(), kPlate.getY(), true);
    vignette.addColour (0.70, juce::Colours::transparentBlack);
    g.setGradientFill (vignette);
    g.fillRect (kPlate);

    // The scene's neon rig sits camera-right; let a whisper of it spill onto the enamel.
    juce::ColourGradient spill (juce::Colour (0xffff4d5e).withAlpha (0.07f),
                                kPlate.getRight(), kPlate.getCentreY(),
                                juce::Colours::transparentBlack,
                                kPlate.getRight() - 90.0f, kPlate.getCentreY(), false);
    g.setGradientFill (spill);
    g.fillRect (kPlate.withLeft (kPlate.getRight() - 90.0f));

    // A soft clear-coat hotspot upper-left where the key light reflects off the
    // glossy enamel — the highlight the eye reads as a curved, lacquered surface.
    const juce::Point<float> hot (kPlate.getCentreX() - 58.0f, kPlate.getY() + 96.0f);
    juce::ColourGradient gloss (juce::Colours::white.withAlpha (0.14f), hot.x, hot.y,
                                juce::Colours::transparentWhite, hot.x, hot.y + 120.0f, true);
    gloss.addColour (0.55, juce::Colours::transparentWhite);
    g.setGradientFill (gloss);
    g.fillEllipse (juce::Rectangle<float> (230.0f, 150.0f).withCentre (hot));
}

// Cross-head screws seat the plate into the chassis. Procedural, but at 14 px the
// radial shading and per-screw slot angles read photographic.
void drawScrew (juce::Graphics& g, juce::Point<float> centre, float radius, float slotAngle)
{
    const juce::Rectangle<float> head (centre.x - radius, centre.y - radius,
                                       radius * 2.0f, radius * 2.0f);

    // The head sits in a drilled countersink: a dark recessed ring around it, with
    // the far (lower-right) side of the hole catching the least light.
    const auto hole = head.expanded (2.4f);
    juce::ColourGradient bore (juce::Colours::black.withAlpha (0.42f),
                               hole.getRight(), hole.getBottom(),
                               juce::Colours::black.withAlpha (0.10f),
                               hole.getX(), hole.getY(), false);
    g.setGradientFill (bore);
    g.fillEllipse (hole);

    addEllipseShadow (g, head.reduced (1.0f), 0.30f, 3, lighting::shadowOffset (2.0f));

    // Head metal, lit from the upper-left toward the light.
    juce::ColourGradient metal (juce::Colour (0xffe6e3dc),
                                centre.x + lighting::toLight.x * radius * 0.7f,
                                centre.y + lighting::toLight.y * radius * 0.7f,
                                juce::Colour (0xff6d6963),
                                centre.x - lighting::toLight.x * radius,
                                centre.y - lighting::toLight.y * radius, true);
    g.setGradientFill (metal);
    g.fillEllipse (head);

    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.drawEllipse (head.reduced (0.4f), 0.9f);

    const auto slot = juce::Point<float> (std::cos (slotAngle), std::sin (slotAngle)) * (radius * 0.72f);
    g.setColour (juce::Colours::black.withAlpha (0.62f));
    g.drawLine ({ centre - slot, centre + slot }, 1.6f);
    g.setColour (juce::Colours::white.withAlpha (0.30f));
    g.drawLine ({ centre - slot + juce::Point<float> (0.0f, 1.2f),
                  centre + slot + juce::Point<float> (0.0f, 1.2f) }, 0.8f);

    // A single specular glint on the light-facing shoulder of the head.
    const auto glint = centre + juce::Point<float> (lighting::toLight.x, lighting::toLight.y) * (radius * 0.5f);
    g.setColour (juce::Colours::white.withAlpha (0.55f));
    g.fillEllipse (juce::Rectangle<float> (radius * 0.5f, radius * 0.5f).withCentre (glint));
}

void drawPlateScrews (juce::Graphics& g)
{
    using namespace facelayout;
    const auto inset = 20.0f;
    drawScrew (g, { kPlate.getX() + inset, kPlate.getY() + inset }, 7.0f, 0.6f);
    drawScrew (g, { kPlate.getRight() - inset, kPlate.getY() + inset }, 7.0f, 2.2f);
    drawScrew (g, { kPlate.getX() + inset, kPlate.getBottom() - inset }, 7.0f, 1.1f);
    drawScrew (g, { kPlate.getRight() - inset, kPlate.getBottom() - inset }, 7.0f, 2.9f);
}

// A film-grain wash knits the separately photographed parts into one exposure.
void drawGrain (juce::Graphics& g, int width, int height)
{
    juce::Image noise (juce::Image::ARGB, width / 2, height / 2, true);
    juce::Random random (0x5eedb100);

    for (int y = 0; y < noise.getHeight(); ++y)
        for (int x = 0; x < noise.getWidth(); ++x)
        {
            const auto v = random.nextFloat() * 2.0f - 1.0f;
            const auto tone = v > 0.0f ? juce::Colours::white : juce::Colours::black;
            noise.setPixelAt (x, y, tone.withAlpha (std::abs (v) * 0.055f));
        }

    g.setOpacity (1.0f);
    g.setImageResamplingQuality (juce::Graphics::mediumResamplingQuality);
    g.drawImage (noise, { 0.0f, 0.0f, static_cast<float> (width), static_cast<float> (height) },
                 juce::RectanglePlacement::stretchToFit, false);
}

void drawContactShadows (juce::Graphics& g)
{
    using namespace facelayout;
    using lighting::shadowOffset;

    // The nameplate stands proudest of everything on the plate, so it throws the
    // longest, softest shadow — that gap sells a screwed-on metal badge.
    addRoundedShadow (g, kLogo.toFloat().reduced (3.0f), 10.0f, 0.34f, 7, shadowOffset (5.0f));
    addRoundedShadow (g, kPresetField.toFloat().reduced (1.0f), 7.0f, 0.26f, 4, shadowOffset (3.0f));
    addRoundedShadow (g, kPresetLoad.toFloat().reduced (1.0f), 7.0f, 0.30f, 4, shadowOffset (3.0f));
    addRoundedShadow (g, kPresetSave.toFloat().reduced (1.0f), 7.0f, 0.30f, 4, shadowOffset (3.0f));

    // Knobs get a soft cast shadow plus a tight ambient-occlusion seam right at the
    // skirt — the close-contact darkening is what actually plants them (#3).
    for (const auto& knob : { kDistortionKnob, kSizeKnob, kLevelKnob })
    {
        const auto body = knob.withHeight (kKnobLarge).toFloat().reduced (5.0f);
        addEllipseShadow (g, body, 0.40f, 10, shadowOffset (7.0f));
        addEllipseShadow (g, body.expanded (1.0f), 0.28f, 3, shadowOffset (1.5f));
    }

    for (const auto& knob : { kInputKnob, kOutputKnob })
    {
        const auto body = knob.withHeight (kKnobSmall).toFloat().reduced (4.0f);
        addEllipseShadow (g, body, 0.38f, 9, shadowOffset (6.0f));
        addEllipseShadow (g, body.expanded (1.0f), 0.26f, 3, shadowOffset (1.5f));
    }

    addRoundedShadow (g, kClipLens.toFloat().reduced (2.0f), 8.0f, 0.32f, 4, shadowOffset (3.0f));

    addRoundedShadow (g, kDarkButton.toFloat().reduced (5.0f), 16.0f, 0.36f, 8, shadowOffset (6.0f));
    addRoundedShadow (g, kDarkButton.toFloat().reduced (4.0f), 16.0f, 0.26f, 3, shadowOffset (1.5f));

    // The toggle's weight sits in its hex nut. Ground it with a contact shadow the
    // size of the nut footprint, centred on the fixed nut position (see the
    // nut-anchored draw) so the switch reads as seated in the plate, not floating.
    addEllipseShadow (g,
                      { kGateSwitch.toFloat().getCentreX() - 26.0f, 470.0f - 22.0f,
                        52.0f, 48.0f },
                      0.38f, 8, shadowOffset (6.0f));

    // Footswitch is deliberately shadow-free: it seats on its own bevelled metal
    // edge (a drawn halo just reads as a ring on the plate).
}

constexpr int kDrawerShadowMargin = 26;

// The open drawer floats above the plate; its silhouette (matching
// AdvancedDrawer::paint) casts onto whatever sits underneath.
const juce::Image& drawerShadowImage()
{
    static const juce::Image image = []
    {
        constexpr int margin = kDrawerShadowMargin;
        const auto b = facelayout::kAdvancedDrawer;
        const auto w = static_cast<float> (b.getWidth());
        const auto h = static_cast<float> (b.getHeight());
        juce::Image img (juce::Image::ARGB, b.getWidth() + margin * 2, b.getHeight() + margin * 2, true);
        juce::Graphics g (img);

        juce::Path panel;
        const auto m = static_cast<float> (margin);
        panel.startNewSubPath (m + 14.0f, m);
        panel.lineTo (m + w - 3.0f, m);
        panel.lineTo (m + w - 3.0f, m + h - 28.0f);
        panel.lineTo (m + w - 28.0f, m + h - 3.0f);
        panel.lineTo (m + 2.0f, m + h - 3.0f);
        panel.lineTo (m + 2.0f, m + 14.0f);
        panel.closeSubPath();

        juce::DropShadow (juce::Colours::black.withAlpha (0.50f), 15, { 5, 8 }).drawForPath (g, panel);
        return img;
    }();

    return image;
}

void drawPresetInnerShadow (juce::Graphics& g)
{
    using namespace facelayout;
    juce::Graphics::ScopedSaveState state (g);

    const auto field = kPresetField.toFloat().reduced (3.0f);
    juce::Path inner;
    inner.addRoundedRectangle (field, 6.0f);
    g.reduceClipRegion (inner);

    // Recessed window: the bezel shades the top of the field.
    juce::ColourGradient shade (juce::Colours::black.withAlpha (0.18f),
                                field.getCentreX(), field.getY(),
                                juce::Colours::transparentBlack,
                                field.getCentreX(), field.getY() + 8.0f, false);
    g.setGradientFill (shade);
    g.fillRect (field.withHeight (8.0f));

    g.setColour (juce::Colours::white.withAlpha (0.28f));
    g.fillRect (field.withTop (field.getBottom() - 1.5f));
}

// Labels are stamped into the enamel: the light-facing (upper-left) inner wall of
// each glyph falls into shadow, and the opposite (lower-right) lip catches a
// highlight. Drawing both around the ink reads as debossed, not printed.
void drawEngravedText (juce::Graphics& g, const juce::String& text,
                       juce::Rectangle<int> area, float fontHeight, juce::Colour ink,
                       juce::Justification justification = juce::Justification::centred)
{
    g.setFont (juce::FontOptions (fontHeight, juce::Font::bold));

    g.setColour (juce::Colours::white.withAlpha (0.32f));
    g.drawText (text, area.translated (1, 1), justification, false);   // lit lower-right lip
    g.setColour (juce::Colours::black.withAlpha (0.22f));
    g.drawText (text, area.translated (-1, -1), justification, false); // shaded upper-left wall

    g.setColour (ink);
    g.drawText (text, area, justification, false);
}

const juce::Image& staticFaceLayer()
{
    static const juce::Image layer = []
    {
        const auto& art = artwork();
        juce::Image image (juce::Image::ARGB, 420, 780, true);
        juce::Graphics g (image);

        g.fillAll (juce::Colours::black);
        drawBackground (g, art.background, { 0.0f, 0.0f, 420.0f, 780.0f });
        drawPlateLighting (g);
        drawPlateScrews (g);
        drawContactShadows (g);

        drawImage (g, art.logo, facelayout::kLogo.toFloat());
        drawImage (g, art.preset, facelayout::kPresetField.toFloat(),
                   juce::RectanglePlacement::stretchToFit);
        drawPresetInnerShadow (g);
        drawImage (g, art.load, facelayout::kPresetLoad.toFloat());
        drawImage (g, art.save, facelayout::kPresetSave.toFloat());

        drawGrain (g, 420, 780);
        return image;
    }();

    return layer;
}

// -----------------------------------------------------------------------------

bool isParamOn (juce::AudioProcessorValueTreeState& apvts, const char* id)
{
    if (auto* value = apvts.getRawParameterValue (id))
        return value->load() > 0.5f;

    return false;
}

float getParamNorm (juce::AudioProcessorValueTreeState& apvts, const char* id)
{
    if (auto* value = apvts.getRawParameterValue (id))
        return value->load();

    return 0.0f;
}

int getChoiceIndex (juce::AudioProcessorValueTreeState& apvts, const char* id)
{
    if (auto* value = apvts.getRawParameterValue (id))
        return static_cast<int> (std::round (value->load()));

    return 0;
}

void drawGateMarkings (juce::Graphics& g, bool postGate)
{
    using namespace facelayout;

    drawEngravedText (g, "GATE", kGateTitle, 12.0f, kInk.withAlpha (0.94f));
    drawEngravedText (g, "PRE", kGatePre, 10.5f, postGate ? kInk.withAlpha (0.38f) : kOrange);
    drawEngravedText (g, "POST", kGatePost, 10.5f, postGate ? kOrange : kInk.withAlpha (0.38f));
}

void drawClipLamp (juce::Graphics& g, const PedalArtwork& art, bool active, bool darkRoom)
{
    using namespace facelayout;
    const auto lens = kClipLens.toFloat();

    if (active)
    {
        // An LED reads brighter in a dimmed room.
        const auto boost = darkRoom ? 1.6f : 1.0f;
        g.setColour (juce::Colour (0xffff2a20).withAlpha (juce::jmin (0.9f, 0.28f * boost)));
        g.fillEllipse (lens.expanded (5.0f));
        g.setColour (juce::Colour (0xffff5a43).withAlpha (juce::jmin (0.6f, 0.10f * boost)));
        g.fillEllipse (lens.expanded (darkRoom ? 18.0f : 12.0f));
    }

    drawImage (g, active ? art.clipOn : art.clipOff, lens);

    // The lens is a glossy dome: a hot core when lit, and a small off-centre glass
    // highlight (upper-left, with the key light) in either state.
    if (active)
    {
        g.setColour (juce::Colours::white.withAlpha (darkRoom ? 0.85f : 0.7f));
        g.fillEllipse (lens.reduced (lens.getWidth() * 0.34f));
    }

    const auto spec = lens.getCentre()
                    + juce::Point<float> (lighting::toLight.x, lighting::toLight.y) * (lens.getWidth() * 0.22f);
    g.setColour (juce::Colours::white.withAlpha (active ? 0.9f : 0.4f));
    g.fillEllipse (juce::Rectangle<float> (lens.getWidth() * 0.2f,
                                           lens.getWidth() * 0.2f).withCentre (spec));
}

} // namespace

bool shouldDrawFootswitchPressedOverlay (bool padPressed, float displayAmount, float sendAmountNorm) noexcept
{
    constexpr auto kEpsilon = 0.001f;
    return padPressed || displayAmount > kEpsilon || sendAmountNorm > kEpsilon;
}

void paintPedalFaceplate (juce::Graphics& g,
                          juce::Rectangle<float> bounds,
                          juce::Colour accent,
                          juce::AudioProcessorValueTreeState& apvts,
                          bool clipActive,
                          bool advancedExpanded,
                          bool padPressed,
                          float padDisplayAmount)
{
    juce::ignoreUnused (accent);
    using namespace facelayout;
    const auto& art = artwork();

    // Scene, plate lighting, contact shadows, and static assets render once.
    g.setOpacity (1.0f);
    g.drawImage (staticFaceLayer(), bounds, juce::RectanglePlacement::stretchToFit, false);

    const auto dark = isParamOn (apvts, ParameterIDs::darkMode);
    const auto postGate = getChoiceIndex (apvts, ParameterIDs::gatePrePost) == 1;
    const auto sendAmount = getParamNorm (apvts, ParameterIDs::sendAmount);
    const auto footPressed = shouldDrawFootswitchPressedOverlay (
        padPressed, padDisplayAmount, sendAmount);

    drawImage (g, dark ? art.darkOn : art.darkOff, kDarkButton.toFloat());

    // PRE (lever up) and POST (lever down) are the same switch; their frames differ
    // in height only because the lever leaves the crop. Anchor by the hex nut —
    // measured centre and pixel width per frame — so the body stays fixed on the
    // plate and only the lever swings. (A centred blit used to hop the nut ~9px.)
    const juce::Point<float> gateNutCentre { kGateSwitch.toFloat().getCentreX(), 470.0f };
    constexpr float gateNutWidth = 50.0f;
    if (postGate)
        drawNutAnchoredImage (g, art.gatePost, 0.500f, 0.389f, 275.0f, gateNutCentre, gateNutWidth);
    else
        drawNutAnchoredImage (g, art.gatePre, 0.498f, 0.532f, 271.0f, gateNutCentre, gateNutWidth);

    // No cast shadow under the footswitch: photographed head-on, a chromed stomp
    // cap seats on its own bevelled edge, and any drawn halo just reads as a ring
    // painted on the plate. The metal edge alone carries the depth.
    // Register the up/down frames by the collar marks so the base stays pinned and
    // only the cap drops when stomped (the down photo already carries that depth).
    const auto footTf = footPressed
        ? registerByTwoPoints (kFootDownDotA, kFootDownDotB, kFootTargetA, kFootTargetB)
        : registerByTwoPoints (kFootUpDotA, kFootUpDotB, kFootTargetA, kFootTargetB);
    g.setOpacity (1.0f);
    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
    g.drawImageTransformed (footPressed ? art.footDown : art.footUp, footTf, false);

    drawGateMarkings (g, postGate);
    drawEngravedText (g, "CLIP", kClipLabel, 10.0f, kInk.withAlpha (0.94f));
    drawEngravedText (g, "PRESSURE SEND", kPressureLabel, 11.5f, kOrange);

    if (advancedExpanded)
    {
        g.setOpacity (1.0f);
        g.drawImageAt (drawerShadowImage(),
                       kAdvancedDrawer.getX() - kDrawerShadowMargin,
                       kAdvancedDrawer.getY() - kDrawerShadowMargin);
    }
    else
    {
        drawEngravedText (g, "ADVANCED >", kAdvancedLabel, 11.5f, kOrange,
                          juce::Justification::centredRight);
    }
}

void paintPedalOverlay (juce::Graphics& g,
                        juce::Rectangle<float> bounds,
                        bool darkMode,
                        bool clipActive)
{
    if (darkMode)
    {
        // Dark mode kills the room lights: everything on the bench dims and cools,
        // and only emissive parts (the clip LED) punch through.
        g.setColour (juce::Colour (0xff0a0f18).withAlpha (0.34f));
        g.fillRect (bounds);
    }

    drawClipLamp (g, artwork(), clipActive, darkMode);
}

} // namespace sendbloom::ui
