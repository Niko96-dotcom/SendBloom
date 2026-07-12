#pragma once

#include <juce_core/juce_core.h>

#include <cstddef>
#include <memory>
#include <string_view>

namespace sendbloom::SafeXml
{

inline constexpr size_t kMaxDocumentBytes = 4u * 1024u * 1024u;

inline bool containsForbiddenDtd (const void* data, size_t size) noexcept
{
    if (data == nullptr || size == 0)
        return false;

    const auto bytes = std::string_view (static_cast<const char*> (data), size);
    const auto containsAsciiCaseInsensitive = [&] (std::string_view needle)
    {
        if (needle.size() > bytes.size())
            return false;

        for (size_t offset = 0; offset + needle.size() <= bytes.size(); ++offset)
        {
            bool matches = true;

            for (size_t i = 0; i < needle.size(); ++i)
            {
                auto value = bytes[offset + i];

                if (value >= 'a' && value <= 'z')
                    value = static_cast<char> (value - ('a' - 'A'));

                if (value != needle[i])
                {
                    matches = false;
                    break;
                }
            }

            if (matches)
                return true;
        }

        return false;
    };

    return containsAsciiCaseInsensitive ("<!DOCTYPE")
        || containsAsciiCaseInsensitive ("<!ENTITY");
}

inline bool acceptsDocument (const void* data, size_t size) noexcept
{
    return data != nullptr
        && size > 0
        && size <= kMaxDocumentBytes
        && ! containsForbiddenDtd (data, size);
}

inline std::unique_ptr<juce::XmlElement> parseDocument (const char* data, size_t size)
{
    if (! acceptsDocument (data, size))
        return {};

    return juce::parseXML (juce::String (data, size));
}

} // namespace sendbloom::SafeXml
