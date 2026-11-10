#include "../../include/SaveSystem.h"
#include "../../include/Party.h"
#include "../../include/Player.h"
#include "../../include/Inventory.h"
#include "../../include/Item.h"
#include "../../include/QuestLog.h"
#include "../../include/Quest.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <filesystem>
#include <unordered_map>

namespace {

std::string trim(const std::string& s) {
    std::string result = s;
    while (!result.empty() && (result.front() == ' ' || result.front() == '\t' || result.front() == '\r')) {
        result.erase(result.begin());
    }
    while (!result.empty() && (result.back() == ' ' || result.back() == '\t' || result.back() == '\r')) {
        result.pop_back();
    }
    return result;
}

} // namespace

SaveSystem::SaveSystem() = default;

bool SaveSystem::saveExists(const std::string& filepath) const {
    return std::filesystem::exists(filepath);
}

std::string SaveSystem::defaultSavePath() {
    namespace fs = std::filesystem;
    fs::path dir;
#ifdef _WIN32
    if (const char* appData = std::getenv("APPDATA"); appData && *appData) {
        dir = fs::path(appData) / "CyberRayne";
    }
#else
    if (const char* xdg = std::getenv("XDG_DATA_HOME"); xdg && *xdg) {
        dir = fs::path(xdg) / "CyberRayne";
    } else if (const char* home = std::getenv("HOME"); home && *home) {
        dir = fs::path(home) / ".local" / "share" / "CyberRayne";
    }
#endif
    return (dir / "savegame.txt").string();
}

bool SaveSystem::saveGame(const std::string& filepath, Party* party, QuestLog* questLog,
                          const SaveWorldState* world) {
    std::string output;
    output += "[SAVEFILE]\n";
    output += "version=2\n";

    if (world) {
        output += "[WORLD]\n";
        output += "map=" + world->mapName + "\n";
    }

    if (party) {
        serializeParty(output, party);
    }
    if (questLog) {
        serializeQuestLog(output, questLog);
    }

    // Write to a temporary file and rename it over the real one, so a crash
    // or full disk mid-write cannot destroy the previous save.
    namespace fs = std::filesystem;
    std::error_code ec;
    const fs::path target(filepath);
    if (target.has_parent_path()) {
        fs::create_directories(target.parent_path(), ec);
    }

    const fs::path temp = fs::path(filepath + ".tmp");
    {
        std::ofstream file(temp);
        if (!file.is_open()) {
            std::cerr << "Failed to open save file for writing: " << temp.string() << std::endl;
            return false;
        }
        file << output;
        file.flush();
        if (!file.good()) {
            std::cerr << "Failed while writing save file: " << temp.string() << std::endl;
            return false;
        }
    }

    fs::rename(temp, target, ec);
    if (ec) {
        std::cerr << "Failed to move save file into place: " << ec.message() << std::endl;
        fs::remove(temp, ec);
        return false;
    }
    return true;
}

bool SaveSystem::loadGame(const std::string& filepath, Party* party, QuestLog* questLog,
                          SaveWorldState* world) {
    if (!saveExists(filepath)) {
        std::cerr << "Save file does not exist: " << filepath << std::endl;
        return false;
    }

    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open save file for reading: " << filepath << std::endl;
        return false;
    }

    std::stringstream ss;
    ss << file.rdbuf();
    std::string content = ss.str();
    file.close();

    bool partyLoaded = false;
    bool questLogLoaded = false;

    if (party) {
        partyLoaded = parseParty(content, party);
    }
    if (questLog) {
        questLogLoaded = parseQuestLog(content, questLog);
    }

    if (world) {
        world->mapName.clear();
        world->hasBag = false;
        std::istringstream lines(content);
        std::string line;
        bool inWorld = false;
        while (std::getline(lines, line)) {
            line = trim(line);
            if (line == "[BAG]") {
                world->hasBag = true;
            }
            if (line == "[WORLD]") {
                inWorld = true;
            } else if (!line.empty() && line[0] == '[') {
                inWorld = false;
            } else if (inWorld && line.rfind("map=", 0) == 0) {
                world->mapName = line.substr(4);
            }
        }
    }

    return partyLoaded || questLogLoaded;
}

