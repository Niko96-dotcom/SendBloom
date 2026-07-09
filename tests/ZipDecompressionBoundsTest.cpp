#include <catch2/catch_test_macros.hpp>
#include <juce_core/juce_core.h>

namespace
{

juce::MemoryBlock createCompressedZipEntry (const juce::String& entryName, const juce::String& content)
{
    juce::MemoryBlock contentBlock;
    juce::MemoryOutputStream contentStream (contentBlock, false);
    contentStream << content;

    juce::ZipFile::Builder builder;
    builder.addEntry (new juce::MemoryInputStream (contentBlock, false),
                      9,
                      entryName,
                      juce::Time::getCurrentTime());

    juce::MemoryBlock zipData;
    juce::MemoryOutputStream zipStream (zipData, false);
    builder.writeToStream (zipStream, nullptr);
    return zipData;
}

juce::MemoryBlock patchZipUncompressedSize (juce::MemoryBlock zipData, juce::uint32 inflatedSize)
{
    const auto writeInt = [&] (const int writePos)
    {
        auto* bytes = static_cast<juce::uint8*> (zipData.getData());
        bytes[writePos + 0] = (juce::uint8) (inflatedSize);
        bytes[writePos + 1] = (juce::uint8) (inflatedSize >> 8);
        bytes[writePos + 2] = (juce::uint8) (inflatedSize >> 16);
        bytes[writePos + 3] = (juce::uint8) (inflatedSize >> 24);
    };

    const auto* bytes = static_cast<const juce::uint8*> (zipData.getData());

    for (int i = 0; i + 4 <= (int) zipData.getSize(); ++i)
    {
        const auto signature = juce::ByteOrder::littleEndianInt (bytes + i);

        if (signature == 0x04034b50)
            writeInt (i + 22);
        else if (signature == 0x02014b50)
            writeInt (i + 24);
    }

    return zipData;
}

juce::MemoryBlock createDeflatedPayload (const juce::String& content)
{
    juce::MemoryBlock compressed;
    juce::MemoryOutputStream compressedStream (compressed, false);
    juce::GZIPCompressorOutputStream compressor (compressedStream,
                                                 9,
                                                 juce::GZIPCompressorOutputStream::windowBitsRaw);
    compressor << content;
    compressor.flush();
    return compressed;
}

} // namespace

TEST_CASE ("ZipFile rejects entries above maximum uncompressed size", "[zip][security]")
{
    const auto previousLimit = juce::ZipFile::getMaximumUncompressedEntrySize();
    juce::ZipFile::setMaximumUncompressedEntrySize (1024);

    const auto zipData = patchZipUncompressedSize (createCompressedZipEntry ("bomb.txt", "small"), 8 * 1024 * 1024);
    juce::MemoryInputStream input (zipData, false);
    juce::ZipFile zip (input);

    REQUIRE (zip.getNumEntries() == 1);
    REQUIRE (zip.createStreamForEntry (0) == nullptr);

    juce::ZipFile::setMaximumUncompressedEntrySize (previousLimit);
}

TEST_CASE ("ZipFile still opens legitimate compressed entries", "[zip][security]")
{
    const auto zipData = createCompressedZipEntry ("index.html", "<html>ok</html>");
    juce::MemoryInputStream input (zipData, false);
    juce::ZipFile zip (input);

    REQUIRE (zip.getNumEntries() == 1);

    std::unique_ptr<juce::InputStream> stream (zip.createStreamForEntry (0));
    REQUIRE (stream != nullptr);
    REQUIRE (stream->readEntireStreamAsString() == "<html>ok</html>");
}

TEST_CASE ("GZIP decompressor stops when output exceeds declared length", "[zip][security]")
{
    const auto compressed = createDeflatedPayload ("toolong");
    juce::MemoryInputStream input (compressed, false);
    juce::GZIPDecompressorInputStream decompressor (&input,
                                                    false,
                                                    juce::GZIPDecompressorInputStream::deflateFormat,
                                                    3);

    char buffer[16]{};
    const auto bytesRead = decompressor.read (buffer, (int) sizeof (buffer));

    REQUIRE (bytesRead == 3);
    REQUIRE (decompressor.getPosition() == 3);
    REQUIRE (decompressor.isExhausted());
}

TEST_CASE ("GZIP decompressor does not require inflated declared length bytes", "[zip][security]")
{
    const auto compressed = createDeflatedPayload ("small");
    juce::MemoryInputStream input (compressed, false);
    juce::GZIPDecompressorInputStream decompressor (&input,
                                                    false,
                                                    juce::GZIPDecompressorInputStream::deflateFormat,
                                                    1024 * 1024);

    char buffer[16]{};
    const auto bytesRead = decompressor.read (buffer, (int) sizeof (buffer));

    REQUIRE (bytesRead == (int) juce::String ("small").getNumBytesAsUTF8());
    REQUIRE (decompressor.isExhausted());
    REQUIRE (decompressor.getPosition() < decompressor.getTotalLength());
}
