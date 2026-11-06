module;

#include <cctype>
#include <cstdint>
#include <string_view>

export module http:util;

export namespace http::util
{

// Case sensitive string comparison.
inline bool iequals(std::string_view a, std::string_view b)
{
    if (a.size() != b.size())
        return false;

    return std::equal(a.begin(), a.end(), b.begin(), b.end(), [](char c1, char c2) {
        return std::tolower(uint8_t(c1)) == std::tolower(uint8_t(c2));
    });
}

// Case-sensitive string contains.
inline bool icontains(std::string_view haystack, std::string_view needle)
{
    if (needle.empty())
        return true;
    for (std::size_t i = 0; i + needle.size() <= haystack.size(); ++i) {
        bool match = true;
        for (std::size_t j = 0; j < needle.size(); ++j) {
            if (std::tolower(uint8_t(haystack[i + j])) != std::tolower(uint8_t(needle[j]))) {
                match = false;
                break;
            }
        }
        if (match)
            return true;
    }
    return false;
}

} // namespace http::util