void SaveSystem::serializeParty(std::string& output, Party* party) {
    output += "[PARTY]\n";
    output += "member_count=" + std::to_string(party->size()) + "\n";

    for (std::size_t i = 0; i < party->size(); ++i) {
        Player* member = party->getMember(i);
        if (!member) continue;

        output += "member_" + std::to_string(i) + "_name=" + member->getName() + "\n";
        output += "member_" + std::to_string(i) + "_class=" + std::to_string(static_cast<int>(member->getCharacterClass())) + "\n";
        output += "member_" + std::to_string(i) + "_level=" + std::to_string(member->getLevel()) + "\n";
        output += "member_" + std::to_string(i) + "_health=" + std::to_string(member->getHealth()) + "\n";
        output += "member_" + std::to_string(i) + "_maxhealth=" + std::to_string(member->getMaxHealth()) + "\n";
        output += "member_" + std::to_string(i) + "_mana=" + std::to_string(member->getMana()) + "\n";
        output += "member_" + std::to_string(i) + "_maxmana=" + std::to_string(member->getMaxMana()) + "\n";
        output += "member_" + std::to_string(i) + "_str=" + std::to_string(member->getStrength()) + "\n";
        output += "member_" + std::to_string(i) + "_mag=" + std::to_string(member->getMagic()) + "\n";
        output += "member_" + std::to_string(i) + "_spd=" + std::to_string(member->getSpeed()) + "\n";
        output += "member_" + std::to_string(i) + "_def=" + std::to_string(member->getDefense()) + "\n";
        output += "member_" + std::to_string(i) + "_exp=" + std::to_string(member->getExperience()) + "\n";
        output += "member_" + std::to_string(i) + "_gold=" + std::to_string(member->getGold()) + "\n";
        output += "member_" + std::to_string(i) + "_x=" + std::to_string(member->getX()) + "\n";
        output += "member_" + std::to_string(i) + "_y=" + std::to_string(member->getY()) + "\n";
        // Gear is stored by catalog ID; an empty value means the slot is bare.
        output += "member_" + std::to_string(i) + "_weapon=" +
                  (member->getEquippedWeapon() ? member->getEquippedWeapon()->getId() : "") + "\n";
        output += "member_" + std::to_string(i) + "_armor=" +
                  (member->getEquippedArmor() ? member->getEquippedArmor()->getId() : "") + "\n";
    }

    const auto& stacks = party->getBag().getStacks();
    output += "[BAG]\n";
    output += "stack_count=" + std::to_string(stacks.size()) + "\n";
    for (std::size_t i = 0; i < stacks.size(); ++i) {
        output += "stack_" + std::to_string(i) + "_id=" + stacks[i].id + "\n";
        output += "stack_" + std::to_string(i) + "_count=" + std::to_string(stacks[i].count) + "\n";
    }
}

void SaveSystem::serializeQuestLog(std::string& output, QuestLog* questLog) {
    output += "[QUESTLOG]\n";
    output += "quest_count=" + std::to_string(questLog->getQuests().size()) + "\n";

    for (std::size_t i = 0; i < questLog->getQuests().size(); ++i) {
        const Quest* quest = questLog->getQuests()[i].get();
        if (!quest) continue;

        output += "quest_" + std::to_string(i) + "_id=" + std::to_string(quest->getId()) + "\n";
        output += "quest_" + std::to_string(i) + "_name=" + quest->getName() + "\n";
        output += "quest_" + std::to_string(i) + "_desc=" + quest->getDescription() + "\n";
        output += "quest_" + std::to_string(i) + "_state=" + std::to_string(static_cast<int>(quest->getState())) + "\n";
        output += "quest_" + std::to_string(i) + "_exp=" + std::to_string(quest->getExperienceReward()) + "\n";
        output += "quest_" + std::to_string(i) + "_gold=" + std::to_string(quest->getGoldReward()) + "\n";
        output += "quest_" + std::to_string(i) + "_objcount=" + std::to_string(quest->getObjectives().size()) + "\n";

        for (std::size_t j = 0; j < quest->getObjectives().size(); ++j) {
            const auto& obj = quest->getObjectives()[j];
            output += "quest_" + std::to_string(i) + "_obj_" + std::to_string(j) + "_desc=" + obj.getDescription() + "\n";
            output += "quest_" + std::to_string(i) + "_obj_" + std::to_string(j) + "_state=" + std::to_string(static_cast<int>(obj.getState())) + "\n";
        }
    }
}

