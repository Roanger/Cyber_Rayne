#pragma once

#include <cstddef>
#include <string>
#include <vector>

// Break text into lines of at most maxChars characters, splitting at spaces.
// A single word longer than maxChars is hard-split. The bitmap font is
// monospace, so a character budget is also a pixel budget.
inline std::vector<std::string> wrapText(const std::string& text, std::size_t maxChars) {
    std::vector<std::string> lines;
    if (maxChars == 0) {
        return lines;
    }

    std::string line;
    std::size_t pos = 0;
    while (pos < text.size()) {
        while (pos < text.size() && text[pos] == ' ') ++pos;
        std::size_t end = text.find(' ', pos);
        if (end == std::string::npos) end = text.size();
        std::string word = text.substr(pos, end - pos);
        pos = end;
        if (word.empty()) continue;

        while (word.size() > maxChars) {
            if (!line.empty()) {
                lines.push_back(line);
                line.clear();
            }
            lines.push_back(word.substr(0, maxChars));
            word.erase(0, maxChars);
        }

        if (line.empty()) {
            line = word;
        } else if (line.size() + 1 + word.size() <= maxChars) {
            line += ' ';
            line += word;
        } else {
            lines.push_back(line);
            line = word;
        }
    }
    if (!line.empty()) {
        lines.push_back(line);
    }
    return lines;
}
