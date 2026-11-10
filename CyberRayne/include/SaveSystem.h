#pragma once

#include <string>

class Party;
class QuestLog;

// Where the party is in the world. Optional on both save and load, so callers
// that only care about the party and quests can ignore it.
struct SaveWorldState {
    std::string mapName;
    // Load only: whether the file had a [BAG] section at all. Saves written
    // before the shared bag existed do not, which is different from a bag
    // that was saved empty.
    bool hasBag = false;
};

// Handles save/load serialization for the game state.
// Uses a simple text-based format (key=value lines).
// Gameplay logic only — no rendering dependencies.
class SaveSystem {
public:
    SaveSystem();
    ~SaveSystem() = default;

    // Save the full game state to a file.
    // Returns true on success.
    bool saveGame(const std::string& filepath, Party* party, QuestLog* questLog,
                  const SaveWorldState* world = nullptr);

    // Load the game state from a file.
    // Returns true on success. The party and questLog are populated from the file.
    // If world is given it is filled from the file; mapName stays empty for
    // saves written before the map was recorded.
    bool loadGame(const std::string& filepath, Party* party, QuestLog* questLog,
                  SaveWorldState* world = nullptr);

    // Check if a save file exists.
    bool saveExists(const std::string& filepath) const;

    // The per-user save file: $XDG_DATA_HOME/CyberRayne/savegame.txt (or
    // ~/.local/share/...) on Linux, %APPDATA%\CyberRayne on Windows. Falls back
    // to the working directory if neither is set. Does not create anything;
    // saveGame creates missing directories.
    static std::string defaultSavePath();

private:
    void serializeParty(std::string& output, Party* party);
    void serializeQuestLog(std::string& output, QuestLog* questLog);
    bool parseParty(const std::string& input, Party* party);
    void parseBag(const std::string& input, Party* party);
    bool parseQuestLog(const std::string& input, QuestLog* questLog);
};
