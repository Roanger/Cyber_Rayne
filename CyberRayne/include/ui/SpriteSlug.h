#pragma once

#include <cctype>
#include <string>

// The asset filename stem for a display name: lowercase, with anything that is
// not a letter or digit collapsed to a single underscore.
// "Street Thug" -> "street_thug", "Black ICE" -> "black_ice".
inline std::string spriteSlug(const std::string& name) {
    std::string slug;
    for (unsigned char c : name) {
        if (std::isalnum(c)) {
            slug.push_back(static_cast<char>(std::tolower(c)));
        } else if (!slug.empty() && slug.back() != '_') {
            slug.push_back('_');
        }
    }
    while (!slug.empty() && slug.back() == '_') {
        slug.pop_back();
    }
    return slug;
}
