// Render an SVG through JUCE's own parser and write the result to a PNG.
//
// This exists to show what JUCE actually does with an asset, rather than what
// a spec-compliant renderer would do. JUCE's SVG support is a subset: patterns
// and masks are unimplemented, and <use> only resolves to single shape
// primitives. None of those failures raise an error — they render as black, or
// as nothing at all. Rendering is the only honest check.
//
//   SvgSnapshot <input.svg> <output.png> [--size N] [--bg RRGGBB]

#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI gui;

    if (argc < 3)
    {
        std::fprintf (stderr, "usage: SvgSnapshot <input.svg> <output.png> [--size N] [--bg RRGGBB]\n");
        return 2;
    }

    const juce::File input { juce::String (argv[1]) };
    juce::File output { juce::String (argv[2]) };

    int size = 512;
    auto background = juce::Colour (0xff808080); // neutral grey: black fills and holes both read against it

    for (int i = 3; i < argc; ++i)
    {
        const juce::String arg { argv[i] };

        if (arg == "--size" && i + 1 < argc)
            size = juce::String (argv[++i]).getIntValue();
        else if (arg == "--bg" && i + 1 < argc)
            background = juce::Colour::fromString ("ff" + juce::String (argv[++i]));
    }

    if (! input.existsAsFile())
    {
        std::fprintf (stderr, "no such file: %s\n", input.getFullPathName().toRawUTF8());
        return 1;
    }

    const auto svgText = input.loadFileAsString();
    std::unique_ptr<juce::Drawable> drawable (
        juce::Drawable::createFromImageData (svgText.toRawUTF8(), (size_t) svgText.getNumBytesAsUTF8()));

    if (drawable == nullptr)
    {
        std::fprintf (stderr, "JUCE failed to parse: %s\n", input.getFileName().toRawUTF8());
        return 1;
    }

    const auto bounds = drawable->getDrawableBounds();
    std::printf ("%-52s parsed bounds %7.1f x %-7.1f\n",
                 input.getFileName().toRawUTF8(), bounds.getWidth(), bounds.getHeight());

    juce::Image image (juce::Image::ARGB, size, size, true);
    juce::Graphics g (image);
    g.fillAll (background);

    drawable->drawWithin (g, juce::Rectangle<float> (0.0f, 0.0f, (float) size, (float) size),
                          juce::RectanglePlacement::centred, 1.0f);

    output.getParentDirectory().createDirectory();
    output.deleteFile();

    juce::PNGImageFormat format;
    juce::FileOutputStream stream (output);

    if (! stream.openedOk() || ! format.writeImageToStream (image, stream))
    {
        std::fprintf (stderr, "failed writing: %s\n", output.getFullPathName().toRawUTF8());
        return 1;
    }

    return 0;
}
