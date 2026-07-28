#pragma once

#include <string>

class Party;
class QuestLog;

// Handles save/load serialization for the game state.
// Uses a simple text-based format (key=value lines).
// Gameplay logic only — no rendering dependencies.
class SaveSystem {
public:
    SaveSystem();
    ~SaveSystem() = default;

    // Save the full game state to a file.
    // Returns true on success.
    bool saveGame(const std::string& filepath, Party* party, QuestLog* questLog);

    // Load the game state from a file.
    // Returns true on success. The party and questLog are populated from the file.
    bool loadGame(const std::string& filepath, Party* party, QuestLog* questLog);

    // Check if a save file exists.
    bool saveExists(const std::string& filepath) const;

private:
    void serializeParty(std::string& output, Party* party);
    void serializeQuestLog(std::string& output, QuestLog* questLog);
    bool parseParty(const std::string& input, Party* party);
    bool parseQuestLog(const std::string& input, QuestLog* questLog);
};
