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

/** A render plus its box-halved standard-DPI variant. */
struct Art
{
    explicit Art (juce::Image hiRes)
        : hi (std::move (hiRes)), lo (boxHalveImage (hi)) {}

    juce::Image hi, lo;
};

/** Every asset here is a Cycles render from tools/render_ui.py: the whole
    pedal modelled and lit as one scene, so the shadows, occlusion and bounce
    between parts were computed together instead of drawn per part. The
    background carries the knob contact shadows; each moving part is an
    alpha overlay that carries its own state-correct cast shadow. All art is
    rendered at 2x for hi-DPI backing stores. */
struct PedalArtwork
{
    // Opaque full-frame backdrop, so it ships as JPEG (a tenth of the PNG).
    Art background { loadImage (BinaryData::pedal_background_jpg, BinaryData::pedal_background_jpgSize) };
    Art darkOff { loadImage (BinaryData::dark_off_png, BinaryData::dark_off_pngSize) };
    Art darkOn { loadImage (BinaryData::dark_on_png, BinaryData::dark_on_pngSize) };
    Art gatePre { loadImage (BinaryData::gate_pre_png, BinaryData::gate_pre_pngSize) };
    Art gatePost { loadImage (BinaryData::gate_post_png, BinaryData::gate_post_pngSize) };
    Art footUp { loadImage (BinaryData::footswitch_up_png, BinaryData::footswitch_up_pngSize) };
    Art footDown { loadImage (BinaryData::footswitch_down_png, BinaryData::footswitch_down_pngSize) };
    Art clipOff { loadImage (BinaryData::clip_off_png, BinaryData::clip_off_pngSize) };
    Art clipOn { loadImage (BinaryData::clip_on_png, BinaryData::clip_on_pngSize) };
};

const PedalArtwork& artwork()
{
    static const PedalArtwork images;
    return images;
}

const auto kInk = juce::Colour (0xff161413);
const auto kOrange = juce::Colour (0xffe66c0b);

// Overlay draw rects. Each matches the render crop in tools/render_ui.py
// (ART_RECTS) exactly: the same ortho camera rendered both states and the
// background, so blitting the rect registers the part to the pixel and the
// part's soft shadow lands where the scene computed it. Keep in sync.
const juce::Rectangle<float> kGateArt { 262.0f, 430.0f, 76.0f, 100.0f };
const juce::Rectangle<float> kFootArt { 126.0f, 534.0f, 168.0f, 168.0f };
const juce::Rectangle<float> kDarkArt { 78.0f, 432.0f, 86.0f, 86.0f };
const juce::Rectangle<float> kClipArt { 186.0f, 320.0f, 48.0f, 48.0f };

void drawImage (juce::Graphics& g, const Art& art, juce::Rectangle<float> bounds)
{
    const auto& image = wantsHiResArt (g) ? art.hi : art.lo;
    if (! image.isValid())
        return;

    // Images composite with the current colour's opacity; never inherit a label alpha.
    g.setOpacity (1.0f);
    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
    g.drawImage (image, bounds, juce::RectanglePlacement::stretchToFit, false);
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
    const auto lens = facelayout::kClipLens.toFloat();

    if (active)
    {
        // The bloom around the lens is live (it brightens in a dimmed room),
        // so it stays a runtime pass; the lit lens itself is the rendered
        // emissive dome in clip_on.
        const auto boost = darkRoom ? 1.6f : 1.0f;
        g.setColour (juce::Colour (0xffff2a20).withAlpha (juce::jmin (0.9f, 0.28f * boost)));
        g.fillEllipse (lens.expanded (5.0f));
        g.setColour (juce::Colour (0xffff5a43).withAlpha (juce::jmin (0.6f, 0.10f * boost)));
        g.fillEllipse (lens.expanded (darkRoom ? 18.0f : 12.0f));
    }

    drawImage (g, active ? art.clipOn : art.clipOff, kClipArt);
}

} // namespace

juce::Image boxHalveImage (const juce::Image& source)
{
    if (! source.isValid())
        return {};

    const auto w = source.getWidth() / 2;
    const auto h = source.getHeight() / 2;
    juce::Image out (juce::Image::ARGB, juce::jmax (1, w), juce::jmax (1, h), true);

    const juce::Image::BitmapData src (source, juce::Image::BitmapData::readOnly);
    juce::Image::BitmapData dst (out, juce::Image::BitmapData::writeOnly);

    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            float a = 0, r = 0, g = 0, b = 0;
            for (int sy = 0; sy < 2; ++sy)
                for (int sx = 0; sx < 2; ++sx)
                {
                    const auto c = src.getPixelColour (x * 2 + sx, y * 2 + sy);
                    const auto alpha = c.getFloatAlpha();
                    a += alpha;                       // premultiplied average
                    r += c.getFloatRed() * alpha;
                    g += c.getFloatGreen() * alpha;
                    b += c.getFloatBlue() * alpha;
                }
            const auto scale = a > 0.0f ? 1.0f / a : 0.0f;
            dst.setPixelColour (x, y,
                juce::Colour::fromFloatRGBA (r * scale, g * scale, b * scale, a * 0.25f));
        }
    }

    return out;
}

bool wantsHiResArt (juce::Graphics& g)
{
    return g.getInternalContext().getPhysicalPixelScaleFactor() >= 1.5f;
}

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
                          float padDisplayAmount,
                          float footTravel)
{
    juce::ignoreUnused (accent, clipActive);
    using namespace facelayout;
    const auto& art = artwork();

    // The scene render: plate, chassis, screws, badge, preset furniture and
    // every baked contact shadow, in one exposure.
    drawImage (g, art.background, bounds);

    const auto dark = isParamOn (apvts, ParameterIDs::darkMode);
    const auto postGate = getChoiceIndex (apvts, ParameterIDs::gatePrePost) == 1;
    const auto sendAmount = getParamNorm (apvts, ParameterIDs::sendAmount);
    const auto footPressed = shouldDrawFootswitchPressedOverlay (
        padPressed, padDisplayAmount, sendAmount);

    drawImage (g, dark ? art.darkOn : art.darkOff, kDarkArt);

    // PRE is the lever up, POST the lever down. Both states share one camera
    // and crop, so the same rect holds the body still and only the lever swings.
    drawImage (g, postGate ? art.gatePost : art.gatePre, kGateArt);

    // Cross-dissolve the two states by travel so the cap moves instead of
    // teleporting. A caller with no travel to offer (the snapshot tool) passes a
    // negative and gets the plain two-state swap.
    if (footTravel < 0.0f)
    {
        drawImage (g, footPressed ? art.footDown : art.footUp, kFootArt);
    }
    else if (footTravel <= 0.0f)
    {
        drawImage (g, art.footUp, kFootArt);
    }
    else if (footTravel >= 1.0f)
    {
        drawImage (g, art.footDown, kFootArt);
    }
    else
    {
        drawImage (g, art.footUp, kFootArt);
        g.setOpacity (footTravel);
        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        g.drawImage (wantsHiResArt (g) ? art.footDown.hi : art.footDown.lo,
                     kFootArt, juce::RectanglePlacement::stretchToFit, false);
        g.setOpacity (1.0f);
    }

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
