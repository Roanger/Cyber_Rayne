#pragma once

#include <string>

// The player's volume choices, in steps of ten percent (0 to 10). Kept in
// its own small file beside the key bindings so they survive between runs.
//
// Gameplay logic only.
struct AudioSettings {
    static constexpr int MAX_STEP = 10;

    int master = 10;
    int music = 7;
    int effects = 10;

    // A step as the 0.0-1.0 volume AudioManager takes.
    static float toVolume(int step);

    // key=value lines. A missing or unusable file leaves the current values
    // alone and returns false; out-of-range numbers are clamped.
    bool loadFromFile(const std::string& path);
    // Creates missing directories.
    bool saveToFile(const std::string& path) const;
    // $XDG_CONFIG_HOME/CyberRayne/audio.cfg (normally ~/.config/CyberRayne/).
    static std::string defaultConfigPath();
};
