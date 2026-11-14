#include "../../include/AudioSettings.h"
#include "../../include/InputConfig.h"

#include <algorithm>
#include <filesystem>
#include <fstream>

float AudioSettings::toVolume(int step) {
    return static_cast<float>(std::clamp(step, 0, MAX_STEP)) / static_cast<float>(MAX_STEP);
}

bool AudioSettings::loadFromFile(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        return false;
    }
    AudioSettings loaded = *this;
    bool any = false;
    std::string line;
    while (std::getline(in, line)) {
        const std::size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        const std::string key = line.substr(0, eq);
        int value = 0;
        try {
            value = std::stoi(line.substr(eq + 1));
        } catch (...) {
            continue;  // not a number: skip the line
        }
        value = std::clamp(value, 0, MAX_STEP);
        if (key == "master") { loaded.master = value; any = true; }
        else if (key == "music") { loaded.music = value; any = true; }
        else if (key == "effects") { loaded.effects = value; any = true; }
    }
    if (!any) {
        return false;
    }
    *this = loaded;
    return true;
}

bool AudioSettings::saveToFile(const std::string& path) const {
    namespace fs = std::filesystem;
    std::error_code ignored;
    const fs::path target(path);
    if (target.has_parent_path()) {
        fs::create_directories(target.parent_path(), ignored);
    }
    std::ofstream out(path, std::ios::trunc);
    if (!out) {
        return false;
    }
    out << "master=" << master << "\n"
        << "music=" << music << "\n"
        << "effects=" << effects << "\n";
    return static_cast<bool>(out);
}

std::string AudioSettings::defaultConfigPath() {
    // Beside the key bindings.
    return (std::filesystem::path(InputConfig::defaultConfigPath()).parent_path() / "audio.cfg").string();
}
