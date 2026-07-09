#include <catch2/catch_test_macros.hpp>
#include <juce_core/juce_core.h>

namespace
{

juce::String makeBillionLaughsXml (int levels)
{
    juce::String xml = R"(<?xml version="1.0"?>
<!DOCTYPE doc [
<!ENTITY a1 "1234567890">)";

    for (int i = 1; i < levels; ++i)
    {
        const auto prev = "a" + juce::String (i);
        const auto next = "a" + juce::String (i + 1);
        xml << "\n<!ENTITY " << next << " \"&" << prev << ";&" << prev << ";&" << prev << ";&" << prev << ";&" << prev
            << ";&" << prev << ";&" << prev << ";&" << prev << ";&" << prev << ";&" << prev << ";\">";
    }

    const auto finalEntity = "a" + juce::String (levels);
    xml << "\n]>\n<root>&" << finalEntity << ";</root>";
    return xml;
}

} // namespace

TEST_CASE ("XmlDocument rejects exponential entity expansion", "[xml][security]")
{
    juce::XmlDocument doc (makeBillionLaughsXml (8));
    const auto element = doc.getDocumentElement();

    REQUIRE (element == nullptr);
    REQUIRE (doc.getLastParseError().containsIgnoreCase ("limit"));
}

TEST_CASE ("XmlDocument rejects oversized entity payload", "[xml][security]")
{
    const auto xml = R"(<?xml version="1.0"?>
<!DOCTYPE doc [
<!ENTITY big ")" + juce::String::repeatedString ("A", 2 * 1024 * 1024) + R"(">
]>
<root>&big;</root>)";

    juce::XmlDocument doc (xml);
    const auto element = doc.getDocumentElement();

    REQUIRE (element == nullptr);
    REQUIRE (doc.getLastParseError().containsIgnoreCase ("limit"));
}

TEST_CASE ("XmlDocument rejects truncated DTD input", "[xml][security]")
{
    const juce::String xml = R"(<?xml version="1.0"?>
<!DOCTYPE doc [
<!ENTITY a "value"
<root>&a;</root>)";

    juce::XmlDocument doc (xml);
    const auto element = doc.getDocumentElement();

    REQUIRE (element == nullptr);
    REQUIRE (doc.getLastParseError().isNotEmpty());
}

TEST_CASE ("XmlDocument rejects external SYSTEM entities", "[xml][security]")
{
    const juce::String xml = R"(<?xml version="1.0"?>
<!DOCTYPE doc SYSTEM "evil.dtd">
<root/>)";

    juce::XmlDocument doc (xml);
    const auto element = doc.getDocumentElement();

    REQUIRE (element == nullptr);
    REQUIRE (doc.getLastParseError().containsIgnoreCase ("SYSTEM"));
}

TEST_CASE ("XmlDocument parses normal APVTS-style state XML", "[xml][security]")
{
    const juce::String xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<SendBloomParams>
  <PARAM id="input_gain" value="0.55"/>
  <PARAM id="size" value="0.35"/>
</SendBloomParams>)";

    const auto element = juce::parseXML (xml);

    REQUIRE (element != nullptr);
    REQUIRE (element->hasTagName ("SendBloomParams"));
    REQUIRE (element->getChildByName ("PARAM") != nullptr);
    REQUIRE (element->getChildByName ("PARAM")->getStringAttribute ("id") == "input_gain");
}

TEST_CASE ("XmlDocument allows small nested entity definitions", "[xml][security]")
{
    const juce::String xml = R"(<?xml version="1.0"?>
<!DOCTYPE doc [
<!ENTITY greeting "hello">
]>
<doc>&greeting;</doc>)";

    const auto element = juce::parseXML (xml);

    REQUIRE (element != nullptr);
    REQUIRE (element->hasTagName ("doc"));
    REQUIRE (element->getAllSubText().trim() == "hello");
}