bool SaveSystem::parseParty(const std::string& input, Party* party) {
    // Simple line-based parser.
    std::istringstream stream(input);
    std::string line;
    bool inPartySection = false;
    int memberCount = 0;

    // Collect member data into a map for easy lookup.
    std::unordered_map<std::string, std::string> values;

    while (std::getline(stream, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        if (line == "[PARTY]") {
            inPartySection = true;
            continue;
        }
        if (line[0] == '[' && inPartySection) {
            // Left the party section.
            break;
        }
        if (!inPartySection) continue;

        // Parse key=value.
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string value = line.substr(eq + 1);
        values[key] = value;

        if (key == "member_count") {
            memberCount = std::stoi(value);
        }
    }

    if (memberCount == 0) {
        return false;
    }

    for (int i = 0; i < memberCount; ++i) {
        std::string prefix = "member_" + std::to_string(i) + "_";

        auto getStr = [&](const std::string& field) -> std::string {
            auto it = values.find(prefix + field);
            return it != values.end() ? it->second : "";
        };
        auto getInt = [&](const std::string& field, int fallback = 0) -> int {
            auto it = values.find(prefix + field);
            if (it == values.end()) return fallback;
            try {
                return std::stoi(it->second);
            } catch (const std::exception&) {
                std::cerr << "Save field " << prefix << field << " is not a number: '"
                          << it->second << "' — using " << fallback << std::endl;
                return fallback;
            }
        };
        auto getFloat = [&](const std::string& field) -> float {
            auto it = values.find(prefix + field);
            if (it == values.end()) return 0.0f;
            try {
                return std::stof(it->second);
            } catch (const std::exception&) {
                return 0.0f;
            }
        };

        std::string name = getStr("name");
        if (name.empty()) continue;

        // The class is stored as a bare enum ordinal. A missing or corrupt value
        // would otherwise be cast straight into the enum, producing a Player that
        // falls through every class switch with no default arm.
        int charClassInt = getInt("class", -1);
        if (!Player::isValidClassOrdinal(charClassInt)) {
            std::cerr << "Save has invalid class ordinal " << charClassInt << " for " << name
                      << " — defaulting to Solo." << std::endl;
            charClassInt = static_cast<int>(Player::CharacterClass::SOLO);
        }

        auto charClass = static_cast<Player::CharacterClass>(charClassInt);
        auto player = std::make_unique<Player>(charClass, name);

        player->setPosition(getFloat("x"), getFloat("y"));

        // Derived stats (max HP/MP, STR, MAG, SPD, DEF) are not stored — they
        // are rebuilt by replaying levelUp() from the class base up to the saved
        // level, which also re-grants the class ability kit. Note setLevel()
        // must NOT be called first: it would satisfy the loop condition
        // immediately and leave the character at level-1 stats.
        const int savedLevel = getInt("level", 1);
        while (player->getLevel() < savedLevel) {
            player->levelUp();
        }

        // Belt and braces: abilities are deterministic from class and level, so
        // this is a no-op after a clean replay, but it keeps a save written by a
        // future version with a different curve consistent.
        player->refreshAbilities();

        // Exact health/mana last, since levelUp() full-heals.
        player->setHealth(getInt("health"));
        player->setMana(getInt("mana"));

        // Add XP and gold.
        int savedExp = getInt("exp");
        if (savedExp > 0) {
            // We add XP directly to the field via addExperience, but that would
            // trigger more level-ups. Instead, we just set it through the public API.
            // For the vertical slice, we add it as raw XP without level-up checks.
            // Since addExperience may level up, we use it but the level is already correct.
            // The XP will be toward the next level.
            player->addExperience(savedExp);
        }

        int savedGold = getInt("gold");
        if (savedGold > 0) {
            player->addGold(savedGold);
        }

        // An ID the catalog no longer knows leaves the slot bare rather than
        // failing the whole load.
        if (auto weapon = ItemCatalog::create(getStr("weapon"))) {
            player->equipWeapon(std::move(weapon));
        }
        if (auto armor = ItemCatalog::create(getStr("armor"))) {
            player->equipArmor(std::move(armor));
        }

        party->addMember(std::move(player));
    }

    parseBag(input, party);
    return true;
}

void SaveSystem::parseBag(const std::string& input, Party* party) {
    std::istringstream stream(input);
    std::string line;
    bool inBag = false;
    std::unordered_map<std::string, std::string> values;

    while (std::getline(stream, line)) {
        line = trim(line);
        if (line == "[BAG]") {
            inBag = true;
            continue;
        }
        if (!line.empty() && line[0] == '[') {
            inBag = false;
            continue;
        }
        if (!inBag) continue;
        const size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        values[line.substr(0, eq)] = line.substr(eq + 1);
    }

    auto toInt = [&values](const std::string& key) -> int {
        auto it = values.find(key);
        if (it == values.end()) return 0;
        try {
            return std::stoi(it->second);
        } catch (const std::exception&) {
            return 0;
        }
    };

    party->getBag().clear();
    const int stackCount = toInt("stack_count");
    for (int i = 0; i < stackCount; ++i) {
        const std::string prefix = "stack_" + std::to_string(i) + "_";
        auto idIt = values.find(prefix + "id");
        if (idIt == values.end()) continue;
        // Unknown IDs and bad counts are skipped: add() refuses both.
        if (!party->getBag().add(idIt->second, toInt(prefix + "count"))) {
            std::cerr << "Save has an unusable bag entry '" << idIt->second << "' — skipped." << std::endl;
        }
    }
}

bool SaveSystem::parseQuestLog(const std::string& input, QuestLog* questLog) {
    std::istringstream stream(input);
    std::string line;
    bool inQuestSection = false;
    int questCount = 0;

    std::unordered_map<std::string, std::string> values;

    while (std::getline(stream, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        if (line == "[QUESTLOG]") {
            inQuestSection = true;
            continue;
        }
        if (line[0] == '[' && inQuestSection) {
            break;
        }
        if (!inQuestSection) continue;

        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string value = line.substr(eq + 1);
        values[key] = value;

        if (key == "quest_count") {
            questCount = std::stoi(value);
        }
    }

    if (questCount == 0) {
        return false;
    }

    for (int i = 0; i < questCount; ++i) {
        std::string prefix = "quest_" + std::to_string(i) + "_";

        auto getStr = [&](const std::string& field) -> std::string {
            auto it = values.find(prefix + field);
            return it != values.end() ? it->second : "";
        };
        auto getInt = [&](const std::string& field) -> int {
            auto it = values.find(prefix + field);
            return it != values.end() ? std::stoi(it->second) : 0;
        };

        int questId = getInt("id");
        std::string questName = getStr("name");
        std::string questDesc = getStr("desc");
        int questStateInt = getInt("state");
        int questExp = getInt("exp");
        int questGold = getInt("gold");
        int objCount = getInt("objcount");

        if (questName.empty()) continue;

        auto quest = std::make_unique<Quest>(questId, questName, questDesc);
        quest->setExperienceReward(questExp);
        quest->setGoldReward(questGold);

        for (int j = 0; j < objCount; ++j) {
            std::string objPrefix = prefix + "obj_" + std::to_string(j) + "_";
            auto objIt = values.find(objPrefix + "desc");
            std::string objDesc = objIt != values.end() ? objIt->second : "";
            auto objStateIt = values.find(objPrefix + "state");
            int objStateInt = objStateIt != values.end() ? std::stoi(objStateIt->second) : 0;

            QuestObjective objective(objDesc);
            objective.setState(static_cast<QuestObjective::State>(objStateInt));
            quest->addObjective(objective);
        }

        quest->setState(static_cast<Quest::State>(questStateInt));
        questLog->addQuest(std::move(quest));
    }

    return true;
}
