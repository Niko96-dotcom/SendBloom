#pragma once
#include <ClearInstrumentsData.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <map>
#include <vector>

namespace sendbloom::ui
{
class SendBloomSceneArt
{
public:
    struct Layer
    {
        juce::Rectangle<int> crop;
        std::vector<juce::Image> frames;
        juce::var sources;
        void paint (juce::Graphics& g, int index) const
        {
            if (! frames.empty())
                g.drawImage (frames[static_cast<size_t> (juce::jlimit (0, static_cast<int> (frames.size()) - 1, index))], crop.toFloat() * 0.5f);
        }
    };
    static juce::Rectangle<int> rectangle (const juce::var& v)
    {
        if (auto* a = v.getArray(); a != nullptr && a->size() == 4)
            return { int((*a)[0]), int((*a)[1]), int((*a)[2]), int((*a)[3]) };
        return {};
    }
    static const void* resource (const juce::String& filename, int& size)
    {
        for (int i = 0; i < ClearInstrumentsData::namedResourceListSize; ++i)
            if (filename == ClearInstrumentsData::originalFilenames[i])
                return ClearInstrumentsData::getNamedResource (ClearInstrumentsData::namedResourceList[i], size);
        return nullptr;
    }
    static juce::Image image (const juce::String& filename)
    {
        int size = 0; const auto* data = resource (filename, size);
        return data != nullptr ? juce::ImageCache::getFromMemory (data, size) : juce::Image();
    }
    SendBloomSceneArt()
    {
        int size = 0; const auto* data = resource ("manifest.json", size);
        if (data == nullptr) return;
        manifest = juce::JSON::parse (juce::String::fromUTF8 (static_cast<const char*> (data), size));
        const auto render = manifest["render"];
        if (int(render["width"]) != 1680 || int(render["height"]) != 1400 || render["origin"].toString() != "top-left") return;
        base = image (manifest["base_image"].toString());
        valid = base.isValid() && base.getWidth() == 1680 && base.getHeight() == 1400;
        const auto auxiliaryKnob = image("aux-knob-strip.png");
        valid = valid && auxiliaryKnob.isValid() && auxiliaryKnob.getWidth() == 256 && auxiliaryKnob.getHeight() == 256 * 65;
        for (const auto* filename : { "aux-switch-0.png", "aux-switch-1.png" })
        {
            const auto skin = image(filename);
            valid = valid && skin.isValid() && skin.getWidth() == 400 && skin.getHeight() == 232;
        }
        for (const auto* filename : { "aux-display.png", "aux-button-0.png", "aux-button-1.png" })
        {
            const auto skin = image(filename);
            valid = valid && skin.isValid() && skin.getWidth() == 400 && skin.getHeight() == 144;
        }
        for (const auto* id : { "input_gain", "distn", "size", "level", "output_gain" })
            knobs[id] = readLayer (manifest["knobs"][juce::Identifier(id)], 65);
        pressure = readLayer (manifest["pressure"]["send_amount"], 17);
        if (const auto* groups = manifest["discrete_groups"].getArray())
            for (const auto& group : *groups)
            {
                auto layer = readLayer (group, 1 << group["state_sources"].size());
                layer.sources = group["state_sources"];
                discrete.push_back (std::move (layer));
            }
        else valid = false;
        for (const auto* id : { "input_gain", "distn", "size", "level", "output_gain", "send_amount", "dark_mode", "gate_pre_post", "bypass", "preset", "load_preset", "save_preset", "advanced" })
            valid = valid && ! hit(id).isEmpty();
        for (const auto* id : { "input_gain", "distn", "size", "level", "output_gain", "send_amount", "ui_preset" })
            valid = valid && ! display(id).isEmpty();
        std::vector<juce::Rectangle<int>> crops;
        for (const auto& item : knobs) crops.push_back (item.second.crop);
        crops.push_back (pressure.crop);
        for (const auto& layer : discrete) crops.push_back (layer.crop);
        for (size_t a = 0; a < crops.size(); ++a)
            for (size_t b = a + 1; b < crops.size(); ++b)
                valid = valid && ! crops[a].intersects(crops[b]);
    }
    juce::Rectangle<int> hit (const juce::String& id) const
    { return (rectangle (manifest["input_regions"][juce::Identifier(id)]["hitbox_px"]).toFloat() * 0.5f).getSmallestIntegerContainer(); }
    juce::Rectangle<float> display (const juce::String& id) const
    { return rectangle (manifest["displays"][juce::Identifier(id)]["content_safe_bounds_px"]).toFloat() * 0.5f; }
    static const SendBloomSceneArt& instance() { static const SendBloomSceneArt art; return art; }
    juce::Image base;
    std::map<juce::String, Layer> knobs;
    Layer pressure;
    std::vector<Layer> discrete;
    bool valid = false;
private:
    Layer readLayer (const juce::var& v, int expected)
    {
        Layer layer; layer.crop = rectangle (v["crop_px"]);
        const auto frames = v["frames"];
        const int count = int(frames["count"]), width = int(frames["width"]), height = int(frames["height"]), columns = int(frames["columns"]), rows = int(frames["rows"]);
        const auto atlas = image (frames["file"].toString());
        if (! atlas.isValid() || count != expected || columns <= 0 || rows <= 0 || columns * rows < count
            || width != layer.crop.getWidth() || height != layer.crop.getHeight()
            || atlas.getWidth() != width * columns || atlas.getHeight() != height * rows
            || layer.crop.isEmpty() || ! juce::Rectangle<int>(0,0,1680,1400).contains(layer.crop))
        { valid = false; return layer; }
        for (int i = 0; i < count; ++i)
            layer.frames.push_back (atlas.getClippedImage ({ (i % columns)*width, (i / columns)*height, width, height }));
        return layer;
    }
    juce::var manifest;
};
}
